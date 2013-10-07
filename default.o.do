exec >&2
redo-ifchange $2.c
DEP_FILE=$(dirname $2)/.$(basename $2).d
CFLAGS="-O3 -g -Wall -Werror -D_GNU_SOURCE -D_FILE_OFFSET_BITS=64 $CFLAGS"
CC=${CC:-gcc}
$CC $CPPFLAGS $CFLAGS -I$PWD/include -M -MG -MF $DEP_FILE -E -o /dev/null $2.c
read DEPS <$DEP_FILE
redo-ifchange ${DEPS#*:}
[ -n "$VERBOSE" ] && echo $CC $CPPFLAGS $CFLAGS -I$PWD/include -I. -c -o $3 $2.c
$CC $CPPFLAGS $CFLAGS -I$PWD/include -I. -c -o $3 $2.c
