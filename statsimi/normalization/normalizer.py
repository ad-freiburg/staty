# -*- coding: utf-8 -*-
'''
Copyright 2019, University of Freiburg.
Chair of Algorithms and Data Structures.
Patrick Brosi <brosi@informatik.uni-freiburg.de>
'''

import re
import logging


class Normalizer(object):
    '''
    Normalizes stations and station groups according to rules from
    a normalization file.
    '''

    def __init__(self, rulesfile=None):
        '''
        >>> n = Normalizer("testdata/test.rules")
        >>> len(n.chain)
        120
        '''

        self.log = logging.getLogger('normzer')
        self.chain = []

        self.log.info("Reading label normalization rules from %s" % rulesfile)
        with open(rulesfile, 'r', encoding="utf-8") as f:
            for line in f:
                line = line.strip()
                if len(line) < 1 or line[0] == '#':
                    # skip comments
                    continue
                entry = line.split(" -> ")
                entry[1] = entry[1].rstrip(";")
                entry[0] = entry[0].strip("'")
                entry[1] = entry[1].strip("'")
                self.chain.append((re.compile(entry[0]), entry[1]))

    def normalize(self, groups, stats):
        '''
        Normalize all names and all stations in groups and stats

        >>> from statsimi.osm.osm_parser import OsmParser
        >>> p = OsmParser()
        >>> p.parse_xml("testdata/test.osm")
        >>> n = Normalizer("testdata/test.rules")
        >>> n.normalize(p.groups, p.stations)
        >>> p.groups[0].names[0]
        ('schwabentorbruecke', 'name')
        '''
        for gid, g in enumerate(groups):
            for nid, n in enumerate(g.names):
                normed = self.normalize_string(n[0])
                if len(normed) == 0:
                    self.log.warn("Normalization for '%s' is empty!" % n[0])
                g.names[nid] = (normed, n[1])
        for sid, st in enumerate(stats):
            if st.name is not None:
                normed = self.normalize_string(st.name)
                if len(normed) == 0:
                    self.log.warn("Normalization for '%s' is empty!" % st.name)
                stats[sid].name = normed

    def normalize_string(self, a):
        '''
        >>> n = Normalizer("testdata/test.rules")
        >>> n.normalize_string("Hbf Freiburg")
        'freiburg hauptbahnhof'
        >>> n.normalize_string("Hbf, Freiburg")
        'freiburg hauptbahnhof'
        >>> n.normalize_string("Freiburg,    Hbf")
        'freiburg hauptbahnhof'
        >>> n.normalize_string("Freiburg,   Hbf   ")
        'freiburg hauptbahnhof'
        >>> n.normalize_string("Freiburg, Hbf Gleis 5 ")
        'freiburg hauptbahnhof'
        >>> n.normalize_string("Freiburg,  Hbf Gleis 5,U + S ")
        'freiburg hauptbahnhof'
        >>> n.normalize_string("Z??rich HB")
        'zuerich hauptbahnhof'
        >>> n.normalize_string("Liestal Bf")
        'liestal bahnhof'
        >>> n.normalize_string("La??bergstra??e Freiburg im Breisgau")
        'lassberg strasse freiburg breisgau'
        >>> n.normalize_string("La??bergstra??e Freiburg i. Breisgau")
        'lassberg strasse freiburg breisgau'
        >>> n.normalize_string("La??bergstr. Freiburg(Breisgau)")
        'lassberg strasse freiburg breisgau'
        >>> n.normalize_string("La??bergstr. Freiburg ( Breisgau)")
        'lassberg strasse freiburg breisgau'
        >>> n.normalize_string("Freib??rg; Teststr.+Testav.  ")
        'freibuerg test strasse test avenue'
        >>> n.normalize_string("Freib??rg; Test str.+Testav.  ")
        'freibuerg test strasse test avenue'
        '''
        orig = a
        for rule in self.chain:
            a = rule[0].sub(rule[1], a.lower())

        return a
