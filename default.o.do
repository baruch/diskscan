# This is for generated files
HEADERS="include/ata_parse.h"

redo-ifchange $2.c ${HEADERS}
DEP_FILE=$(dirname $2)/.$(basename $2).d
CFLAGS="-O3 -g -Wall -Werror"
gcc $CFLAGS -I$PWD/include -MD -MF $DEP_FILE -c -o $3 $2.c
read DEPS <$DEP_FILE
redo-ifchange ${DEPS#*:}
