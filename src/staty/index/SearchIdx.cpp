// Copyright 2019, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Authors: Patrick Brosi <brosi@informatik.uni-freiburg.de>.

#include <algorithm>
#include <codecvt>
#include <fstream>
#include <locale>
#include <queue>
#include <string>
#include <vector>
#include "util/log/Log.h"

#include "staty/index/SearchIdx.h"

using staty::SearchIdx;
using staty::TripleList;
using staty::TupleList;

// _____________________________________________________________________________
void SearchIdx::build(std::vector<std::pair<IdRange, StatIdx>>& idxs) {
  _qGramIndex.clear();

  size_t nameid = 0;

  std::map<std::wstring, size_t> tokenIds;

  for (const auto& idx : idxs) {
    for (size_t gid = 0; gid < idx.second.getGroups().size(); gid++) {
      auto group = idx.second.getGroup(gid);

      // slightly prefer larger stations
      double groupScore = 1.0 + log(group->stations.size()) / 20;

      // dont index empty groups
      if (group->stations.size() == 0) continue;
      if (group->polyStations.size() == 0) continue;

      for (const auto& namep : group->uniqueNames) {
        // use wstring to get UTF-8 chars right
        const auto& name = namep.second;
        std::wstring wname =
            std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(name);

        _nameToGroup[nameid] = idx.first.gidStart + gid;
        _names.push_back(name);

        for (const auto& token : tokenize(wname)) {
          if (tokenIds.count(token)) {
            if (_inv[tokenIds.find(token)->second].size() == 0 ||
                _inv[tokenIds.find(token)->second].back().first != nameid) {
              // only use a token once per station
              _inv[tokenIds.find(token)->second].push_back(
                  {nameid, groupScore});
            }
          } else {
            tokenIds[token] = _tokens.size();
            _tokens.push_back(token);
            _inv.push_back(TupleList());
            _inv.back().push_back({nameid, groupScore});
          }

          size_t tokenId = tokenIds.find(token)->second;
          for (const auto& qGram : getQGrams(token)) {
            if (_qGramIndex[qGram].size() &&
                _qGramIndex[qGram].back().first == tokenId) {
              _qGramIndex[qGram].back().second++;
            } else {
              _qGramIndex[qGram].push_back({tokenId, 1});
            }
          }
        }
        nameid++;
      }
    }
  }

  for (auto& idx : _qGramIndex) std::sort(idx.second.begin(), idx.second.end());

  for (auto& idx : _qGramIndex) {
    TupleList tl;
    for (auto& cur : idx.second) {
      if (tl.size() != 0 && tl.back().first == cur.first)
        tl.back().second += cur.second;
      else
        tl.push_back(cur);
    }
    idx.second = tl;
  }
}

// _____________________________________________________________________________
std::vector<std::wstring> SearchIdx::getQGrams(const std::wstring& word) {
  std::vector<std::wstring> ret;

  std::wstring s;
  for (size_t i = 0; i < 2; ++i) s += '$';

  s += normalize(word);

  for (size_t i = 0; i < s.size() - 2; ++i) ret.push_back(s.substr(i, 3));

  return ret;
}

// _____________________________________________________________________________
std::vector<std::wstring> SearchIdx::tokenize(const std::wstring& str) {
  std::vector<std::wstring> ret;
  std::wstring cur;

  const wchar_t* seps = L"_-?'\"|!@#$%^&*()_+{}[]|<>.:,\\/";

  for (size_t i = 0; i < str.size(); i++) {
    if (std::iswspace(str[i]) || wcschr(seps, str[i])) {
      if (cur.size()) {
        ret.push_back(cur);
        cur.clear();
      }
    } else {
      cur += std::towlower(str[i]);
    }
  }

  if (cur.size()) ret.push_back(cur);

  return ret;
}

// _____________________________________________________________________________
std::wstring SearchIdx::normalize(const std::wstring& str) {
  std::wstring s;
  const wchar_t* unallowed = L"_-?'\"|!@#$%^&*()_+}|><.,\\";
  for (size_t i = 0; i < str.size(); ++i) {
    if (std::iswspace(str[i]) || wcschr(unallowed, str[i])) {
      continue;
    }
    s += std::towlower(str[i]);
  }

  return s;
}

