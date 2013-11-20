OBJS="src/cdb.o src/parse_inquiry.o src/ata.o src/parse_sense.o src/parse_read_cap.o src/str_map.o"
redo-ifchange ${OBJS}

ar rcs $3 $OBJS
