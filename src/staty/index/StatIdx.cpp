// Copyright 2019, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Authors: Patrick Brosi <brosi@informatik.uni-freiburg.de>

#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include "staty/index/StatIdx.h"
#include "util/geo/BezierCurve.h"
#include "util/log/Log.h"

using staty::Group;
using staty::MetaGroup;
using staty::OsmAttrs;
using staty::StatIdx;
using staty::Station;
using staty::Suggestion;

// _____________________________________________________________________________
void StatIdx::readFromFile(const std::string& path) {
  std::ifstream f(path);

  std::string line;

  // first, parse nodes
  LOG(INFO) << "Parsing nodes...";
  while (std::getline(f, line)) {
    // empty line is separator between blocks
    if (line.size() == 0) break;

    std::stringstream rec(line);
    int64_t osmid, origGroup, group;
    double lat = 0, lng = 0;
    OsmAttrs attrs;

    rec >> osmid;
    rec >> origGroup;
    rec >> group;

    util::geo::DLine geom;
    util::geo::DLine latLngs;

    std::string temp;
    std::string key, val;
    uint8_t sw = 0;
    std::getline(rec, temp, '\t');
    while (std::getline(rec, temp, '\t')) {
      if (sw == 2) {
        std::getline(rec, val, '\t');
        attrs[temp].push_back(val);
      } else {
        try {
          double a = std::stod(temp);
          if (sw == 1) {
            lat = a;
            geom.push_back(util::geo::latLngToWebMerc<double>(lat, lng));
            sw = 0;
          } else if (sw == 0) {
            lng = a;
            sw = 1;
          }
        } catch (std::invalid_argument&) {
          sw = 2;
          std::getline(rec, val, '\t');
          attrs[temp].push_back(val);
        }
      }
    }

    if (geom.size() != 1 && util::geo::dist(geom.front(), geom.back()) > 1) {
      // transform lines into polygons
      geom = hull(geom, 2).getOuter();
    }

    if (geom.size() > 1) geom = util::geo::simplify(geom, 0.5);
    for (const auto& p : geom)
      latLngs.push_back(util::geo::webMercToLatLng<double>(p.getX(), p.getY()));

    addStation(osmid, geom, latLngs, origGroup, group, attrs);
  }
  LOG(INFO) << "Done.";

  // second, parse groups
  LOG(INFO) << "Parsing groups... ";
  while (std::getline(f, line)) {
    // empty line is separator between blocks
    if (line.size() == 0) break;

    std::stringstream rec(line);
    int64_t osmid;
    OsmAttrs attrs;

    rec >> osmid;

    std::string key;
    std::getline(rec, key, '\t');
    while (std::getline(rec, key, '\t')) {
      std::string val;
      std::getline(rec, val, '\t');
      attrs[key].push_back(val);
    }

    addGroup(osmid, attrs);
  }

  LOG(INFO) << "Done.";

  // third, parse attr errs
  LOG(INFO) << "Parsing station errors... ";
  while (std::getline(f, line)) {
    // empty line is separator between blocks
    if (line.size() == 0) break;

    std::stringstream rec(line);
    size_t id;
    std::string attr, otherAttr;
    double conf;
    int64_t otherIdRaw;

    rec >> id;

    std::getline(rec, attr, '\t');
    std::getline(rec, attr, '\t');
    std::getline(rec, otherAttr, '\t');

    rec >> conf;
    rec >> otherIdRaw;

    size_t otherId;
    bool fromRel = false;

    if (otherIdRaw < 0) fromRel = true;

    otherId = labs(otherIdRaw);

    _stations[id].attrErrs.push_back({attr, otherAttr, conf, otherId, fromRel});
  }
  LOG(INFO) << "Done.";

  // fourth, parse attr errs in groups
  LOG(INFO) << "Parsing group errors... ";
  while (std::getline(f, line)) {
    // empty line is separator between blocks
    if (line.size() == 0) break;

    std::stringstream rec(line);
    size_t id;
    std::string attr, otherAttr;
    double conf;
    int64_t otherIdRaw;

    rec >> id;

    std::getline(rec, attr, '\t');
    std::getline(rec, attr, '\t');
    std::getline(rec, otherAttr, '\t');

    rec >> conf;
    rec >> otherIdRaw;

    size_t otherId;
    bool fromRel = false;

    if (otherIdRaw < 0) fromRel = true;

    otherId = labs(otherIdRaw);

    _groups[id].attrErrs.push_back({attr, otherAttr, conf, otherId, fromRel});
  }
  LOG(INFO) << "Done.";

  // fifth, parse meta groups
  LOG(INFO) << "Parsing meta groups... ";
  while (std::getline(f, line)) {
    // empty line is separator between blocks
    if (line.size() == 0) break;

    std::stringstream rec(line);
    size_t gid, osmid;

    rec >> gid;
    rec >> osmid;

    addGroupToMeta(gid, osmid);
  }

  _numOSMMetaGroups = _metaGroups.size();

  LOG(INFO) << "Done.";

  LOG(INFO) << "Init groups";
  initGroups();
  LOG(INFO) << "Done.";
  LOG(INFO) << "Init meta groups";
  initMetaGroups();
  LOG(INFO) << "Done.";
  LOG(INFO) << "Init suggestions";
  initSuggestions();
  LOG(INFO) << "Done.";

  LOG(INFO) << "Building indices...";
  initIndex();
  LOG(INFO) << "Done.";

  LOG(INFO) << "Input Stats:";
  LOG(INFO) << _numOSMStations << " OSM stations";
  LOG(INFO) << _numOSMGroups << " OSM groups";
  LOG(INFO) << _numOSMMetaGroups << " OSM meta groups";

  LOG(INFO) << "Suggestion Stats:";
  LOG(INFO) << _numOK << " objects OK";
  LOG(INFO) << _numDL << " stations should be removed from their group (DL)";
  LOG(INFO) << _numMV << " stations should be moved into another group (MV)";
  LOG(INFO) << _numGR << " stations should be moved into a group (GR)";
  LOG(INFO) << _numCR
            << " stations should be moved into newly created groups (CR)";
  LOG(INFO) << _numER << " tags were marked as erroneous (ER)";
  LOG(INFO) << _numMG << " relations should be merged (MG)";
  LOG(INFO) << _numNM << " objects should be outfitted with a name (NM)";
  LOG(INFO) << _numTR << " station names are most likely track numbers";
}

