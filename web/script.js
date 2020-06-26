var widths = [12, 13, 13, 13, 13, 13, 13, 13];
var opas = [0.8, 0.6, 0.5, 0.5, 0.4];
var mwidths = [1, 1, 1, 1.5, 2, 3, 5, 6, 6, 4, 3, 2];
var stCols = ['#78f378', '#0000c3', 'red'];

var osmUrl = "//www.openstreetmap.org/";

var grIdx, mGrIdx, stIdx, selectedRes, prevSearch, delayTimer;

var reqs = {};

var openedGr = -1;
var openedMGr = -1;
var openedSt = -1;


// station sugg messages
var sgMvOrNew = "Move into a <span class='grouplink' onmouseover='grHl(${tid})' onmouseout='grUnHl(${tid})'>new relation</span> <tt>public_transport=stop_area</tt>.";
var sgMvOrEx = "Move into relation <a onmouseover='grHl(${tid})' onmouseout='grUnHl(${tid})' href=\"" + osmUrl + "relation/${toid}\" target=\"_blank\">${toid}</a>.";
var sgMvRelNew = "Move from relation <a onmouseover='grHl(${oid})' onmouseout='grUnHl(${oid})' href=\"" + osmUrl + "relation/${ooid}\" target=\"_blank\">${ooid}</a> into a <span class='grouplink' onmouseover='grHl(${tid})' onmouseout='grUnHl(${tid})'>new relation</span> <tt>public_transport=stop_area</tt>.";
var sgMvRelRel = "Move from relation <a onmouseover='grHl(${oid})' onmouseout='grUnHl(${oid})' href=\"" + osmUrl + "relation/${ooid}\" target=\"_blank\">${ooid}</a> into relation <a onmouseover='grHl(${tid})' onmouseout='grUnHl(${tid})' href=\"" + osmUrl + "relation/${toid}\" target=\"_blank\">${toid}</a>.";
var sgMvOutRel = "Move out of relation <a onmouseover='grHl(${oid})' onmouseout='grUnHl(${oid})' href=\"" + osmUrl + "relation/${ooid}\" target=\"_blank\">${ooid}</a>";
var sgFixAttr = "Fix attribute <tt>${attr}</tt>.";
var sgAddName = "Consider adding a <tt><a target='_blank' href='//wiki.openstreetmap.org/wiki/Key:name'>name</a></tt> attribute.";
var sgAttrTr = "Attribute <tt>${attr}</tt> seems to be a track/platform number. Use <tt>ref</tt> for this and set <tt>${attr}</tt> to the station name.";
var sgMergeRel = "Merge parent relation <a onmouseover='grHl(${oid})' onmouseout='grUnHl(${oid})' href=\"" + osmUrl + "relation/${ooid}\" target=\"_blank\">${ooid}</a> with relation <a onmouseover='grHl(${tid})' onmouseout='grUnHl(${tid})' href=\"" + osmUrl + "relation/${toid}\" target=\"_blank\">${toid}</a>, or move them into a new relation <tt>public_transport=stop_area_group</tt>";
var sgMergeMetaGr = "Move parent relation <a onmouseover='grHl(${oid})' onmouseout='grUnHl(${oid})' href=\"" + osmUrl + "relation/${ooid}\" target=\"_blank\">${ooid}</a> into meta <tt>public_transport=stop_area_group</tt> relation <a onmouseover='mGrHl(${tid})' onmouseout='mGrUnHl(${tid})' href=\"" + osmUrl + "relation/${toid}\" target=\"_blank\">${toid}</a>";

var suggsMsg = [sgMvOrNew, sgMvOrEx, sgMvRelNew, sgMvRelRel, sgMvOutRel, sgFixAttr, sgAddName, sgAttrTr, sgMergeRel, sgMergeMetaGr];

// group sugg messages
var sgGrFixAttr = "Fix attribute <tt>${attr}</tt>.";
var sgGrAddName = "Consider adding a <tt><a target='_blank' href='//wiki.openstreetmap.org/wiki/Key:name'>name</a></tt> attribute.";
var sgGrAttrTr = "Attribute <tt>${attr}</tt> seems to be a track/platform number. Use <tt>ref</tt> for this and set <tt>${attr}</tt> to the station name.";
var sgGrMergeRel = "Merge with relation <a onmouseover='grHl(${tid})' onmouseout='grUnHl(${tid})' href=\"" + osmUrl + "relation/${toid}\" target=\"_blank\">${toid}</a>, or move both into a new relation <tt>public_transport=stop_area_group</tt>";
var sgGrMergeMeta = "Move relation into <tt>public_transport=stop_area_group</tt> relation <a onmouseover='mGrHl(${tid})' onmouseout='mGrUnHl(${tid})' href=\"" + osmUrl + "relation/${toid}\" target=\"_blank\">${toid}</a>";

