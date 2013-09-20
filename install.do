exec >&2
redo-ifchange diskscan README.md Documentation/all

: ${INSTALL:=install}
: ${DESTDIR:=}
: ${PREFIX:=/usr}
: ${MANDIR:=$DESTDIR$PREFIX/share/man}
: ${DOCDIR:=$DESTDIR$PREFIX/share/doc/diskscan}
: ${BINDIR:=$DESTDIR$PREFIX/bin}

echo "Installing to: $DESTDIR$PREFIX"

# make dirs
$INSTALL -d $MANDIR/man1 $DOCDIR $BINDIR

# docs
$INSTALL -m 0644 README.md $DOCDIR

# manpages
$INSTALL -m 0644 Documentation/*.1 $MANDIR/man1

# binaries
$INSTALL -m 0755 diskscan $BINDIR