// _____________________________________________________________________________
void StatIdx::addGroupToMeta(size_t gid, size_t metaId) {
  _groups[gid].metaGroupId = metaId;
  _metaGroups[metaId].osmid = metaId;
  _metaGroups[metaId].groups.push_back(gid);
}

// _____________________________________________________________________________
void StatIdx::addStation(int64_t osmid, const util::geo::DLine& geom,
                         const util::geo::DLine& latLngs, size_t origGroup,
                         size_t group, const OsmAttrs& attrs) {
  _numOSMStations++;
  auto centroid = util::geo::centroid(latLngs);
  _stations.emplace_back(Station{_stations.size(),
                                 osmid,
                                 origGroup,
                                 group,
                                 centroid,
                                 geom,
                                 latLngs,
                                 attrs,
                                 {},
                                 {}});

  // extend bounding box
  for (const auto& point : geom) _bbox = util::geo::extendBox(point, _bbox);
}

// _____________________________________________________________________________
void StatIdx::addGroup(size_t osmid, const OsmAttrs& attrs) {
  if (osmid > 2) _numOSMGroups++;
  Group g;
  g.id = _groups.size();
  g.osmid = osmid;
  g.metaGroupId = 0;
  g.mergeId = 0;
  g.attrs = attrs;
  _groups.emplace_back(g);
}

// _____________________________________________________________________________
void StatIdx::initMetaGroups() {
  // build hull polygons
  for (auto& p : _metaGroups) {
    auto& mg = p.second;
    util::geo::MultiPoint<double> mp;
    for (size_t gid : mg.groups) {
      if (_groups[gid].polyStations.size() == 0) continue;
      for (const auto& p : _groups[gid].poly.getOuter()) mp.push_back(p);
    }
    mg.poly = hull(mp, 11);
    assert(mg.poly.getOuter().size());

    for (auto p : mg.poly.getOuter()) {
      mg.llPoly.getOuter().push_back(
          util::geo::webMercToLatLng<double>(p.getX(), p.getY()));
    }

    mg.centroid = util::geo::centroid(mg.llPoly);
  }
}