var groupSuggMsg = [sgGrFixAttr, sgGrAddName, sgGrAttrTr, sgGrMergeRel, sgGrMergeMeta];

function $(a){return document.querySelectorAll(a)}
function $$(t){return document.createElement(t) }
function ll(g){return {"lat" : g[0], "lng" : g[1]}}
function hasCl(e, c){return e.className.split(" ").indexOf(c) != -1}
function addCl(e, c){if (!hasCl(e, c)) e.className += " " + c;e.className = e.className.trim()}
function delCl(e, c){var a = e.className.split(" "); delete a[a.indexOf(c)]; e.className = a.join(" ").trim()}
function stCol(s){return s.e ? stCols[2] : s.s ? stCols[1] : stCols[0]}
function tmpl(s, r){for (var p in r) s = s.replace(new RegExp("\\${" + p + "}", "g"), r[p]); return s}
function req(id, u, cb) {
    if (reqs[id]) reqs[id].abort();
    reqs[id] = new XMLHttpRequest();
    reqs[id].onreadystatechange = function() { if (this.readyState == 4 && this.status == 200 && this == reqs[id]) cb(JSON.parse(this.responseText))};
    reqs[id].open("GET", u, 1);
    reqs[id].send();
}
function aa(e, k, v) {e.setAttribute(k, v)}
function ap(a, b) {a.appendChild(b)}

function marker(stat, z) {
    if (stat.g.length == 1) {
        if (z > 15) {
            return L.circle(
                stat.g[0], {
                    color: '#000',
                    fillColor: stCol(stat),
                    radius: mwidths[23 - z],
                    fillOpacity: 1,
                    weight: z > 17 ? 1.5 : 1,
                    id: stat.i
                }
            );
        } else {
            return L.polyline(
                [stat.g[0], stat.g[0]], {
                    color: stCol(stat),
                    fillColor: stCol(stat),
                    weight: widths[15 - z],
                    opacity: opas[15 - z],
                    id: stat.i
                }
            );
        }
    } else {
        return L.polygon(
            stat.g, {
                color: z > 15 ? '#000': stCol(stat),
                fillColor: stCol(stat),
                smoothFactor: 0,
                fillOpacity: 0.75,
                weight: z > 17 ? 1.5 : 1,
                id: stat.i
            }
        );
    }
}

function poly(group, z, dotted) {
    var col = group.e ? 'red' : group.s ? '#0000c3' : '#85f385';
    var style = {
        color: col,
        fillColor: col,
        smoothFactor: 0.4,
        fillOpacity: 0.2,
        id: group.i
    };
    if (dotted) {
        var col = group.e ? 'red' : group.s ? '#0000c3' : '#a3b750';
        style.dashArray = '8, 8';
        style.fillOpacity = 0.15;
        style.color = col;
        style.fillColor = col;
        style.weight = 2;
    }
    if (z < 16) {
        style.weight = 11;
        style.opacity = 0.5;
        style.fillOpacity = 0.5;
    }
    return L.polygon(group.g, style)
}

function sugArr(sug, z) {
    return L.polyline(sug.a, {
        id: sug.i,
        color: '#0000c3',
        smoothFactor: 0.1,
        weight: 4,
        opacity: 0.5
    });
}

