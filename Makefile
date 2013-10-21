all:
	./do all

static:
	LDFLAGS=-static ./do all

scan:
	scan-build ./do all

clean:
	./do clean

install:
	./do all install

# This only works with redo
dist:
	redo dist

distclean:
	./do clean
