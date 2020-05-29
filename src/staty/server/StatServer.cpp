// Copyright 2019, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Authors: Patrick Brosi <brosi@informatik.uni-freiburg.de>

#include <codecvt>
#include <locale>
#include <vector>
#include "staty/build.h"
#include "staty/index.h"
#include "staty/server/StatServer.h"
#include "util/Misc.h"
#include "util/String.h"
#include "util/geo/Geo.h"
#include "util/log/Log.h"

using staty::Params;
using staty::StatServer;

// _____________________________________________________________________________
util::http::Answer StatServer::handle(const util::http::Req& req,
                                      int con) const {
  UNUSED(con);
  util::http::Answer a;
  try {
    Params params;
    auto cmd = parseUrl(req.url, req.payload, &params);

    if (cmd == "/") {
      a = util::http::Answer(
          "200 OK",
          std::string(index_html,
                      index_html + sizeof index_html / sizeof index_html[0]));
      a.params["Content-Type"] = "text/html; charset=utf-8";
    } else if (cmd == "/build.js") {
      a = util::http::Answer(
          "200 OK", std::string(build_js, build_js + sizeof build_js /
                                                         sizeof build_js[0]));
      a.params["Content-Type"] = "application/javascript; charset=utf-8";
      a.params["Cache-Control"] = "public, max-age=10000";
    } else if (cmd == "/map") {
      a = handleMapReq(params);
    } else if (cmd == "/heatmap") {
      a = handleHeatMapReq(params);
    } else if (cmd == "/stat") {
      a = handleStatReq(params);
    } else if (cmd == "/group") {
      a = handleGroupReq(params);
    } else if (cmd == "/mgroup") {
      a = handleMGroupReq(params);
    } else if (cmd == "/search") {
      a = handleSearch(params);
    } else {
      a = util::http::Answer("404 Not Found", "dunno");
    }
  } catch (std::runtime_error e) {
    a = util::http::Answer("400 Bad Request", e.what());
  } catch (std::invalid_argument e) {
    a = util::http::Answer("400 Bad Request", e.what());
  } catch (...) {
    a = util::http::Answer("500 Internal Server Error",
                           "Internal Server Error.");
  }

  a.params["Access-Control-Allow-Origin"] = "*";
  a.params["Server"] = "staty";

  return a;
}

// _____________________________________________________________________________
util::http::Answer StatServer::handleHeatMapReq(const Params& pars) const {
  if (pars.count("bbox") == 0 || pars.find("bbox")->second.empty())
    throw std::invalid_argument("No bbox specified.");
  std::string cb;
  if (pars.count("cb")) cb = pars.find("cb")->second.c_str();
  auto box = util::split(pars.find("bbox")->second, ',');

  double z = 0;
  if (pars.count("z")) z = atoi(pars.find("z")->second.c_str());

  if (box.size() != 4) throw std::invalid_argument("Invalid request.");

  double lat1 = atof(box[0].c_str());
  double lng1 = atof(box[1].c_str());
  double lat2 = atof(box[2].c_str());
  double lng2 = atof(box[3].c_str());

  util::geo::DBox bbox(util::geo::DPoint(lat1, lng1),
                       util::geo::DPoint(lat2, lng2));

  std::stringstream json;

  size_t p = 5;
  if (z < 11) p = 3;
  if (z < 10) p = 2;
  if (z < 9) p = 2;
  if (z < 7) p = 1;

  json << std::fixed << std::setprecision(p);

  if (cb.size()) json << cb << "(";
  json << "{\"ok\":[";

  char sep = ' ';
  for (const auto& idxPair : _idxs) {
    const auto& idx = idxPair.second;

    const auto& hgOk = idx.getHeatGridOk(bbox, z);
    for (const auto& cluster : hgOk) {
      json << sep;
      sep = ',';

      json << "[" << cluster.latLng.getY() << "," << cluster.latLng.getX()
           << "," << cluster.size << "]";
    }
  }

  json << "],\"sugg\":[";

  sep = ' ';
  for (const auto& idxPair : _idxs) {
    const auto& idx = idxPair.second;

    const auto& hgSugg = idx.getHeatGridSugg(bbox, z);

    for (const auto& cluster : hgSugg) {
      json << sep;
      sep = ',';

      json << "[" << cluster.latLng.getY() << "," << cluster.latLng.getX()
           << "," << cluster.size << "]";
    }
  }

  json << "],\"err\":[";

  sep = ' ';
  for (const auto& idxPair : _idxs) {
    const auto& idx = idxPair.second;

    const auto& hgErr = idx.getHeatGridErr(bbox, z);
    for (const auto& cluster : hgErr) {
      json << sep;
      sep = ',';

      json << "[" << cluster.latLng.getY() << "," << cluster.latLng.getX()
           << "," << cluster.size << "]";
    }
  }

  json << "]}";

  if (cb.size()) json << ")";

  util::http::Answer answ = util::http::Answer("200 OK", json.str(), true);
  answ.params["Content-Type"] = "application/javascript; charset=utf-8";

  return answ;
}

