all:
	./do all

static:
	LDFLAGS=-static ./do all

clean:
	./do clean

install:
	./do all install

distclean:
	./do clean
