#!/usr/bin/env python

import sys
import csv
import os

def parse_file(f):
    c = csv.reader(f)
    for line in c:
        msg = line[0]
        if msg != '': continue
        if len(line) != 4: continue

        cdb = line[1]
        sense = line[2]
        data = line[3]

        print('Parsing CDB: %s' % cdb)
        print('Parsing Sense: %s' % sense)
        print('Parsing Data: %s' % data)
        sys.stdout.flush()
        sys.stderr.flush()
        os.system('../parse_scsi "%s" "%s" "%s"' % (cdb, sense, data))
        sys.stdout.flush()
        sys.stderr.flush()
        print('=========================================================================\n')

if len(sys.argv) > 1:
    for filename in sys.argv[1:]:
        print('Parsing file %s' % filename)
        f = file(filename, 'r')
        parse_file(f)
        print('Done')
        print('')
else:
    parse_file(sys.stdin)
