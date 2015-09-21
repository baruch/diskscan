# Adapter for those who don't know ninja

DESTDIR ?= 
PREFIX ?= /usr/local

all: build

build.ninja: configure
	./configure

build: build.ninja
	@if [ "${V}" = 1 ]; then ninja -v; else ninja; fi

clean:
	ninja -t clean

install: diskscan Documentation/diskscan.1
	install -m 0755 diskscan $(DESTDIR)$(PREFIX)/bin/diskscan
	install -m 0444 Documentation/diskscan.1 $(DESTDIR)$(PREFIX)/man/man1/diskscan.1

.PHONY: all clean build install

# TODO: Port the scan to ninja
#scan:
#	scan-build ./do all

# Recreate targets: dist distclean
