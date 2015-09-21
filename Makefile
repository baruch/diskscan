# Adapter for those who don't know ninja
all: build

build.ninja: configure
	./configure

build: build.ninja
	ninja

clean:
	ninja -t clean

.PHONY: all clean build

# TODO: Port the scan to ninja
#scan:
#	scan-build ./do all

# Recreate targets: dist distclean install