// _____________________________________________________________________________
util::http::Answer StatServer::handleMapReq(const Params& pars) const {
  if (pars.count("bbox") == 0 || pars.find("bbox")->second.empty())
    throw std::invalid_argument("No bbox specified.");
  std::string cb;
  if (pars.count("cb")) cb = pars.find("cb")->second.c_str();
  auto box = util::split(pars.find("bbox")->second, ',');

  double z = 0;
  if (pars.count("z")) z = atoi(pars.find("z")->second.c_str());

  if (box.size() != 4) throw std::invalid_argument("Invalid request.");

  double lat1 = atof(box[0].c_str());
  double lng1 = atof(box[1].c_str());
  double lat2 = atof(box[2].c_str());
  double lng2 = atof(box[3].c_str());

  util::geo::DBox bbox(util::geo::DPoint(lat1, lng1),
                       util::geo::DPoint(lat2, lng2));

  size_t p = 7;
  if (z < 20) p = 6;
  if (z < 17) p = 5;
  if (z < 15) p = 4;
  if (z < 13) p = 3;
  if (z < 10) p = 2;
  if (z < 9) p = 2;
  if (z < 7) p = 1;

  std::stringstream json;

  json << std::fixed << std::setprecision(p);

  if (cb.size()) json << cb << "(";
  json << "{\"stats\":[";

  char sep = ' ';
  for (size_t did = 0; did < _idxs.size(); did++) {
    const auto& idx = _idxs[did].second;

    const auto& ret = idx.getStations(bbox);
    for (const auto& stat : ret) {
      json << sep;
      sep = ',';
      printStation(stat, did, z < 15, &json);
    }
  }

  json << "], \"groups\":[";
  sep = ' ';
  if (z > 13) {
    for (size_t did = 0; did < _idxs.size(); did++) {
      const auto& idx = _idxs[did].second;

      const auto& gret = idx.getGroups(bbox);
      for (const auto& group : gret) {
        json << sep;
        sep = ',';
        printGroup(group, did, z < 15, false, &json);
      }
    }
  }

  json << "], \"su\":[";
  sep = ' ';
  if (z > 15) {
    for (size_t did = 0; did < _idxs.size(); did++) {
      const auto& idx = _idxs[did].second;

      const auto& suggs = idx.getSuggestions(bbox);
      for (const auto& sugg : suggs) {
        if (sugg->arrow.size() < 2) continue;
        json << sep;
        sep = ',';
        printSugg(sugg, did, &json);
      }
    }
  }

  json << "], \"mgroups\":[";
  sep = ' ';
  if (z > 13) {
    for (size_t did = 0; did < _idxs.size(); did++) {
      const auto& idx = _idxs[did].second;

      const auto& mgroups = idx.getMetaGroups(bbox);
      for (const auto& mgroup : mgroups) {
        json << sep;
        sep = ',';
        printMetaGroup(mgroup, did, z < 15, &json);
      }
    }
  }

  json << "]}";

  if (cb.size()) json << ")";

  auto answ = util::http::Answer("200 OK", json.str(), true);
  answ.params["Content-Type"] = "application/javascript; charset=utf-8";

  return answ;
}

