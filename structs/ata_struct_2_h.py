#!/usr/bin/python

import sys
import yaml

def emit_func_bit(name, field, params):
	bit_params = dict(name=name, field=field, word=int(params[0]), bit=int(params[1]))
	print("""static inline bool ata_get_%(name)s_%(field)s(const char *buf) {
	ata_word_t val = ata_get_word(buf, %(word)d);
	return val & (1 << %(bit)d);
}
""" % bit_params)

def emit_func_string(name, field, params):
	bit_params = dict(name=name, field=field, word_start=int(params[0]), word_end=int(params[1]))
	print("""static inline void ata_get_%(name)s_%(field)s(const char *buf, char *out) {
	ata_get_string(buf, %(word_start)d, %(word_end)d, out);
}
""" % bit_params)

def emit_func_longword(name, field, params):
	bit_params = dict(name=name, field=field, word_start=int(params))
	print("""static inline ata_longword_t ata_get_%(name)s_%(field)s(const char *buf) {
	return ata_get_longword(buf, %(word_start)d);
}
""" % bit_params)

kinds = {
	'bit': emit_func_bit,
	'string': emit_func_string,
	'longword': emit_func_longword,
}

def emit_header_single(name, struct):
	for field, info in list(struct.items()):
		keys = list(info.keys())
		assert(len(keys) == 1)
		kind = keys[0]
		params = info[kind]

		kinds[kind](name, field, params)

def emit_header(structs):
	for name, struct in list(structs.items()):
		emit_header_single(name, struct)

def emit_prefix():
	print('/* Generated file, do not edit */')
	print('#ifndef ATA_PARSE_H')
	print('#define ATA_PARSE_H')
	print('#include "ata.h"')

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
