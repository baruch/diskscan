SCRIPT=../structs/ata_struct_2_c_dump.py
DEF=../structs/ata_identify.yaml
redo-ifchange $SCRIPT $DEF
$SCRIPT $DEF
