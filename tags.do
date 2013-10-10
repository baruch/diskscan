SRCS=$(find . -name \*.c -or -name \*.h)
redo-ifchange $SRCS
ctags -f $3 $SRCS
