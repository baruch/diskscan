SRC="main.c ata_smart_read_data.c"
OBJ=${SRC/.c/.o}
redo-ifchange $OBJ ../libscsicmd.a
gcc -I ../include -o $3 $OBJ ../libscsicmd.a
