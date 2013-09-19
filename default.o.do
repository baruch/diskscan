redo-ifchange $2.c
DEP_FILE=$(dirname $2)/.$(basename $2).d
CFLAGS="-O0 -g -Wall -Werror -D_GNU_SOURCE -D_FILE_OFFSET_BITS=64"
gcc $CFLAGS -I$PWD/include -M -MG -MF $DEP_FILE -E -o /dev/null $2.c
read DEPS <$DEP_FILE
redo-ifchange ${DEPS#*:}
gcc $CFLAGS -I$PWD/include -I. -c -o $3 $2.c
