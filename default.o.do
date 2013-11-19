exec >&2
redo-ifchange $2.c
DEP_FILE=$(dirname $2)/.$(basename $2).d
CFLAGS="-O3 -g -Wall -Wextra -Wformat=2 -Winit-self -Winline -Wpacked -Wp,-D_FORTIFY_SOURCE=2 -Wpointer-arith -Wlarger-than-65500 -Wmissing-declarations -Wmissing-format-attribute -Wmissing-noreturn -Wnested-externs -Wold-style-definition -Wredundant-decls -Wsign-compare -Wstrict-aliasing=2 -Wstrict-prototypes -Wswitch-enum -Wundef -Wunreachable-code -Wwrite-strings -Wfloat-equal -fno-common -Werror -D_GNU_SOURCE -D_FILE_OFFSET_BITS=64 $CFLAGS"
CC=${CC:-gcc}
$CC $CPPFLAGS $CFLAGS -I$PWD/include -M -MG -MF $DEP_FILE -E -o /dev/null $2.c
read DEPS <$DEP_FILE
redo-ifchange ${DEPS#*:}
[ -n "$VERBOSE" ] && echo $CC $CPPFLAGS $CFLAGS -I$PWD/include -I. -c -o $3 $2.c
$CC $CPPFLAGS $CFLAGS -I$PWD/include -I. -c -o $3 $2.c
