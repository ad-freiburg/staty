// Copyright 2019, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Authors: Patrick Brosi <brosi@informatik.uni-freiburg.de>

#ifndef staty_SERVER_STATSERVER_H_
#define staty_SERVER_STATSERVER_H_

#include <map>
#include <string>
#include "staty/index/SearchIdx.h"
#include "staty/index/StatIdx.h"
#include "util/http/Server.h"

namespace staty {

typedef std::map<std::string, std::string> Params;

class StatServer : public util::http::Handler {
 public:
  explicit StatServer(
      const std::vector<std::pair<IdRange, staty::StatIdx>>& idxs,
      const SearchIdx& searchIdx)
      : _idxs(idxs), _searchIdx(searchIdx) {}

  virtual util::http::Answer handle(const util::http::Req& request,
                                    int connection) const;

 private:
  static std::string parseUrl(std::string u, std::string pl, Params* params);

  util::http::Answer handleMapReq(const Params& pars) const;
  util::http::Answer handleHeatMapReq(const Params& pars) const;
  util::http::Answer handleStatReq(const Params& pars) const;
  util::http::Answer handleGroupReq(const Params& pars) const;
  util::http::Answer handleMGroupReq(const Params& pars) const;
  util::http::Answer handleSearch(const Params& pars) const;

  void printStation(const Station* stat, size_t did, bool simple,
                    std::ostream* out) const;
  void printGroup(const Group* stat, size_t did, bool simple, bool aggr,
                  std::ostream* out) const;
  void printSugg(const Suggestion* stat, size_t did, std::ostream* out) const;
  void printMetaGroup(const MetaGroup* stat, size_t did, bool simple,
                      std::ostream* out) const;

  size_t getDidBySid(size_t sid) const;
  size_t getDidBySuggid(size_t suggid) const;
  size_t getDidByGid(size_t gid) const;
  size_t getDidByMGid(size_t mgid) const;

  const std::vector<std::pair<IdRange, staty::StatIdx>>& _idxs;
  const SearchIdx& _searchIdx;
};
}  // namespace staty

#endif  // staty_SERVER_STATSERVER_H_
