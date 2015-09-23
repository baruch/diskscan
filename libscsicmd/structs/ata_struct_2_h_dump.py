#!/usr/bin/python

import sys
import yaml

def emit_header(structs):
	for name, struct in list(structs.items()):
		print('void dump_%s(const char *buf);' % name)

def emit_prefix():
	print('#ifndef _DUMP_H_')
	print('#define _DUMP_H_')

def emit_suffix():
	print('#endif')

def convert_def(filename):
	f = file(filename)
	structs = yaml.load(f)
	f.close()
	emit_header(structs)

if __name__ == '__main__':
	emit_prefix()
	filenames = sys.argv[1:]
	for filename in filenames:
		convert_def(filename)
	emit_suffix()