// _____________________________________________________________________________
void StatServer::printStation(const Station* stat, size_t did, bool simple,
                              std::ostream* out) const {
  const auto& range = _idxs[did].first;

  (*out) << "{\"i\":" << stat->id + range.sidStart << ",\"g\":[";

  std::string sep = "";

  if (simple) {
    (*out) << sep << "[" << stat->centroid.getY() << ","
           << stat->centroid.getX() << "]";
  } else {
    for (const auto& ll : stat->latLng) {
      (*out) << sep << "[" << ll.getY() << "," << ll.getX() << "]";
      sep = ",";
    }
  }

  (*out) << "]";

  if (stat->suggestions.size() > 0) (*out) << ",\"s\":1";

  if (stat->attrErrs.size()) (*out) << ",\"e\":" << stat->attrErrs.size();

  (*out) << "}";
}

// _____________________________________________________________________________
void StatServer::printSugg(const Suggestion* sugg, size_t did,
                           std::ostream* out) const {
  const auto& range = _idxs[did].first;

  std::vector<util::geo::DPoint> projArrow;

  for (const auto& p : sugg->arrow) {
    projArrow.push_back(util::geo::webMercToLatLng<double>(p.getX(), p.getY()));
  }

  if (sugg->station >= 0) {
    (*out) << "{\"i\":" << sugg->station + range.sidStart
           << ",\"t\":" << sugg->type << ",\"a\":[";
  } else {
    (*out) << "{\"i\":" << -(-sugg->station + (int64_t)range.gidStart)
           << ",\"t\":" << sugg->type << ",\"a\":[";
  }
  char sep = ' ';
  for (const auto& p : projArrow) {
    (*out) << sep << "[" << p.getY() << "," << p.getX() << "]";
    sep = ',';
  }
  (*out) << "]";
  (*out) << "}";
}

// _____________________________________________________________________________
void StatServer::printMetaGroup(const MetaGroup* mg, size_t did, bool simple,
                                std::ostream* out) const {
  (*out) << "{\"i\":" << mg->osmid << ",\"g\":[";
  char sep = ' ';
  if (simple) {
    (*out) << sep << "[" << mg->centroid.getY() << "," << mg->centroid.getX()
           << "]";
  } else {
    for (const auto& p : mg->llPoly.getOuter()) {
      (*out) << sep << "[" << p.getY() << "," << p.getX() << "]";
      sep = ',';
    }
  }
  (*out) << "]";
  bool sugg = mg->osmid == 1;  // || mg->suggestions.size() > 0;

  if (sugg) (*out) << ",\"s\":1";

  (*out) << "}";
}

// _____________________________________________________________________________
void StatServer::printGroup(const Group* group, size_t did, bool simple,
                            bool aggr, std::ostream* out) const {
  const auto& range = _idxs[did].first;

  (*out) << "{\"i\":" << group->id + range.gidStart << ",\"g\":[";
  char sep = ' ';
  if (simple) {
    (*out) << sep << "[" << group->centroid.getY() << ","
           << group->centroid.getX() << "]";
  } else {
    for (const auto& p : group->llPoly.getOuter()) {
      (*out) << sep << "[" << p.getY() << "," << p.getX() << "]";
      sep = ',';
    }
  }
  (*out) << "]";
  bool sugg = group->osmid == 1 || group->suggestions.size() > 0;
  int err = group->attrErrs.size();

  if (aggr) {
    for (const auto sid : group->polyStations) {
      sugg = sugg || _idxs[did].second.getStation(sid)->suggestions.size();
      err += _idxs[did].second.getStation(sid)->attrErrs.size();
    }
  }

  if (sugg) (*out) << ",\"s\":1";
  if (err) (*out) << ",\"e\":" << err;

  (*out) << "}";
}

// _____________________________________________________________________________
std::string StatServer::parseUrl(std::string u, std::string pl,
                                 std::map<std::string, std::string>* params) {
  auto parts = util::split(u, '?');

  if (parts.size() > 1) {
    auto kvs = util::split(parts[1], '&');
    for (const auto& kv : kvs) {
      auto kvp = util::split(kv, '=');
      if (kvp.size() == 1) kvp.push_back("");
      (*params)[util::urlDecode(kvp[0])] = util::urlDecode(kvp[1]);
    }
  }

  // also parse post data
  auto kvs = util::split(pl, '&');
  for (const auto& kv : kvs) {
    auto kvp = util::split(kv, '=');
    if (kvp.size() == 1) kvp.push_back("");
    (*params)[util::urlDecode(kvp[0])] = util::urlDecode(kvp[1]);
  }

  return util::urlDecode(parts.front());
}

