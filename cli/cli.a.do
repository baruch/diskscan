OBJS="verbose.o cli.o"
redo-ifchange $OBJS
ar rc $3 $OBJS