function rndrSt(stat) {
    openedSt = stat.id;
    stHl(stat.id);
    var attrrows = {};

    var way = stat.osmid < 0;
    var osmid = Math.abs(stat.osmid);
    var ident = way ? "Way" : "Node";

    var con = $$('div');
    aa(con, "id", "nav")

    var suggD = $$('div');
    aa(suggD, "id", "sugg")

    con.innerHTML = ident + " <a target='_blank' href='" + osmUrl + ident.toLowerCase()+"/" + osmid + "'>" + osmid + "</a>";

    if (stat.attrs.name) con.innerHTML += " (<b>\"" + stat.attrs.name + "\"</b>)";

    con.innerHTML += "<a class='ebut' target='_blank' href='" + osmUrl + "edit?" + ident.toLowerCase() + "=" + osmid +"'>&#9998;</a>";

    var attrTbl = $$('table');
    aa(attrTbl, "id", "attr-tbl")
    ap(con, attrTbl)
    ap(con, suggD)

    var tbody = $$('tbody');
    ap(attrTbl, tbody)

    for (var key in stat.attrs) {
        var row = $$('tr');
        var col1 = $$('td');
        var col2 = $$('td');
        addCl(col2, "err-wrap");
        ap(tbody, row)
        ap(row, col1)
        ap(row, col2)
        col1.innerHTML = "<a rel=\"noreferrer\" href=\"//wiki.openstreetmap.org/wiki/Key:" + key + "\" target=\"_blank\"><tt>" + key + "</tt></a>";
        for (var i = 0; i < stat.attrs[key].length; i++) col2.innerHTML += "<span class='attrval'>" + stat.attrs[key][i] + "</span>" + "<br>";
        attrrows[key] = row;
    }

    for (var i = 0; i < stat.attrerrs.length; i++) {
        var err = stat.attrerrs[i];
        var row = attrrows[err.attr[0]];
        addCl(row, "err-" + Math.round(err.conf * 10));

        var info = $$('div');

        if (err.other_grp) {
            // the mismatch was with a group name
            if (err.other_osmid > 1) info.innerHTML = "Does not match <tt>" + err.other_attr[0] + "</tt> in relation <a onmouseover='grHl( " + err.other_grp + ")' onmouseout='grUnHl( " + err.other_grp + ")' target=\"_blank\" href=\"" + osmUrl + "relation/" + Math.abs(err.other_osmid) + "\">" + Math.abs(err.other_osmid) + "</a>";
            else info.innerHTML = "Does not match <tt>" + err.other_attr[0] + "</tt> in relation <span onmouseover='grHl( " + err.other_grp + ")' onmouseout='grUnHl( " + err.other_grp + ")'>" + Math.abs(err.other_osmid) + "</span>";
        } else {
            // the mismatch was with another station
            if (err.other_osmid != stat.osmid) {
                var lident = err.other_osmid < 0 ? "way" : "node";
                info.innerHTML = "Does not match <tt>" + err.other_attr[0] + "</tt> in " + lident + " <a onmouseover='stHl( " + err.other + ")' onmouseout='stUnHl( " + err.other + ")' target=\"_blank\" href=\"" + osmUrl + lident+"/" + Math.abs(err.other_osmid) + "\">" + Math.abs(err.other_osmid) + "</a>";
            } else {
                info.innerHTML = "Does not match <tt>" + err.other_attr[0] + "</tt> = '" + err.other_attr[1] + "'";
            }
        }
        addCl(info, 'attr-err-info');
        ap(row.childNodes[1], info)
    }

    var suggList = $$('ul');

    if (stat.su.length) {
        var a = $$('span');
        addCl(a, "sugtit");
        a.innerHTML = "Suggestions";
        ap(suggD, a)
    }

    ap(suggD, suggList)

    for (var i = 0; i < stat.su.length; i++) {
        var sg = stat.su[i];
        var sgDiv = $$('li');        
        sgDiv.innerHTML = tmpl(suggsMsg[sg.type - 1], {"attr" : sg.attr, "tid" : sg.target_gid, "ooid" : sg.orig_osm_rel_id, "toid" : sg.target_osm_rel_id, "oid" : sg.orig_gid});
        ap(suggList, sgDiv)
    }

    L.popup({opacity: 0.8})
        .setLatLng(stat)
        .setContent(con)
        .openOn(map)
        .on('remove', function() {if (openedSt == stat.id) {openedSt = -1; stUnHl(stat.id)}});
}

function openSt(id) {req("s", "/stat?id=" + id, function(c) {rndrSt(c)});}

