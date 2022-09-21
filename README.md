# DiskScan -- Scan HDD/SSD for failed and near failed sectors

[![Build Status](https://travis-ci.org/baruch/diskscan.svg?branch=master)](https://travis-ci.org/baruch/diskscan)
[![Gitter](https://badges.gitter.im/Join%20Chat.svg)](https://gitter.im/baruch/diskscan?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

DiskScan is a Unix/Linux tool to scan a block device and check if there are
unreadable sectors, in addition it uses read latency times as an assessment for
a near failure as sectors that are problematic to read usually entail many
retries. This can be used to assess the state of the disk and maybe decide on a
replacement in advance to its imminent failure. The disk self test may or may
not pick up on such clues depending on the disk vendor decision making logic.

## diskscan vs. badblocks

badblocks is intended for a simple task, to find bad blocks in the media. diskscan is trying to say a lot more about the media, specifically it is trying not just to say where is a bad block but also what blocks are already deteriorated but still readable and also give information on the latency of reading each block which should help to give an overall assessment of the disk media.

In essence badblocks looks for fatal issues already happening and diskscan is for upcoming issues that can be fixed.

Also, badblocks is essentially obsolete in this day and age since the disks themselves will reallocate the data and there is no real need to map the bad blocks in the filesystem level anymore.

## Build

This project is using CMake, on Debian/Ubuntu it is as simple as:
    apt-get install cmake make libtinfo-dev libncurses5-dev zlib1g-dev python-yaml

For RedHat/SuSe based distros you need to install ninja-build first and then:
    yum install compat-libtermcap libtermcap-devel cmake python-yaml zlib-devel

A Makefile is provided to avoid learning the ninja commands and do the non-build stuff (install, etc.)

To do the build:

    cmake . && make

## Install

make install

You can control the DESTDIR when building packages and PREFIX if /usr is not right.

## License

diskscan is licensed under the GPL version 3 or later.