// _____________________________________________________________________________
void StatIdx::initGroups() {
  for (size_t i = 0; i < _stations.size(); i++) {
    // this should be ensured by the input file
    assert(_stations[i].origGroup < _groups.size());
    _groups[_stations[i].origGroup].polyStations.push_back(i);

    _groups[_stations[i].group].stations.push_back(i);

    if (_stations[i].group != _stations[i].origGroup) {
      _groups[_stations[i].origGroup].stations.push_back(i);
    }

    // if all stations of a relation are moved into another same relation,
    // suggest to merge the relation instead of suggesting each invidivual
    // move
    if (_groups[_stations[i].group].osmid > 2 &&
        _groups[_stations[i].origGroup].osmid > 2) {
      auto& orGr = _groups[_stations[i].origGroup];
      if (orGr.mergeId == 0) {
        // if no merge group has been yet written, set it to the target group
        // of the currently check station
        orGr.mergeId = _stations[i].group;
      } else if (orGr.mergeId != _stations[i].group) {
        // if a merge group was set, but the current station is not moved into
        // this group, set the merge id to the group id. This is later inter-
        // preted as DO NOT MERGE
        orGr.mergeId = orGr.id;
      }
    }

    assert(_stations[i].group < _groups.size());
    if (_stations[i].group != _stations[i].origGroup &&
        _groups[_stations[i].group].osmid == 1) {
      // only add non-orig stations to polygons of new (non-osm) groups
      _groups[_stations[i].group].polyStations.push_back(i);
    }
  }

  // collect unique names
  for (size_t gid = 0; gid < _groups.size(); gid++) {
    std::set<std::string> usedGroupNames;

    for (const auto& np : _groups[gid].attrs) {
      for (const auto& name : np.second) {
        if (usedGroupNames.count(name)) continue;
        usedGroupNames.insert(name);

        // relation names are preferred
        _groups[gid].uniqueNames.push_back({nameAttrRel(np.first) + 3, name});
      }
    }

    for (size_t sid : _groups[gid].stations) {
      for (const auto& np : _stations[sid].attrs) {
        for (const auto& name : np.second) {
          // use every name only once to keep the index small
          if (usedGroupNames.count(name)) continue;
          usedGroupNames.insert(name);

          _groups[gid].uniqueNames.push_back({nameAttrRel(np.first), name});
        }
      }
    }

    // take the best fitting name as the identifier
    std::sort(_groups[gid].uniqueNames.begin(), _groups[gid].uniqueNames.end(),
              byAttrScore);
  }

  ByErrId byErrId(_stations);

  // sort stations by error or sugg occurance
  for (size_t gid = 0; gid < _groups.size(); gid++) {
    std::sort(_groups[gid].stations.begin(), _groups[gid].stations.end(),
              byErrId);
  }

  // build hull polygon

  for (size_t i = 0; i < _groups.size(); i++) {
    util::geo::MultiPoint<double> mp;
    for (size_t stid : _groups[i].polyStations) {
      for (const auto& geom : _stations[stid].pos) {
        mp.push_back(util::geo::DPoint(geom.getX(), geom.getY()));
      }
    }

    _groups[i].poly = hull(mp, 11);

    for (auto p : _groups[i].poly.getOuter()) {
      _groups[i].llPoly.getOuter().push_back(
          util::geo::webMercToLatLng<double>(p.getX(), p.getY()));
    }

    _groups[i].centroid = util::geo::centroid(_groups[i].llPoly);
  }
}

// _____________________________________________________________________________
util::geo::Polygon<double> StatIdx::hull(
    const util::geo::MultiPoint<double>& imp, double rad) const {
  util::geo::MultiPoint<double> mp;
  util::geo::Polygon<double> ret;
  for (const auto& geom : imp) {
    int n = 20;
    for (int i = 0; i < n; i++) {
      double x = rad * cos((2.0 * M_PI / static_cast<double>(n)) *
                           static_cast<double>(i));
      double y = rad * sin((2.0 * M_PI / static_cast<double>(n)) *
                           static_cast<double>(i));

      mp.push_back(util::geo::DPoint(geom.getX() + x, geom.getY() + y));
    }
  }
  ret = util::geo::simplify(util::geo::convexHull(mp).getOuter(), 0.5);

  return ret;
}

