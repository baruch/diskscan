exec >&2

redo-always
redo-ifchange config
redo-ifchange Documentation/all version/all

# Clean unneeded files

# We have everything we need to tar everything up
. version/vars
NAME="diskscan-$TAG"
TARNAME="$NAME.tar.bz2"
tar --transform "s|^|$NAME/|" -X dist.exclude -cvjf "$TARNAME" *

# Create a fixed name that can be used in scripts
ln -sf "$TARNAME" "diskscan.tar.bz2"