// _____________________________________________________________________________
TripleList SearchIdx::find(const std::string& qry) const {
  TupleList ret;
  std::wstring wqry =
      std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(qry);

  const auto& tokens = tokenize(wqry);

  std::vector<TupleList> lists;

  for (const auto& token : tokens) {
    double delta = token.size() / 4.0;
    auto res = tokenFind(token);

    std::partial_sort(res.begin(),
                      res.begin() + std::min<size_t>(100, res.size()),
                      res.end(), resComp);

    std::map<size_t, double> bests;
    std::map<size_t, size_t> bestToken;

    size_t TOPK = 100;

    // res contains the 100 best token matches
    for (size_t i = 0; i < res.size() && i < TOPK; i++) {
      for (size_t j = 0; j < _inv[res[i].first].size(); j++) {
        double score =
            _inv[res[i].first][j].second * (delta / (1.0 + res[i].second));

        if (score > bests[_inv[res[i].first][j].first]) {
          bests[_inv[res[i].first][j].first] = score;
          bestToken[_inv[res[i].first][j].first] = res[i].first;
        }
      }
    }

    for (size_t i = 0; i < res.size() && i < TOPK; i++) {
      // inv[res[i]] contains all the names the token res[i] occurs in

      lists.push_back(_inv[res[i].first]);

      // give them a score based on their PED
      for (size_t j = 0; j < lists.back().size(); j++) {
        double score = lists.back()[j].second * (delta / (1.0 + res[i].second));
        // best is the token for this name that matched best for the input token
        size_t best = bestToken[lists.back()[j].first];

        // if it is not this token, we dont count it
        if (res[i].first != best) score = 0;
        lists.back()[j].second = score;
      }
    }
  }

  std::vector<const TupleList*> mlists;
  for (const auto& l : lists) mlists.push_back(&l);

  ret = lmerge(mlists);

  TripleList fr;

  for (const auto& r : ret) {
    fr.push_back({{_nameToGroup.find(r.first)->second, r.first}, r.second});
  }

  std::sort(fr.begin(), fr.end(), resGidCompInv);

  TripleList fin;
  for (const auto& r : fr) {
    if (fin.size() == 0 || fin.back().first.first != r.first.first)
      fin.push_back(r);
  }

  std::partial_sort(fin.begin(), fin.begin() + std::min<size_t>(fin.size(), 10),
                    fin.end(), resCompInv);

  TripleList top10;
  for (const auto& r : fin) {
    top10.push_back(r);
    if (top10.size() == 10) break;
  }

  return top10;
}

// _____________________________________________________________________________
TupleList SearchIdx::tokenFind(const std::wstring& prefix) const {
  TupleList result;

  size_t delta = prefix.size() / 4;

  std::vector<const TupleList*> lists;
  for (const auto& qGram : getQGrams(prefix)) {
    auto it = _qGramIndex.find(qGram);
    if (it != _qGramIndex.end()) lists.push_back(&it->second);
  }

  size_t threshold = prefix.size() - 3 * delta;

  for (const auto& tpl : lmerge(lists)) {
    if (tpl.second >= threshold) {
      size_t ped = util::prefixEditDist(prefix, _tokens[tpl.first], delta);
      if (ped <= delta) result.push_back({tpl.first, ped});
    }
  }

  return result;
}

// _____________________________________________________________________________
TupleList SearchIdx::lmerge(const std::vector<const TupleList*>& lists) {
  if (lists.size() == 0) return {};

  TupleList a = *lists[0];

  for (size_t k = 1; k < lists.size(); k++) {
    const auto& b = lists[k];

    TupleList ret;
    size_t i = 0;
    size_t j = 0;
    while (i < a.size() && j < b->size()) {
      if (a[i].first < (*b)[j].first) {
        ret.push_back(a[i]);
        i++;
      } else if ((*b)[j].first < a[i].first) {
        ret.push_back((*b)[j]);
        j++;
      } else {
        ret.push_back({a[i].first, a[i].second + (*b)[j].second});
        i++;
        j++;
      }
    }

    while (i < a.size()) {
      ret.push_back(a[i]);
      i++;
    }

    while (j < b->size()) {
      ret.push_back((*b)[j]);
      j++;
    }

    a = ret;
  }

  return a;
}
