SRC="main.c ata_smart_return_status.c sense_dump.c"
OBJ=${SRC/.c/.o}
redo-ifchange $OBJ ../libscsicmd.a
gcc -I ../include -o $3 $OBJ ../libscsicmd.a