// _____________________________________________________________________________
void StatIdx::initIndex() {
  double gSize = 10000;
  _sgrid = util::geo::Grid<size_t, util::geo::Polygon, double>(gSize, gSize,
                                                               _bbox, false);
  _ggrid = util::geo::Grid<size_t, util::geo::Polygon, double>(gSize, gSize,
                                                               _bbox, false);
  _mgrid = util::geo::Grid<size_t, util::geo::Polygon, double>(gSize, gSize,
                                                               _bbox, false);
  _suggrid = util::geo::Grid<size_t, util::geo::Line, double>(gSize, gSize,
                                                              _bbox, false);

  for (size_t i = 0; i < _stations.size(); i++) _sgrid.add(_stations[i].pos, i);
  for (size_t i = 0; i < _groups.size(); i++) {
    if (_groups[i].polyStations.size() == 1 && _groups[i].osmid < 2) continue;
    _ggrid.add(_groups[i].poly, i);
  }
  for (const auto& gp : _metaGroups) {
    if (gp.second.poly.getOuter().size() < 2) continue;
    _mgrid.add(gp.second.poly, gp.first);
  }
  for (size_t i = 0; i < _suggestions.size(); i++)
    _suggrid.add(_suggestions[i].arrow, i);

  _heatGridsOk.resize(10);
  _heatGridsSugg.resize(10);
  _heatGridsErr.resize(10);

  for (size_t i = 0; i < 10; i++) {
    _heatGridsOk[i] =
        (util::geo::Grid<staty::Cluster, util::geo::Point, double>(
            5000, 5000, _bbox, false));
    _heatGridsSugg[i] =
        (util::geo::Grid<staty::Cluster, util::geo::Point, double>(
            5000, 5000, _bbox, false));
    _heatGridsErr[i] =
        (util::geo::Grid<staty::Cluster, util::geo::Point, double>(
            5000, 5000, _bbox, false));
    int step = 2000 * (i + 1);

    for (size_t x = 1; x * step < (_sgrid.getXWidth() * gSize); x++) {
      for (size_t y = 1; y * step < (_sgrid.getYHeight() * gSize); y++) {
        std::set<size_t> tmp;
        auto reqBox =
            util::geo::DBox({_bbox.getLowerLeft().getX() + (x - 1) * step,
                             _bbox.getLowerLeft().getY() + (y - 1) * step},
                            {_bbox.getLowerLeft().getX() + x * step,
                             _bbox.getLowerLeft().getY() + y * step});
        _sgrid.get(reqBox, &tmp);

        size_t countOk = 0;
        double avgXOk = 0;
        double avgYOk = 0;

        size_t countSugg = 0;
        double avgXSugg = 0;
        double avgYSugg = 0;

        size_t countErr = 0;
        double avgXErr = 0;
        double avgYErr = 0;

        for (auto j : tmp) {
          const auto& stat = _stations[j];
          if (stat.suggestions.size() == 0 && stat.attrErrs.size() == 0) {
            for (const auto& pos : stat.pos) {
              if (util::geo::contains(pos, reqBox)) {
                countOk++;
                avgXOk += pos.getX();
                avgYOk += pos.getY();
              }
            }
          } else if (stat.suggestions.size() > 0 && stat.attrErrs.size() == 0) {
            for (const auto& pos : stat.pos) {
              if (util::geo::contains(pos, reqBox)) {
                countSugg++;
                avgXSugg += pos.getX();
                avgYSugg += pos.getY();
              }
            }
          } else if (stat.attrErrs.size() > 0) {
            for (const auto& pos : stat.pos) {
              if (util::geo::contains(pos, reqBox)) {
                countErr++;
                avgXErr += pos.getX();
                avgYErr += pos.getY();
              }
            }
          }
        }

        if (countOk != 0) {
          avgXOk /= countOk;
          avgYOk /= countOk;
          auto ll = util::geo::webMercToLatLng<double>(avgXOk, avgYOk);

          _heatGridsOk[i].add({avgXOk, avgYOk},
                              Cluster{{avgXOk, avgYOk}, ll, countOk});
        }

        if (countSugg != 0) {
          avgXSugg /= countSugg;
          avgYSugg /= countSugg;
          auto ll = util::geo::webMercToLatLng<double>(avgXSugg, avgYSugg);

          _heatGridsSugg[i].add({avgXSugg, avgYSugg},
                                Cluster{{avgXSugg, avgYSugg}, ll, countSugg});
        }

        if (countErr != 0) {
          avgXErr /= countErr;
          avgYErr /= countErr;
          auto ll = util::geo::webMercToLatLng<double>(avgXErr, avgYErr);

          _heatGridsErr[i].add({avgXErr, avgYErr},
                               Cluster{{avgXErr, avgYErr}, ll, countErr});
        }
      }
    }
  }
}

