YAML="../structs/ata_identify.yaml"
SCRIPT=../structs/ata_struct_2_h.py
redo-ifchange $SCRIPT $YAML
python $SCRIPT $YAML
