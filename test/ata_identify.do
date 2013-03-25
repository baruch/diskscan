INC="ata_identify_dump.h"
redo-ifchange $INC
SRC="ata_identify.c ata_identify_dump.c"
OBJ="${SRC/.c/.o}"
redo-ifchange $OBJ ../libscsicmd.a
gcc -I../include -o $3 $OBJ ../libscsicmd.a