// _____________________________________________________________________________
size_t StatServer::getDidBySid(size_t sid) const {
  for (size_t i = 0; i < _idxs.size(); i++) {
    auto range = _idxs[i].first;
    if (sid >= range.sidStart && sid <= range.sidEnd) return i;
  }

  return 0;
}

// _____________________________________________________________________________
size_t StatServer::getDidBySuggid(size_t suggid) const {
  for (size_t i = 0; i < _idxs.size(); i++) {
    auto range = _idxs[i].first;
    if (suggid >= range.suggIdStart && suggid <= range.suggIdEnd) return i;
  }

  return 0;
}

// _____________________________________________________________________________
size_t StatServer::getDidByMGid(size_t mgid) const {
  for (size_t i = 0; i < _idxs.size(); i++) {
    if (_idxs[i].second.getMetaGroup(mgid)) return i;
  }

  return 0;
}

// _____________________________________________________________________________
size_t StatServer::getDidByGid(size_t gid) const {
  for (size_t i = 0; i < _idxs.size(); i++) {
    auto range = _idxs[i].first;
    if (gid >= range.gidStart && gid <= range.gidEnd) return i;
  }

  return 0;
}

// _____________________________________________________________________________
util::http::Answer StatServer::handleSearch(const Params& pars) const {
  if (pars.count("q") == 0) throw std::invalid_argument("No query given.");
  std::string cb;
  if (pars.count("cb")) cb = pars.find("cb")->second.c_str();

  std::string query = pars.find("q")->second.c_str();
  if (query.size() > 100) query.resize(100);

  auto results = _searchIdx.find(query);

  std::stringstream json;

  json << std::setprecision(10);

  if (cb.size()) json << cb << "(";
  json << "[";

  std::string sep = "";
  for (const auto& res : results) {
    size_t gid = res.first.first;
    size_t did = getDidByGid(gid);

    const auto& range = _idxs[did].first;
    const auto& idx = _idxs[did].second;
    size_t idxGid = gid - range.gidStart;
    const auto group = idx.getGroup(idxGid);

    bool asStat = group->polyStations.size() == 1;

    json << sep << "{\"n\":";
    std::string name = group->uniqueNames.front().second;
    json << "\"" << util::jsonStringEscape(name) << "\"";
    std::string via = _searchIdx.getName(res.first.second);
    if (name != via) {
      json << ",\"v\":"
           << "\"" << util::jsonStringEscape(via) << "\"";
    }

    if (asStat) {
      auto stat = idx.getStation(group->polyStations.front());
      if (stat->osmid < 0) json << ",\"w\":1";
      json << ",\"s\":";
      printStation(stat, did, true, &json);
    } else {
      json << ",\"g\":";
      printGroup(group, did, true, true, &json);
    }

    sep = ",";
    json << "}";
  }

  json << "]";

  if (cb.size()) json << ")";

  auto answ = util::http::Answer("200 OK", json.str(), true);
  answ.params["Content-Type"] = "application/javascript; charset=utf-8";

  return answ;
}

