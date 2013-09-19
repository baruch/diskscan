exec >&2
SRCS=$(find . -name \*.c -or -name \*.h)
redo-ifchange $SRCS
# Do not panic if ctags is not available, most likely this is a non-developer trying to build or a build machine
which ctags >/dev/null && ctags -f $3 $SRCS || /bin/true