// _____________________________________________________________________________
std::vector<const MetaGroup*> StatIdx::getMetaGroups(
    const util::geo::DBox bbox) const {
  std::vector<const MetaGroup*> ret;
  auto ll = util::geo::latLngToWebMerc<double>(bbox.getLowerLeft().getX(),
                                               bbox.getLowerLeft().getY());
  auto ur = util::geo::latLngToWebMerc<double>(bbox.getUpperRight().getX(),
                                               bbox.getUpperRight().getY());

  std::set<size_t> tmp;
  auto reqBox = util::geo::DBox(ll, ur);
  _mgrid.get(reqBox, &tmp);

  for (auto i : tmp) {
    if (util::geo::intersects(_metaGroups.find(i)->second.poly, reqBox))
      ret.push_back(&_metaGroups.find(i)->second);
  }

  return ret;
}

// _____________________________________________________________________________
std::vector<const Suggestion*> StatIdx::getSuggestions(
    const util::geo::DBox bbox) const {
  std::vector<const Suggestion*> ret;
  auto ll = util::geo::latLngToWebMerc<double>(bbox.getLowerLeft().getX(),
                                               bbox.getLowerLeft().getY());
  auto ur = util::geo::latLngToWebMerc<double>(bbox.getUpperRight().getX(),
                                               bbox.getUpperRight().getY());

  std::set<size_t> tmp;
  auto reqBox = util::geo::DBox(ll, ur);
  _suggrid.get(reqBox, &tmp);

  for (auto i : tmp) {
    if (util::geo::intersects(_suggestions[i].arrow, reqBox))
      ret.push_back(&_suggestions[i]);
  }

  return ret;
}

// _____________________________________________________________________________
std::vector<const Group*> StatIdx::getGroups(const util::geo::DBox bbox) const {
  std::vector<const Group*> ret;
  auto ll = util::geo::latLngToWebMerc<double>(bbox.getLowerLeft().getX(),
                                               bbox.getLowerLeft().getY());
  auto ur = util::geo::latLngToWebMerc<double>(bbox.getUpperRight().getX(),
                                               bbox.getUpperRight().getY());

  std::set<size_t> tmp;
  auto reqBox = util::geo::DBox(ll, ur);
  _ggrid.get(reqBox, &tmp);

  for (auto i : tmp) {
    if (util::geo::intersects(_groups[i].poly, reqBox))
      ret.push_back(&_groups[i]);
  }

  return ret;
}

// _____________________________________________________________________________
std::vector<const Station*> StatIdx::getStations(
    const util::geo::DBox bbox) const {
  std::vector<const Station*> ret;
  auto ll = util::geo::latLngToWebMerc<double>(bbox.getLowerLeft().getX(),
                                               bbox.getLowerLeft().getY());
  auto ur = util::geo::latLngToWebMerc<double>(bbox.getUpperRight().getX(),
                                               bbox.getUpperRight().getY());

  std::set<size_t> tmp;
  auto reqBox = util::geo::DBox(ll, ur);
  _sgrid.get(reqBox, &tmp);

  for (auto i : tmp) {
    if (util::geo::intersects(util::geo::Polygon<double>(_stations[i].pos),
                              reqBox)) {
      ret.push_back(&_stations[i]);
    }
  }

  std::sort(ret.begin(), ret.end(), byErr);

  return ret;
}

// _____________________________________________________________________________
const Station* StatIdx::getStation(size_t id) const {
  if (id >= _stations.size()) return 0;
  return &_stations[id];
}

// _____________________________________________________________________________
std::vector<staty::Cluster> StatIdx::getHeatGridErr(const util::geo::DBox bbox,
                                                    size_t z) const {
  auto ll = util::geo::latLngToWebMerc<double>(bbox.getLowerLeft().getX(),
                                               bbox.getLowerLeft().getY());
  auto ur = util::geo::latLngToWebMerc<double>(bbox.getUpperRight().getX(),
                                               bbox.getUpperRight().getY());

  auto reqBox = util::geo::DBox(ll, ur);

  z = fmin(9, z);
  z = fmax(0, z);

  const auto& grid = _heatGridsErr[9 - z];
  std::vector<staty::Cluster> ret;

  std::set<staty::Cluster> tmp;
  grid.get(reqBox, &tmp);

  for (const auto& i : tmp) {
    if (util::geo::contains(i.pos, reqBox)) ret.push_back(i);
  }

  return ret;
}