// _____________________________________________________________________________
util::http::Answer StatServer::handleMGroupReq(const Params& pars) const {
  if (pars.count("id") == 0 || pars.find("id")->second.empty())
    throw std::invalid_argument("No ID specified.");
  std::string cb;
  if (pars.count("cb")) cb = pars.find("cb")->second.c_str();

  size_t mgid = atol(pars.find("id")->second.c_str());

  size_t did = getDidByMGid(mgid);

  const auto& range = _idxs[did].first;
  const auto& idx = _idxs[did].second;

  const auto& mgroup = idx.getMetaGroup(mgid);

  if (!mgroup) return util::http::Answer("404 Not Found", "Group not found.");

  std::stringstream json;

  json << std::setprecision(10);

  auto centroid = mgroup->centroid;

  if (cb.size()) json << cb << "(";
  json << "{\"id\":" << mgid << ","
       << "\"lat\":" << centroid.getY() << ","
       << "\"lon\":" << centroid.getX() << ","
       << "\"osmid\":" << mgid << ",\"groups\":[";

  char sep = ' ';
  for (const auto& gid : mgroup->groups) {
    json << sep;
    sep = ',';
    const auto& group = idx.getGroup(gid);
    json << "{\"id\":" << gid + range.gidStart << ","
         << "\"osmid\":" << group->osmid << ","
         << "\"attrs\":{";

    char sep = ' ';

    for (const auto& par : group->attrs) {
      json << sep;
      sep = ',';
      json << "\"" << util::jsonStringEscape(par.first) << "\":[";

      char sep2 = ' ';
      for (const auto& val : par.second) {
        json << sep2;
        sep2 = ',';
        json << "\"" << util::jsonStringEscape(val) << "\"";
      }
      json << "]";
    }

    json << "}";
    if (group->suggestions.size() > 0) json << ",\"s\":1";
    if (group->attrErrs.size()) json << ",\"e\":" << group->attrErrs.size();

    json << "}";
  }

  json << "]";

  json << "}";

  if (cb.size()) json << ")";

  auto answ = util::http::Answer("200 OK", json.str(), true);
  answ.params["Content-Type"] = "application/javascript; charset=utf-8";

  return answ;
}

// _____________________________________________________________________________
util::http::Answer StatServer::handleGroupReq(const Params& pars) const {
  if (pars.count("id") == 0 || pars.find("id")->second.empty())
    throw std::invalid_argument("No ID specified.");
  std::string cb;
  if (pars.count("cb")) cb = pars.find("cb")->second.c_str();

  size_t gid = atol(pars.find("id")->second.c_str());

  size_t did = getDidByGid(gid);

  const auto& range = _idxs[did].first;
  const auto& idx = _idxs[did].second;
  size_t idxGid = gid - range.gidStart;

  const auto& group = idx.getGroup(idxGid);

  if (!group) return util::http::Answer("404 Not Found", "Group not found.");

  std::stringstream json;

  json << std::setprecision(10);

  auto centroid = group->centroid;

  if (cb.size()) json << cb << "(";
  json << "{\"id\":" << gid << ","
       << "\"osmid\":" << group->osmid << ","
       << "\"lat\":" << centroid.getY() << ","
       << "\"lon\":" << centroid.getX() << ","
       << "\"attrs\":{";

  char sep = ' ';

  for (const auto& par : group->attrs) {
    json << sep;
    sep = ',';
    json << "\"" << util::jsonStringEscape(par.first) << "\":[";

    char sep2 = ' ';
    for (const auto& val : par.second) {
      json << sep2;
      sep2 = ',';
      json << "\"" << util::jsonStringEscape(val) << "\"";
    }
    json << "]";
  }

  json << "},\"attrerrs\":[";

  sep = ' ';
  for (const auto& err : group->attrErrs) {
    // self errors denote names named as tracks
    if (err.fromRel && err.otherId == group->id && err.attr == err.otherAttr)
      continue;

    json << sep;
    sep = ',';
    json << "{";
    json << "\"attr\":[\"" << err.attr << "\",\""
         << group->attrs.at(err.attr)[0] << "\"]";
    json << ",\"conf\":" << err.conf;

    if (err.fromRel) {
      const auto otherGroup = idx.getGroup(err.otherId);
      json << ",\"other_attr\":[\"" << err.otherAttr << "\",\""
           << otherGroup->attrs.at(err.otherAttr)[0] << "\"]";
      json << ",\"other_grp\":" << err.otherId + range.gidStart;
      json << ",\"other_osmid\":" << otherGroup->osmid;
    } else {
      const auto otherStat = idx.getStation(err.otherId);
      json << ",\"other_attr\":[\"" << err.otherAttr << "\",\""
           << otherStat->attrs.at(err.otherAttr)[0] << "\"]";
      json << ",\"other\":" << err.otherId + range.sidStart;
      json << ",\"other_osmid\":" << otherStat->osmid;
    }
    json << "}";
  }

  json << "]";

  json << ",\"stations\":[";

  sep = ' ';
  for (const auto& sid : group->stations) {
    json << sep;
    sep = ',';
    const auto& stat = idx.getStation(sid);
    json << "{\"id\":" << sid + range.sidStart << ","
         << "\"osmid\":" << stat->osmid << ","
         << "\"group\":" << stat->group + range.gidStart << ","
         << "\"orig_group\":" << stat->origGroup + range.gidStart << ","
         << "\"attrs\":{";

    char sep = ' ';

    for (const auto& par : stat->attrs) {
      json << sep;
      sep = ',';
      json << "\"" << util::jsonStringEscape(par.first) << "\":[";

      char sep2 = ' ';
      for (const auto& val : par.second) {
        json << sep2;
        sep2 = ',';
        json << "\"" << util::jsonStringEscape(val) << "\"";
      }
      json << "]";
    }
    json << "}";
    if (stat->suggestions.size() > 0) json << ",\"s\":1";

    if (stat->attrErrs.size()) json << ",\"e\":" << stat->attrErrs.size();
    json << "}";
  }

  json << "]";

  json << ",\"su\":[";

  std::string seper = "";

  for (size_t sid : group->suggestions) {
    const auto* sugg = idx.getSuggestion(sid);
    if (sugg->type == 6) {
      json << seper << "{\"type\":6,\"attr\":\"" << sugg->attrErrName << "\""
           << "}";
    } else if (sugg->type == 7) {
      json << seper << "{\"type\":7}";
    } else if (sugg->type == 8) {
      json << seper << "{\"type\":8,\"attr\":\"" << sugg->attrErrName << "\""
           << "}";
    } else if (sugg->type == 9) {
      json << seper << "{\"type\":9,\"target_gid\":"
           << (sugg->target_gid + range.gidStart)
           << ",\"target_osm_rel_id\":" << sugg->target_osm_rel_id << "}";
    } else if (sugg->type == 10) {
      json << seper << "{\"type\":10,\"target_gid\":" << sugg->target_gid
           << ",\"target_osm_rel_id\":" << sugg->target_osm_rel_id << "}";
    } else {
      json << seper << "{\"type\":" << sugg->type << "}";
    }
    seper = ',';
  }

  json << "]";

  json << "}";

  if (cb.size()) json << ")";

  auto answ = util::http::Answer("200 OK", json.str(), true);
  answ.params["Content-Type"] = "application/javascript; charset=utf-8";

  return answ;
}