function rndrGr(grp) {
    openedGr = grp.id;
    var attrrows = {};
    grHl(grp.id);

    var con = $$('div');
    aa(con, "id", "nav")

    var newMembers = $$('div');
    aa(newMembers, "id", "gsn")
    newMembers.innerHTML = "<span class='nmt'>New Members</span>";

    var oldMembers = $$('div');
    aa(oldMembers, "id", "gso")
    oldMembers.innerHTML = "<span class='omt'>Existing Members</span>";

    if (grp.osmid == 1) {
        con.innerHTML = "<span class='grouplink'>New relation</span> <tt>public_transport=stop_area</tt>";
    } else {
        con.innerHTML = "OSM relation <a target='_blank' href='" + osmUrl + "relation/" + grp.osmid + "'>" + grp.osmid + "</a>";

        if (grp.attrs.name) con.innerHTML += " (<b>\"" + grp.attrs.name + "\"</b>)";

        con.innerHTML += "<a class='ebut' target='_blank' href='" + osmUrl + "edit?relation=" + grp.osmid +"'>&#9998;</a>";
    }

    var attrTbl = $$('table');
    aa(attrTbl, "id", "attr-tbl")
    ap(con, attrTbl)

    var tbody = $$('tbody');
    ap(attrTbl, tbody)

    var suggD = $$('div');
    aa(suggD, "id", "sugg")

    for (var key in grp.attrs) {
        var row = $$('tr');
        var col1 = $$('td');
        var col2 = $$('td');
        addCl(col2, "err-wrap");
        ap(tbody, row)
        ap(row, col1)
        ap(row, col2)
        col1.innerHTML = "<a rel=\"noreferrer\" href=\"//wiki.openstreetmap.org/wiki/Key:" + key + "\" target=\"_blank\"><tt>" + key + "</tt></a>";
        for (var i = 0; i < grp.attrs[key].length; i++) col2.innerHTML += "<span class='attrval'>" + grp.attrs[key][i] + "</span>" + "<br>";
        attrrows[key] = row;
    }

    for (var i = 0; i < grp.attrerrs.length; i++) {
        var err = grp.attrerrs[i];
        var row = attrrows[err.attr[0]];
        addCl(row, "err-" + Math.round(err.conf * 10));

        var info = $$('div');

        if (err.other_grp) {
            // the mismatch was with a group name
            if (err.other_osmid != grp.osmid) {
                if (err.other_osmid > 1) info.innerHTML = "Does not match <tt>" + err.other_attr[0] + "</tt> in relation <a onmouseover='grHl( " + err.other_grp + ")' onmouseout='grUnHl( " + err.other_grp + ")' target=\"_blank\" href=\"" + osmUrl + "relation/" + Math.abs(err.other_osmid) + "\">" + Math.abs(err.other_osmid) + "</a>";
                else info.innerHTML = "Does not match <tt>" + err.other_attr[0] + "</tt> in relation <span onmouseover='grHl( " + err.other_grp + ")' onmouseout='grUnHl( " + err.other_grp + ")'>" + Math.abs(err.other_osmid) + "</span>";
            } else info.innerHTML = "Does not match <tt>" + err.other_attr[0] + "</tt> = '" + err.other_attr[1] + "'";
        } else {
            // the mismatch was with another station
            var ident = err.other_osmid < 0 ? "way" : "node";
            info.innerHTML = "Does not match <tt>" + err.other_attr[0] + "</tt> in " + ident + " <a onmouseover='stHl( " + err.other + ")' onmouseout='stUnHl( " + err.other + ")' target=\"_blank\" href=\"" + osmUrl + ident+"/" + Math.abs(err.other_osmid) + "\">" + Math.abs(err.other_osmid) + "</a>";
        }

        addCl(info, 'attr-err-info');
        ap(row.childNodes[1], info)
    }

    var suggList = $$('ul');

    if (grp.su.length) {
        var a = $$('span');
        addCl(a, "sugtit");
        a.innerHTML = "Suggestions";
        ap(suggD, a)
    }

    ap(suggD, suggList)
    var mergeGroup = false;

    for (var i = 0; i < grp.su.length; i++) {
        var sg = grp.su[i];
        var sgDiv = $$('li');

        if (sg.type == 9 || sg.type == 10) mergeGroup = true;

        sgDiv.innerHTML = tmpl(groupSuggMsg[sg.type - 6], {"attr" : sg.attr, "tid" : sg.target_gid, "ooid" : sg.orig_osm_rel_id, "toid" : sg.target_osm_rel_id, "oid" : sg.orig_gid});
        ap(suggList, sgDiv)
    }

    ap(con, newMembers)
    if (grp.osmid != 1) ap(con, oldMembers)

    for (var key in grp.stations) {
        var stat = grp.stations[key];
        var row = $$('div');
        var ident = stat.osmid < 0 ? "Way" : "Node";

        row.innerHTML = ident + " <a onmouseover='stHl( " + stat.id + ")' onmouseout='stUnHl( " + stat.id + ")' target='_blank' href='" + osmUrl + ident.toLowerCase() + "/" + Math.abs(stat.osmid) + "'>" + Math.abs(stat.osmid) + "</a>";

        if (stat.attrs.name) row.innerHTML += " (<b>\"" + stat.attrs.name + "\"</b>)";

        row.style.backgroundColor = stat.e ? '#f58d8d' : stat.s ? '#b6b6e4' : '#c0f7c0';

        if (grp.osmid == 1 || stat.orig_group != grp.id) ap(newMembers, row)
        else {
            ap(oldMembers, row)
            if (stat.group != grp.id && !mergeGroup) addCl(row, "del-stat");
        }
    }

    ap(con, suggD)

    L.popup({opacity: 0.8})
        .setLatLng(grp)
        .setContent(con)
        .openOn(map)
        .on('remove', function() {if (openedGr == grp.id) {openedGr = -1; grUnHl(grp.id)}});
}

