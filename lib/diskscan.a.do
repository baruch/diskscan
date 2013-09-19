OBJS="verbose.o diskscan.o ../version/version.o"
redo-ifchange $OBJS
ar rc $3 $OBJS
