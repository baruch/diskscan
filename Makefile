# Adapter for those who don't know ninja

DESTDIR ?= 
PREFIX ?= /usr/local

all: build

build.ninja: configure
	./configure
	@if [ "${V}" = 1 ]; then \
		echo 'build.binja'; cat build.ninja; \
		echo 'lib.ninja'; cat libscsicmd/lib.ninja; \
	fi


build: build.ninja
	@if [ "${V}" = 1 ]; then ninja -v; else ninja; fi

clean:
	if [ -e build.ninja ]; then ninja -t clean; fi

distclean:
	-rm -rf build.ninja .ninja_deps .ninja_log libscsicmd/*.ninja libscsicmd/build/*.pyc

install: diskscan Documentation/diskscan.1
	install -m 0755 diskscan $(DESTDIR)$(PREFIX)/bin/diskscan
	install -m 0444 Documentation/diskscan.1 $(DESTDIR)$(PREFIX)/share/man/man1/diskscan.1

.PHONY: all clean distclean build install update-libscsicmd buildtest

buildtest:
	./configure
	ninja
	ninja -t clean
	
	./configure --dev
	ninja
	ninja -t clean
	
	CC="clang-3.8" ./configure --dev
	ninja
	ninja -t clean

# TODO: Port the scan to ninja
#scan:
#	scan-build ./do all

# Recreate targets: dist distclean

update-libscsicmd:
	git subtree pull --squash --prefix libscsicmd https://github.com/baruch/libscsicmd master

update-progressbar:
	git subtree pull --squash --prefix progressbar https://github.com/doches/progressbar master
