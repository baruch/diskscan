exec >&2
OBJS="diskscan.o"
LIBS="cli/cli.a lib/diskscan.a arch/arch.a"
redo-ifchange $OBJS $LIBS
CC=${CC:-gcc}
[ -n "$VERBOSE" ] && echo $CC -o $3 $LDFLAGS $OBJS $LIBS -lrt
$CC -o $3 $LDFLAGS $OBJS $LIBS -lrt