function rndrMGr(grp) {
    openedMGr = grp.id;
    mGrHl(grp.id);

    var con = $$('div');
    aa(con, "id", "nav")

    var oldMembers = $$('div');
    aa(oldMembers, "id", "gso")
    oldMembers.innerHTML = "<span class='omt'>Members</span>";

    con.innerHTML = "OSM relation <a target='_blank' href='" + osmUrl + "/relation/" + grp.osmid + "'>" + grp.osmid + " </a> (<tt>public_transport=stop_area_group</tt>)";
    con.innerHTML += "<a class='ebut' target='_blank' href='" + osmUrl + "edit?relation=" + grp.osmid +"'>&#9998;</a>";

    ap(con, oldMembers)

    for (var key in grp.groups) {
        var gr = grp.groups[key];
        var row = $$('div');

        row.innerHTML = "Relation <a onmouseover='grHl( " + gr.id + ")' onmouseout='grUnHl( " + gr.id + ")' target='_blank' href='" + osmUrl + "relation/" + gr.osmid + "'>" + gr.osmid + "</a>";
        if (gr.attrs.name) row.innerHTML += " (<b>\"" + gr.attrs.name[0] + "\"</b>)";
        row.style.backgroundColor = gr.e ? '#f58d8d' : gr.s ? '#b6b6e4' : '#c0f7c0';

        ap(oldMembers, row)
    }

    L.popup({opacity: 0.8})
        .setLatLng(grp)
        .setContent(con)
        .openOn(map)
        .on('remove', function() {if (openedMGr == grp.id) {openedMGr = -1; mGrUnHl(grp.id)}});
}

function openGr(id) {
    req("g", "/group?id=" + id, function(c) {rndrGr(c)});
}

function openMGr(id) {
    req("g", "/mgroup?id=" + id, function(c) {rndrMGr(c)});
}

function mGrHl(id) {
    !mGrIdx[id] || mGrIdx[id].setStyle({'weight': 6, 'color': "#eecc00"});
}

function mGrUnHl(id) {
    !mGrIdx[id] || mGrIdx[id].setStyle({
        'weight': 2,
        'color': mGrIdx[id].options["fillColor"]
    });
}

function grHl(id) {
    !grIdx[id] || grIdx[id].setStyle({'weight': 6, 'color': "#eecc00"});
}

function grUnHl(id) {
    !grIdx[id] || grIdx[id].setStyle({
        'weight': 3,
        'color': grIdx[id].options["fillColor"]
    });
}

function stHl(id) {
    if (!stIdx[id]) return;

    if (map.getZoom() > 15) {
        stIdx[id].setStyle({
            'weight': 5,
            'color': "#eecc00"
        });
    } else {
        stIdx[id].setStyle({
            'color': "#eecc00"
        });   
    }
}

function stUnHl(id) {
    if (!stIdx[id]) return;

    if (map.getZoom() > 15) {
        stIdx[id].setStyle({
            'weight': map.getZoom() > 17 ? 1.5 : 1,
            'color': "#000"
        });
    } else {
        stIdx[id].setStyle({
            'color': stIdx[id].options["fillColor"]
        });   
    }
}

