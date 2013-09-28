exec >&2 
redo-ifchange arch.id
ARCH=$(cat arch.id)
ARCH_FILE=arch-${ARCH}.c
redo-ifchange $ARCH_FILE
cp $ARCH_FILE $3
