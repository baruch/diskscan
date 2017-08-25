# libscsicmd

A library to create SCSI commands (CDBs) and parse the results, also for ATA commands and results.

This library doesn't deal with actually submitting the CDBs or getting the results from the storage device, only with
the commands themselves. The actual sending of the command is different between the different OSes and this library
tries to be OS agnostic.

## Build

The build system is using cmake, you need to get it before you can build.

To build, run:

    cmake . && make

For a developer debug build use:

    cmake -DCMAKE_BUILD_TYPE=Debug .

To build for American Fuzzy Lop instrumentation:

    cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_COMPILER=/usr/bin/afl-gcc -DCMAKE_C_FLAGS=-DAFL_HARDEN=1 .

## Testing

Testing is either done manually with some raw data collected from SCSI devices
with test/collect\_raw\_data or with American Fuzzy Lop (AFL) for parsing
problems.

Using AFL:

    afl-fuzz -t 200 -i afl/testcase -o afl/finding test/parse_scsi

## Author

Baruch Even <baruch@ev-en.org>
