libscsicmd
==========

A library to create SCSI commands (CDBs) and parse the results, also for ATA commands and results.

This library doesn't deal with actually submitting the CDBs or getting the results from the storage device, only with
the commands themselves. The actual sending of the command is different between the different OSes and this library
tries to be OS agnostic.

Build
=====

The build system is using cmake, you need to get it before you can build.

To build, run:
  cmake . && make

Author
======
Baruch Even <baruch@ev-en.org>
