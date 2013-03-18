SRC=ata_smart_return_status.c
OBJ=${SRC/.c/.o}
redo-ifchange $OBJ ../libscsicmd.a
gcc -o $3 $OBJ ../libscsicmd.a
