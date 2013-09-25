OBJS="diskscan.o"
LIBS="cli/cli.a lib/diskscan.a"
redo-ifchange $OBJS $LIBS
CC=${CC:-gcc}
$CC -o $3 $OBJS $LIBS $LDFLAGS -lrt