// _____________________________________________________________________________
util::http::Answer StatServer::handleStatReq(const Params& pars) const {
  if (pars.count("id") == 0 || pars.find("id")->second.empty())
    throw std::invalid_argument("No ID specified.");
  std::string cb;
  if (pars.count("cb")) cb = pars.find("cb")->second.c_str();

  size_t sid = atol(pars.find("id")->second.c_str());

  size_t did = getDidBySid(sid);

  const auto& range = _idxs[did].first;
  const auto& idx = _idxs[did].second;
  size_t idxSid = sid - range.sidStart;

  const auto& stat = idx.getStation(idxSid);

  if (!stat) return util::http::Answer("404 Not Found", "Station not found.");

  std::stringstream json;

  json << std::fixed << std::setprecision(6);

  auto centroid = util::geo::centroid(stat->latLng);

  if (cb.size()) json << cb << "(";
  json << "{\"id\":" << sid << ","
       << "\"osmid\":" << stat->osmid << ","
       << "\"lat\":" << centroid.getY() << ","
       << "\"lon\":" << centroid.getX() << ","
       << "\"attrs\":{";

  char sep = ' ';

  for (const auto& par : stat->attrs) {
    json << sep;
    sep = ',';
    json << "\"" << util::jsonStringEscape(par.first) << "\":[";

    char sep2 = ' ';
    for (const auto& val : par.second) {
      json << sep2;
      sep2 = ',';
      json << "\"" << util::jsonStringEscape(val) << "\"";
    }
    json << "]";
  }

  json << "},\"attrerrs\":[";

  sep = ' ';
  for (const auto& err : stat->attrErrs) {
    // self errors denote names named as tracks
    if (!err.fromRel && err.otherId == stat->id && err.attr == err.otherAttr)
      continue;
    json << sep;
    sep = ',';
    json << "{";
    json << "\"attr\":[\"" << err.attr << "\",\"" << stat->attrs.at(err.attr)[0]
         << "\"]";
    json << ",\"conf\":" << err.conf;

    if (err.fromRel) {
      const auto otherGroup = idx.getGroup(err.otherId);
      json << ",\"other_attr\":[\"" << err.otherAttr << "\",\""
           << otherGroup->attrs.at(err.otherAttr)[0] << "\"]";
      json << ",\"other_grp\":" << err.otherId + range.gidStart;
      json << ",\"other_osmid\":" << otherGroup->osmid;
    } else {
      const auto otherStat = idx.getStation(err.otherId);
      json << ",\"other_attr\":[\"" << err.otherAttr << "\",\""
           << otherStat->attrs.at(err.otherAttr)[0] << "\"]";
      json << ",\"other\":" << err.otherId + range.sidStart;
      json << ",\"other_osmid\":" << otherStat->osmid;
    }

    json << "}";
  }

  json << "]";

  json << ",\"su\":[";

  std::string seper = "";

  for (size_t sid : stat->suggestions) {
    const auto* sugg = idx.getSuggestion(sid);
    if (sugg->type == 1) {
      json << seper
           << "{\"type\":1, \"target_gid\":" << stat->group + range.gidStart
           << "}";
    } else if (sugg->type == 2) {
      json << seper
           << "{\"type\":2, \"target_gid\":" << stat->group + range.gidStart
           << ",\"target_osm_rel_id\":" << idx.getGroup(stat->group)->osmid
           << "}";
    } else if (sugg->type == 3) {
      json << seper
           << "{\"type\":3,\"orig_gid\":" << stat->origGroup + range.gidStart
           << ",\"orig_osm_rel_id\":" << idx.getGroup(stat->origGroup)->osmid
           << ",\"target_gid\":" << stat->group + range.gidStart << "}";
    } else if (sugg->type == 4) {
      json << seper
           << "{\"type\":4,\"orig_gid\":" << stat->origGroup + range.gidStart
           << ",\"orig_osm_rel_id\":" << idx.getGroup(stat->origGroup)->osmid
           << ",\"target_gid\":" << stat->group + range.gidStart
           << ",\"target_osm_rel_id\":" << idx.getGroup(stat->group)->osmid
           << "}";
    } else if (sugg->type == 5) {
      json << seper
           << "{\"type\":5,\"orig_gid\":" << stat->origGroup + range.gidStart
           << ",\"orig_osm_rel_id\":" << idx.getGroup(stat->origGroup)->osmid
           << ",\"target_gid\":" << stat->group + range.gidStart
           << ",\"target_osm_rel_id\":" << idx.getGroup(stat->group)->osmid
           << "}";
    } else if (sugg->type == 6) {
      json << seper << "{\"type\":6,\"attr\":\"" << sugg->attrErrName << "\""
           << "}";
    } else if (sugg->type == 7) {
      json << seper << "{\"type\":7}";
    } else if (sugg->type == 8) {
      json << seper << "{\"type\":8,\"attr\":\"" << sugg->attrErrName << "\""
           << "}";
    } else if (sugg->type == 9) {
      json << seper
           << "{\"type\":9,\"orig_gid\":" << stat->origGroup + range.gidStart
           << ",\"orig_osm_rel_id\":" << idx.getGroup(stat->origGroup)->osmid
           << ",\"target_gid\":" << stat->group + range.gidStart
           << ",\"target_osm_rel_id\":" << idx.getGroup(stat->group)->osmid
           << "}";
    } else if (sugg->type == 10) {
      json << seper
           << "{\"type\":10,\"orig_gid\":" << stat->origGroup + range.gidStart
           << ",\"orig_osm_rel_id\":" << idx.getGroup(stat->origGroup)->osmid
           << ",\"target_gid\":" << stat->group + range.gidStart
           << ",\"target_osm_rel_id\":" << idx.getGroup(stat->group)->osmid
           << "}";
    } else {
      json << seper << "{\"type\":" << sugg->type << "}";
    }
    seper = ',';
  }

  json << "]";

  json << "}";

  if (cb.size()) json << ")";

  auto answ = util::http::Answer("200 OK", json.str(), true);
  answ.params["Content-Type"] = "application/javascript; charset=utf-8";

  return answ;
}
