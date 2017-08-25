#!/usr/bin/env python

import sys
import subprocess

files = {
        ',4d ' : 'log_sense',
        ',25 ' : 'read_cap_10',
        ',9e ' : 'read_cap_16',
        ',12 ' : 'inquiry',
        ',5a ' : 'mode_sense_10',
        ',1a ' : 'mode_sense_6',
        ',37 ' : 'read_defect_data_10',
        ',b7 ' : 'read_defect_data_12',
        ',1c ' : 'receive_diagnostics',
}

for key in files.keys():
    filename = files[key] + '.csv.bz2'
    f = subprocess.Popen(['/bin/bzip2', '-z', '-c', '-9'], stdin=subprocess.PIPE, stdout=file(filename, 'w'))
    if f is None:
        print 'Failed to create process for', filename
    files[key] = f

for line in sys.stdin:
    prefix = line[0:4]
    f = files.get(prefix, None)
    if f is None:
        print '"%s"' % prefix
        continue
    f.stdin.write(line)

for key in files.keys():
    print 'Closing', key
    files[key].stdin.close()
    files[key].wait()
    print 'Done'
