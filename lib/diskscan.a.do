OBJS="verbose.o diskscan.o data.o system_id.o sha1.o ../version/version.o"
redo-ifchange $OBJS
ar rc $3 $OBJS
