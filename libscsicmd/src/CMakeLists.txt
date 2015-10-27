add_custom_command(OUTPUT smartdb/smartdb_gen.c
                   COMMAND smartdb/smartdb_gen_c.py smartdb/smartdb.xml > smartdb/smartdb_gen.c
                   DEPENDS smartdb/smartdb_gen_c.py smartdb/smartdb.xml
                   )
add_custom_target(ata_parse.h
                   COMMAND ../structs/ata_struct_2_h.py ../structs/ata_identify.yaml > ../include/ata_parse.h
                   DEPENDS ../structs/ata_struct_2_h.py ../structs/ata_identify.yaml)

add_library(scsicmd STATIC ata.c ata_smart.c cdb.c parse_inquiry.c parse_read_cap.c parse_sense.c str_map.c smartdb/smartdb.c smartdb/smartdb_gen.c)
add_dependencies(scsicmd ata_parse.h)
