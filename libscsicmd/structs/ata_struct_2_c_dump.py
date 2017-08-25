#!/usr/bin/env python

import sys
import yaml

def emit_func_bit(name, field, params):
	bit_params = dict(name=name, field=field, word=int(params[0]), bit=int(params[1]))
	print('printf("%%-40s: %%s\\n", "%(field)s", ata_get_%(name)s_%(field)s(buf) ? "true" : "false");' % bit_params)

def emit_func_string(name, field, params):
	bit_params = dict(name=name, field=field, word_start=int(params[0]), word_end=int(params[1]))
	print('{')
	print('char outbuf[1024];')
	print('ata_get_%(name)s_%(field)s(buf, outbuf);' % bit_params)
	print('printf("%%-40s: %%s\\n", "%(field)s", outbuf);' % bit_params)
	print('}')

def emit_func_bits(name, field, params):
	bit_params = dict(name=name, field=field, word_start=int(params[0]))
	print('printf("%%-40s: %%u\\n", "%(field)s", ata_get_%(name)s_%(field)s(buf));' % bit_params)

def emit_func_longword(name, field, params):
	bit_params = dict(name=name, field=field, word_start=int(params))
	print('printf("%%-40s: %%u\\n", "%(field)s", ata_get_%(name)s_%(field)s(buf));' % bit_params)

def emit_func_qword(name, field, params):
	bit_params = dict(name=name, field=field, word_start=int(params))
	print('printf("%%-40s: %%"PRIu64"\\n", "%(field)s", ata_get_%(name)s_%(field)s(buf));' % bit_params)

kinds = {
    'bit': emit_func_bit,
    'bits': emit_func_bits,
	'string': emit_func_string,
	'longword': emit_func_longword,
    'qword': emit_func_qword,
}

def emit_header_single(name, struct):
	field_names = list(struct.keys())
	field_names.sort()

	for field in field_names:
		info = struct[field]
		keys = list(info.keys())
		assert(len(keys) == 1)
		kind = keys[0]
		params = info[kind]

		kinds[kind](name, field, params)

def emit_header(structs):
	for name, struct in list(structs.items()):
		print('void dump_%s(const unsigned char *buf)' % name)
		print('{')
		emit_header_single(name, struct)
		print('}')

def emit_prefix():
	print('#include "ata.h"')
	print('#include "ata_parse.h"')
	print('#include "ata_identify_dump.h"')
	print('#include <stdio.h>')
	print('#include <inttypes.h>')

def emit_suffix():
	print('')

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