// _____________________________________________________________________________
std::vector<staty::Cluster> StatIdx::getHeatGridOk(const util::geo::DBox bbox,
                                                   size_t z) const {
  auto ll = util::geo::latLngToWebMerc<double>(bbox.getLowerLeft().getX(),
                                               bbox.getLowerLeft().getY());
  auto ur = util::geo::latLngToWebMerc<double>(bbox.getUpperRight().getX(),
                                               bbox.getUpperRight().getY());

  auto reqBox = util::geo::DBox(ll, ur);

  z = fmin(9, z);
  z = fmax(0, z);

  const auto& grid = _heatGridsOk[9 - z];
  std::vector<staty::Cluster> ret;

  std::set<staty::Cluster> tmp;
  grid.get(reqBox, &tmp);

  for (const auto& i : tmp) {
    if (util::geo::contains(i.pos, reqBox)) ret.push_back(i);
  }

  return ret;
}

// _____________________________________________________________________________
std::vector<staty::Cluster> StatIdx::getHeatGridSugg(const util::geo::DBox bbox,
                                                     size_t z) const {
  auto ll = util::geo::latLngToWebMerc<double>(bbox.getLowerLeft().getX(),
                                               bbox.getLowerLeft().getY());
  auto ur = util::geo::latLngToWebMerc<double>(bbox.getUpperRight().getX(),
                                               bbox.getUpperRight().getY());

  auto reqBox = util::geo::DBox(ll, ur);

  z = fmin(9, z);
  z = fmax(0, z);

  const auto& grid = _heatGridsSugg[9 - z];
  std::vector<staty::Cluster> ret;

  std::set<staty::Cluster> tmp;
  grid.get(reqBox, &tmp);

  for (const auto& i : tmp) {
    if (util::geo::contains(i.pos, reqBox)) ret.push_back(i);
  }

  return ret;
}
// _____________________________________________________________________________
void StatIdx::initSuggestions() {
  // group suggestions
  for (size_t i = 0; i < _groups.size(); i++) {
    auto& group = _groups[i];

    if (group.attrs.count("name") == 0) {
      Suggestion sug;
      sug.station = i;
      sug.type = 7;
      _suggestions.push_back(sug);
      group.suggestions.push_back(_suggestions.size() - 1);

      if (group.osmid > 2) _numNM++;
    }

    Suggestion sug;
    sug.station = i;

    if (group.mergeId != 0 && group.mergeId != group.id) {
      if (getGroup(group.mergeId)->metaGroupId) {
        // move group into meta group
        sug.type = 10;
        sug.station = -group.id;
        sug.orig_gid = 0;
        sug.orig_osm_rel_id = 0;
        sug.target_gid = getGroup(group.mergeId)->metaGroupId;
        sug.target_osm_rel_id = getGroup(group.mergeId)->metaGroupId;

        auto a = util::geo::centroid(group.poly);
        auto b = util::geo::centroid(
            getMetaGroup(getGroup(group.mergeId)->metaGroupId)->poly);
        sug.arrow = getGroupArrow(a, b);

        _numMG++;
      } else {
        // merge two groups
        sug.type = 9;
        sug.station = -group.id;
        sug.orig_gid = 0;
        sug.orig_osm_rel_id = 0;
        sug.target_gid = group.mergeId;
        sug.target_osm_rel_id = getGroup(group.mergeId)->osmid;
        auto a = util::geo::centroid(group.poly);
        auto b = util::geo::centroid(getGroup(group.mergeId)->poly);
        sug.arrow = getGroupArrow(a, b);

        _numMG++;
      }

      _suggestions.push_back(sug);
      group.suggestions.push_back(_suggestions.size() - 1);
    }

    std::set<std::pair<std::string, uint16_t>> suggested;
    for (auto attrErr : group.attrErrs) {
      if (!attrErr.fromRel) continue;

      const auto otherGrp = getGroup(attrErr.otherId);
      if (group.id == attrErr.otherId && attrErr.attr == attrErr.otherAttr) {
        if (suggested.count({attrErr.attr, 8})) continue;
        // fix track number as name
        suggested.insert({attrErr.attr, 8});
        sug.type = 8;
        sug.orig_gid = 0;
        sug.orig_osm_rel_id = 0;
        sug.target_gid = 0;
        sug.target_osm_rel_id = 0;
        sug.attrErrName = attrErr.attr;

        _suggestions.push_back(sug);
        group.suggestions.push_back(_suggestions.size() - 1);

        _numTR++;
        _numER++;
      } else if (otherGrp->osmid == group.osmid) {
        if (suggested.count({attrErr.attr, 6})) continue;
        // fix attributes
        suggested.insert({attrErr.attr, 6});
        sug.type = 6;
        sug.orig_gid = 0;
        sug.orig_osm_rel_id = 0;
        sug.target_gid = 0;
        sug.target_osm_rel_id = 0;
        sug.attrErrName = attrErr.attr;

        _suggestions.push_back(sug);
        group.suggestions.push_back(_suggestions.size() - 1);

        _numER++;
      }
    }

    if (group.suggestions.size() + group.attrErrs.size() == 0) _numOK++;
  }

  // station suggestions
  for (size_t i = 0; i < _stations.size(); i++) {
    auto& stat = _stations[i];
    auto centroid = util::geo::centroid(stat.pos);

    if (stat.attrs.count("name") == 0 &&
        _groups[stat.group].attrs.count("name") == 0) {
      Suggestion sug;
      sug.station = i;
      sug.type = 7;
      _suggestions.push_back(sug);
      stat.suggestions.push_back(_suggestions.size() - 1);

      _numNM++;
    }

    Suggestion sug;
    sug.station = i;

    bool hasStationTrackHeur = false;

    std::set<std::pair<std::string, uint16_t>> suggested;
    for (auto attrErr : stat.attrErrs) {
      if (attrErr.fromRel) continue;

      const auto otherStat = getStation(attrErr.otherId);
      if (stat.id == attrErr.otherId && attrErr.attr == attrErr.otherAttr) {
        if (suggested.count({attrErr.attr, 8})) continue;
        hasStationTrackHeur = true;
        suggested.insert({attrErr.attr, 8});
        // fix track number as name
        sug.type = 8;
        sug.orig_gid = 0;
        sug.orig_osm_rel_id = 0;
        sug.target_gid = 0;
        sug.target_osm_rel_id = 0;
        sug.attrErrName = attrErr.attr;

        _suggestions.push_back(sug);
        stat.suggestions.push_back(_suggestions.size() - 1);

        _numTR++;
        _numER++;
      } else if (otherStat->osmid == stat.osmid) {
        if (suggested.count({attrErr.attr, 6})) continue;
        suggested.insert({attrErr.attr, 6});
        // fix attributes
        sug.type = 6;
        sug.orig_gid = 0;
        sug.orig_osm_rel_id = 0;
        sug.target_gid = 0;
        sug.target_osm_rel_id = 0;
        sug.attrErrName = attrErr.attr;

        _suggestions.push_back(sug);
        stat.suggestions.push_back(_suggestions.size() - 1);

        _numER++;
      }
    }

    if (stat.group != stat.origGroup || getGroup(stat.group)->osmid == 1) {
      Suggestion sug;
      sug.station = i;

      if (getGroup(stat.origGroup)->osmid < 2) {
        if (getGroup(stat.group)->osmid == 1 &&
            getGroup(stat.group)->stations.size() > 1) {
          // move orphan into new group
          sug.type = 1;
          sug.target_gid = stat.group;

          _suggestions.push_back(sug);
          stat.suggestions.push_back(_suggestions.size() - 1);

          _numCR++;
        } else if (getGroup(stat.group)->osmid > 1) {
          // move orphan into existing group
          sug.type = 2;
          sug.target_gid = stat.group;
          sug.target_osm_rel_id = getGroup(stat.group)->osmid;

          auto b = util::geo::centroid(getGroup(stat.group)->poly);
          sug.arrow = getGroupArrow(centroid, b);

          _suggestions.push_back(sug);
          stat.suggestions.push_back(_suggestions.size() - 1);

          _numGR++;
        }
      } else {
        if (getGroup(stat.group)->osmid == 1 &&
            getGroup(stat.group)->stations.size() > 1) {
          // move station from relation into new group
          sug.type = 3;
          sug.orig_gid = stat.origGroup;
          sug.orig_osm_rel_id = getGroup(stat.origGroup)->osmid;
          sug.target_gid = stat.group;

          auto b = util::geo::centroid(getGroup(stat.group)->poly);
          sug.arrow = getGroupArrow(centroid, b);

          _suggestions.push_back(sug);
          stat.suggestions.push_back(_suggestions.size() - 1);

          _numCR++;
        } else if (getGroup(stat.group)->osmid > 1) {
          if (stat.group != getGroup(stat.origGroup)->mergeId) {
            // move station from relation into existing group
            sug.type = 4;
            sug.target_gid = stat.group;
            sug.target_osm_rel_id = getGroup(stat.group)->osmid;

            auto b = util::geo::centroid(getGroup(stat.group)->poly);
            sug.arrow = getGroupArrow(centroid, b);

            _numMV++;

            sug.orig_gid = stat.origGroup;
            sug.orig_osm_rel_id = getGroup(stat.origGroup)->osmid;

            _suggestions.push_back(sug);
            stat.suggestions.push_back(_suggestions.size() - 1);
          }
        } else if (!hasStationTrackHeur) {
          // move station out of relation
          //
          // don't show this suggestion if the station track heuristic was
          // triggered
          sug.type = 5;
          sug.orig_gid = stat.origGroup;
          sug.target_gid = stat.group;
          sug.target_osm_rel_id = getGroup(stat.group)->osmid;

          auto b = util::geo::centroid(stat.pos);
          b.setX(b.getX() + 50);
          b.setY(b.getY() + 50);

          sug.arrow = getGroupArrow(centroid, b);

          _suggestions.push_back(sug);
          stat.suggestions.push_back(_suggestions.size() - 1);

          _numDL++;
        }
      }
    }

    if (stat.suggestions.size() + stat.attrErrs.size() == 0) _numOK++;
  }

  // meta groups are always ok
  _numOK += _numOSMMetaGroups;
}