var map = L.map('m', {renderer: L.canvas(), attributionControl: false}).setView([47.9965, 7.8469], 13);

map.addControl(L.control.attribution({
    position: 'bottomright',
    prefix: '<a rel="noreferrer" target="_blank" href="//github.com/ad-freiburg/statsimi">Code on GitHub</a> | &copy; 2020 <a rel="noreferrer" target="_blank" href="//ad.cs.uni-freiburg.de">University of Freiburg, Chair of Algorithms and Data Structures</a>'
}));

map.on('popupopen', function(e) {
    var z = Math.max(map.getZoom(), 16);
    var px = map.project(e.target._popup._latlng, z);
    px.x += e.target._popup._container.clientWidth/2 - 20;
    px.y -= e.target._popup._container.clientHeight/2;

    map.setView(map.unproject(px, z), z, {animate: true});
    s();
});

L.tileLayer('https://stamen-tiles-{s}.a.ssl.fastly.net/toner-lite/{z}/{x}/{y}.png', {
    maxZoom: 20,
    attribution: '&copy; <a rel="noreferrer" target="_blank" href="' + osmUrl + '/copyright">OpenStreetMap</a>',
    opacity: 0.8
}).addTo(map);

var l = L.featureGroup().addTo(map);

map.on("moveend", function() {render();});
map.on("click", function() {s()});
map.on("zoomend", function() {$("main")[0].className = '';addCl($("main")[0], "z"+map.getZoom())});

function render() {
    if (map.getZoom() < 11) {
        var b = map.getBounds();
        var sw = b.getSouthWest();
        var ne = b.getNorthEast();
        req("m", "/heatmap?z=" + map.getZoom() + "&bbox=" + [sw.lat, sw.lng, ne.lat, ne.lng].join(","),
            function(re) {
                l.clearLayers();

                var blur = 22 - map.getZoom();
                var rad = 25 - map.getZoom();

                l.addLayer(L.heatLayer(re.ok, {
                    max: 500,
                    gradient: {
                        0: '#cbf7cb',
                        0.5: '#78f378',
                        1: '#29c329'
                    },
                    minOpacity: 0.65,
                    blur: blur,
                    radius: rad
                }));
                l.addLayer(L.heatLayer(re.sugg, {
                    max: 500,
                    gradient: {
                        0: '#7f7fbd',
                        0.5: '#4444b3',
                        1: '#0606c1'
                    },
                    minOpacity: 0.65,
                    blur: blur - 3,
                    radius: Math.min(12, rad - 3)
                }));
                l.addLayer(L.heatLayer(re.err, {
                    max: 500,
                    gradient: {
                        0: '#f39191',
                        0.5: '#ff5656',
                        1: '#ff0000'
                    },
                    minOpacity: 0.75,
                    blur: blur - 3,
                    radius: Math.min(10, rad - 3),
                    maxZoom: 15
                }));
            }
        )
    } else {
        req("m", "/map?z=" + map.getZoom() + "&bbox=" + [map.getBounds().getSouthWest().lat, map.getBounds().getSouthWest().lng, map.getBounds().getNorthEast().lat, map.getBounds().getNorthEast().lng].join(","),
            function(re) {
                l.clearLayers();
                grIdx = {};
                mGrIdx = {};
                stIdx = {};

                var stats = [];
                for (var i = 0; i < re.stats.length; i++) {
                    stIdx[re.stats[i].i] = stats[stats.push(marker(re.stats[i], map.getZoom())) - 1];
                }

                var groups = [];
                for (var i = 0; i < re.groups.length; i++) {
                    grIdx[re.groups[i].i] = groups[groups.push(poly(re.groups[i], map.getZoom())) - 1];
                }

                var mgroups = [];
                for (var i = 0; i < re.mgroups.length; i++) {
                    mGrIdx[re.mgroups[i].i] = mgroups[mgroups.push(poly(re.mgroups[i], map.getZoom(), 1)) - 1];
                }

                var suggs = [];
                for (var i = 0; i < re.su.length; i++) {
                    suggs.push(sugArr(re.su[i], map.getZoom()));
                }

                if (map.getZoom() > 13) {
                    l.addLayer(L.featureGroup(mgroups).on('click', function(a) {
                        openMGr(a.layer.options.id, a.layer.getBounds().getCenter());
                    }));
                    l.addLayer(L.featureGroup(groups).on('click', function(a) {
                        openGr(a.layer.options.id, a.layer.getBounds().getCenter());
                    }));
                }

                l.addLayer(L.featureGroup(stats).on('click', function(a) {
                    openSt(a.layer.options.id);
                }));

                if (map.getZoom() > 15) {
                    l.addLayer(L.featureGroup(suggs).on('click', function(a) {
                        if (a.layer.options.id < 0) openGr(-a.layer.options.id);
                        else openSt(a.layer.options.id);
                    }));
                }

                grHl(openedGr);
                mGrHl(openedMGr);
                stHl(openedSt);
            }
        )
    };
}

