#!/bin/sh

echo "Regenerate generated files"

src/smartdb/smartdb_gen_c.py src/smartdb/smartdb.xml > src/smartdb/smartdb_gen.c
git add src/smartdb/smartdb_gen.c

structs/ata_struct_2_h.py structs/ata_identify.yaml > include/ata_parse.h
git add include/ata_parse.h
