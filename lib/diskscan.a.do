OBJS="verbose.o diskscan.o"
redo-ifchange $OBJS
ar rc $3 $OBJS
