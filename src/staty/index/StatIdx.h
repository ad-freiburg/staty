// Copyright 2019, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Authors: Patrick Brosi <brosi@informatik.uni-freiburg.de>

#ifndef staty_INDEX_STATIDX_H_
#define staty_INDEX_STATIDX_H_

#include <map>
#include <string>
#include <vector>
#include "util/geo/Geo.h"
#include "util/geo/Grid.h"

namespace staty {

typedef std::map<std::string, std::vector<std::string>> OsmAttrs;

inline bool byAttrScore(const std::pair<int, std::string>& lh,
                        const std::pair<int, std::string>& rh) {
  return lh.first > rh.first;
}

struct IdRange {
  size_t sidStart, sidEnd, gidStart, gidEnd, suggIdStart, suggIdEnd;
};

struct AttrErr {
  std::string attr;
  std::string otherAttr;
  double conf;
  size_t otherId;
  bool fromRel;
};

struct Suggestion {
  uint16_t type;
  int64_t station;
  // if negative, it is a way!
  int64_t target_gid, orig_gid, target_osm_rel_id, orig_osm_rel_id;
  std::string attrErrName;
  util::geo::DLine arrow;
};

struct MetaGroup {
  size_t osmid;
  util::geo::DPolygon poly;
  util::geo::DPolygon llPoly;
  util::geo::DPoint centroid;
  std::vector<size_t> groups;
};

struct Station {
  size_t id;
  // if negative, it is a way!
  int64_t osmid;
  size_t origGroup, group;
  util::geo::DPoint centroid;
  util::geo::DLine pos;
  util::geo::DLine latLng;
  OsmAttrs attrs;
  std::vector<AttrErr> attrErrs;
  std::vector<size_t> suggestions;
};

inline bool byErr(const Station* lh, const Station* rh) {
  size_t a = lh->attrErrs.size() ? 2 : lh->suggestions.size() ? 1 : 0;
  size_t b = rh->attrErrs.size() ? 2 : rh->suggestions.size() ? 1 : 0;
  return a < b;
}

struct ByErrId {
  ByErrId(const std::vector<Station>& stats) : stats(stats) {}
  bool operator()(int64_t lhId, int64_t rhId) {
    const auto& lh = stats[lhId];
    const auto& rh = stats[rhId];
    size_t a = lh.attrErrs.size() ? 2 : lh.suggestions.size() ? 1 : 0;
    size_t b = rh.attrErrs.size() ? 2 : rh.suggestions.size() ? 1 : 0;
    return a > b;
  }
  const std::vector<Station>& stats;
};

struct Group {
  size_t id;
  int64_t osmid;
  size_t mergeId;
  size_t metaGroupId;
  util::geo::DPolygon poly;
  util::geo::DPolygon llPoly;
  util::geo::DPoint centroid;
  OsmAttrs attrs;
  std::vector<size_t> stations;
  std::vector<size_t> polyStations;
  std::vector<AttrErr> attrErrs;
  std::vector<size_t> suggestions;
  std::vector<std::pair<int, std::string>> uniqueNames;
};

struct Cluster {
  util::geo::DPoint pos;
  util::geo::DPoint latLng;
  size_t size;
  bool operator<(const Cluster& other) const { return this < &other; }
};

class StatIdx {
 public:
  StatIdx()
      : _numOSMStations(0),
        _numOSMGroups(0),
        _numOSMMetaGroups(0),
        _numOK(0),
        _numDL(0),
        _numMV(0),
        _numGR(0),
        _numCR(0),
        _numER(0),
        _numMG(0),
        _numNM(0),
        _numTR(0) {}

  void readFromFile(const std::string& path);

  size_t maxSId() const { return _stations.size() - 1; };
  size_t maxGId() const { return _groups.size() - 1; };
  size_t maxSuggId() const { return _suggestions.size() - 1; };

  std::vector<const Station*> getStations(const util::geo::DBox bbox) const;
  std::vector<const Group*> getGroups(const util::geo::DBox bbox) const;
  std::vector<const Suggestion*> getSuggestions(
      const util::geo::DBox bbox) const;
  std::vector<const MetaGroup*> getMetaGroups(const util::geo::DBox bbox) const;

  const Station* getStation(size_t id) const;
  const Group* getGroup(size_t id) const;
  const Suggestion* getSuggestion(size_t id) const;
  const MetaGroup* getMetaGroup(size_t id) const;

  std::vector<Cluster> getHeatGridOk(const util::geo::DBox bbox,
                                     size_t z) const;

  std::vector<Cluster> getHeatGridSugg(const util::geo::DBox bbox,
                                       size_t z) const;

  std::vector<Cluster> getHeatGridErr(const util::geo::DBox bbox,
                                      size_t z) const;

  const std::vector<Station>& getStations() const { return _stations; }
  const std::vector<Group>& getGroups() const { return _groups; }

 private:
  void addStation(int64_t osmid, const util::geo::DLine& geom,
                  const util::geo::DLine& latLngs, size_t origGroup,
                  size_t group, const OsmAttrs& attrs);
  void addGroup(size_t id, const OsmAttrs& attrs);
  void addGroupToMeta(size_t gid, size_t metaId);

  void initIndex();
  void initGroups();
  void initMetaGroups();
  void initSuggestions();

  int nameAttrRel(const std::string& attr) const;

  util::geo::DLine getGroupArrow(const util::geo::DPoint& a,
                                 const util::geo::DPoint& p) const;

  util::geo::Polygon<double> hull(const util::geo::MultiPoint<double>& imp,
                                  double rad) const;

  std::vector<Station> _stations;
  std::vector<Group> _groups;
  std::vector<Suggestion> _suggestions;
  std::map<size_t, MetaGroup> _metaGroups;
  util::geo::DBox _bbox;

  util::geo::Grid<size_t, util::geo::Polygon, double> _sgrid;
  util::geo::Grid<size_t, util::geo::Polygon, double> _ggrid;
  util::geo::Grid<size_t, util::geo::Polygon, double> _mgrid;
  util::geo::Grid<size_t, util::geo::Line, double> _suggrid;

  std::vector<util::geo::Grid<Cluster, util::geo::Point, double>> _heatGridsOk;
  std::vector<util::geo::Grid<Cluster, util::geo::Point, double>>
      _heatGridsSugg;
  std::vector<util::geo::Grid<Cluster, util::geo::Point, double>> _heatGridsErr;

  size_t _numOSMStations;
  size_t _numOSMGroups;
  size_t _numOSMMetaGroups;
  size_t _numOK;
  size_t _numDL;
  size_t _numMV;
  size_t _numGR;
  size_t _numCR;
  size_t _numER;

  size_t _numMG;

  size_t _numNM;
  size_t _numTR;
};
}  // namespace staty

#endif  // staty_INDEX_STATIDX_H_
