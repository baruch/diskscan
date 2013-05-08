OBJS="src/cdb.o src/parse_inquiry.o src/ata.o src/parse_sense.o"
HEADERS="include/ata_parse.h"
redo-ifchange ${OBJS} ${HEADERS}

ar rcs $3 $OBJS