function rowClick(row) {    
    if (!isSearchOpen()) return;
    if (row.stat) openSt(row.stat.i, ll(row.stat.g[0]));
    else openGr(row.group.i, ll(row.group.g[0]));
}

function select(row) {    
    if (!row) return;
    if (!isSearchOpen()) return;
    unselect(selectedRes);
    selectedRes = row;
    addCl(row, "selres");
    if (row.stat) stHl(row.stat.i);
    if (row.group) grHl(row.group.i);
}

function unselect(row) {    
    selectedRes = undefined;
    if (!row) return;
    delCl(row, "selres");
    if (row.stat && row.stat.i != openedSt) stUnHl(row.stat.i);
    if (row.group && row.group.i != openedGr) grUnHl(row.group.i);
}

function isSearchOpen() {
    return $("#sres").className == "res-open";
}

function s(q) {
    var delay = 0;
    if (q == prevSearch) return;
    clearTimeout(delayTimer);
    prevSearch = q;
    //unselect(selectedRes);
    if (!q) {
        $('#si').value = "";
        $("#sres").className = "";
        $("#sres").innerHTML = "";
        return;
    }

    delayTimer = setTimeout(function() {
        req("sr", "/search?q=" + encodeURIComponent(q.substring(0, 100)), function(c) {
                var res = $("#sres");
                addCl(res, "res-open");
                res.innerHTML = "";
                for (var i = 0; i < c.length; i++) {
                    var e = c[i];
                    var row = $$('span');
                    addCl(row, "sres");
                    row.innerHTML = e.n;
                    if (e.w) addCl(row, "res-way");
                    if (e.s) {
                        row.stat = e.s;                    
                        addCl(row, "res-stat");
                        if (e.s.s) addCl(row, "res-sugg");                    
                        if (e.s.e) addCl(row, "res-err");
                    } else {
                        row.group = e.g;
                        addCl(row, "res-group");                       
                        if (e.g.s) addCl(row, "res-sugg");                    
                        if (e.g.e) addCl(row, "res-err");
                    }

                    row.onmouseover = function(){select(this)};
                    row.onclick = function(){rowClick(this)};

                    var dist = $$('span');
                    addCl(dist, "dist");
                    dist.innerHTML = dstr(e.s ? e.s.g : e.g.g);
                    ap(row, dist)

                    if (e.v && e.v != e.name) {
                        var via = $$('span');
                        addCl(via, "via");
                        via.innerHTML = e.v;
                        ap(row, via)
                    }
                    ap(res, row)
                }
                if ($('.sres').length > 0) select($('.sres')[0]);
            }
        )}, delay);
}

function dstr(s) {
    if (map.getBounds().contains(ll(s[0]))) return "";
    var d = map.distance(map.getCenter(), ll(s[0]));
    if (d < 500) return Math.round(d).toFixed(0) + "m";
    if (d < 5000) return (d / 1000.0).toFixed(1) + "km";
    return Math.round(d / 1000.0).toFixed(0) + "km";
}

function kp(e) {
    if (e.keyCode == 40 || (!e.shiftKey && e.keyCode == 9)) {
        var sels = $('.selres')
        if (sels.length) select(sels[0].nextSibling);
        else select($('.sres')[0]);
        e.preventDefault();
    } else if (e.keyCode == 38 || (e.shiftKey && e.keyCode == 9)) {
        var sels = $('.selres')
        if (sels.length) {
            if (sels[0].previousSibling) select(sels[0].previousSibling);
            else unselect(sels[0]);
            e.preventDefault();
        }
    }

    if (e.keyCode == 13) {
        var sels = $('.selres');
        if (sels.length) rowClick(sels[0]);
    }
}

$('#del').onclick = function() {s();$("#si").focus()}

render();