// _____________________________________________________________________________
util::geo::DLine StatIdx::getGroupArrow(const util::geo::DPoint& a,
                                        const util::geo::DPoint& b) const {
  auto pl = util::geo::PolyLine<double>(a, b);
  auto bb = pl.getPointAtDist(fmax(pl.getLength() / 2, pl.getLength() - 5)).p;

  if (util::geo::dist(a, bb) < 1) {
    bb = a;
    bb.setX(bb.getX() + 3);
  }

  util::geo::Line<double> line;

  if (util::geo::dist(a, bb) < 20) {
    line = {a, bb};
  } else {
    int side = rand() % 2;
    if (!side) side = -1;

    auto rot1 = util::geo::rotate(util::geo::DLine{a, bb}, 45 * side, a);
    auto rot2 = util::geo::rotate(util::geo::DLine{a, bb}, -20 * side, bb);
    auto rot3 = util::geo::rotate(util::geo::DLine{a, bb}, -120 * side, bb);

    auto i = util::geo::intersection(
        util::geo::LineSegment<double>{rot1[0], rot1[1]},
        util::geo::LineSegment<double>{rot2[0], rot2[1]});

    line =
        util::geo::BezierCurve<double>(a, i, rot3[0], bb).render(5).getLine();
  }

  // arrowhead
  auto pl2 = util::geo::PolyLine<double>(line);
  util::geo::LineSegment<double> headSeg(
      pl2.getPointAtDist(pl2.getLength() - 5).p, line.back());
  auto arrHeadLeft = util::geo::rotate(headSeg, -45, line.back()).first;
  auto arrHeadRight = util::geo::rotate(headSeg, 45, line.back()).first;

  auto tmp = line.back();

  line.push_back(arrHeadLeft);
  line.push_back(tmp);
  line.push_back(arrHeadRight);

  return line;
}

// _____________________________________________________________________________
const Group* StatIdx::getGroup(size_t id) const {
  if (id >= _groups.size()) return 0;
  return &_groups[id];
}

// _____________________________________________________________________________
const MetaGroup* StatIdx::getMetaGroup(size_t id) const {
  if (_metaGroups.count(id)) return &_metaGroups.find(id)->second;
  return 0;
}

// _____________________________________________________________________________
int StatIdx::nameAttrRel(const std::string& attr) const {
  if (attr == "uic_name") return 5;
  if (attr == "ref_name") return 5;
  if (attr == "nat_name") return 4;
  if (attr == "official_name") return 3;
  if (attr == "gtfs_name") return 3;
  if (attr == "int_name") return 2;
  if (attr == "name") return 1;

  if (attr == "alt_name") return -1;

  return 0;
}

// _____________________________________________________________________________
const Suggestion* StatIdx::getSuggestion(size_t id) const {
  if (id >= _suggestions.size()) return 0;
  return &_suggestions[id];
}
