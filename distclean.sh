#!/bin/sh

# distclean.sh for diskscan
# Copyright 2015 Joao Eriberto Mota Filho <eriberto@eriberto.pro.br>
# v2015111402
# This file can used under GPL-3+ license or BSD-3-Clause.

PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin

PREFIX="."

# Files and directories to remove

OBJS="	CMakeFiles/ \
	Makefile \
	CMakeCache.txt \
	Documentation/diskscan.1 \
	cmake_install.cmake \
	install_manifest.txt \
	libscsicmd/src/CMakeFiles/ \
	libscsicmd/src/Makefile \
	libscsicmd/src/cmake_install.cmake \
        tags"

# Main procedures

remove_files () {
    if [ -e "$PREFIX/$TARGET" ]; then
        echo "Removing $PREFIX/$TARGET"
        rm -rf "${PREFIX:?}/$TARGET"
    else
        echo "$PREFIX/$TARGET NOT FOUND."
    fi
}

# Distclean

echo "DOING DISTCLEAN..."

for TARGET in $OBJS
do
    remove_files
done

echo "DONE."
echo
