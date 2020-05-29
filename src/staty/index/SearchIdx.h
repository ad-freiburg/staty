// Copyright 2019, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Authors: Patrick Brosi <brosi@informatik.uni-freiburg.de>.

#ifndef staty_INDEX_SEARCHIDX_H_
#define staty_INDEX_SEARCHIDX_H_

#include <string>
#include <unordered_map>
#include <vector>
#include "staty/index/StatIdx.h"

namespace staty {

typedef std::pair<size_t, double> Tuple;
typedef std::vector<Tuple> TupleList;
typedef std::pair<std::pair<size_t, size_t>, double> Triple;
typedef std::vector<Triple> TripleList;
typedef std::unordered_map<std::wstring, TupleList> Index;

inline bool resComp(const Tuple& lh, const Tuple& rh) {
  return lh.second < rh.second;
}

inline bool resCompInv(const Triple& lh, const Triple& rh) {
  return lh.second > rh.second;
}

inline bool resGidCompInv(const Triple& lh, const Triple& rh) {
  if (lh.first.first < rh.first.first) return true;
  if (lh.first.first == rh.first.first) {
    // if the scores are equal, order by the name id to prefer the original
    // name id, which is lowest!
    if (fabs(lh.second - rh.second) < 0.01)
      return lh.first.second < rh.first.second;
    return lh.second > rh.second;
  }
  return false;
}

class SearchIdx {
 public:
  explicit SearchIdx(std::vector<std::pair<IdRange, StatIdx>>& idxs) {
    build(idxs);
  }

  void build(std::vector<std::pair<IdRange, StatIdx>>& idxs);

  TripleList find(const std::string& qry) const;

  static std::wstring normalize(const std::wstring& str);
  static std::vector<std::wstring> tokenize(const std::wstring& str);

  static std::vector<std::wstring> getQGrams(const std::wstring& word);

  const Index& getIndex() const { return _qGramIndex; }

  const std::string& getName(size_t nid) const { return _names[nid]; }

 private:
  TupleList tokenFind(const std::wstring& prefix) const;
  static TupleList lmerge(const std::vector<const TupleList*>& lists);

  Index _qGramIndex;

  std::unordered_map<size_t, size_t> _nameToGroup;
  std::vector<std::wstring> _tokens;
  std::vector<std::string> _names;

  std::vector<TupleList> _inv;
};
}  // namespace staty

#endif  // staty_INDEX_SEARCHIDX_H_
