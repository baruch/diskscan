OBJS="diskscan.o"
LIBS="cli/cli.a lib/diskscan.a"
redo-ifchange $OBJS $LIBS
gcc -o $3 $OBJS $LIBS -lrt
