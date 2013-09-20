DiskScan -- Scan HDD/SSD for failed and near failed sectors
-----------------------------------------------------------

DiskScan is a Unix/Linux tool to scan a block device and check if there are
unreadable sectors, in addition it uses read latency times as an assessment for
a near failure as sectors that are problematic to read usually entail many
retries. This can be used to assess the state of the disk and maybe decide on a
replacement in advance to its imminent failure. The disk self test may or may
not pick up on such clues depending on the disk vendor decision making logic.

Build
-----

This project is using [redo][1] and includes the minimal do program as a
replacement if you don't have the full redo installed. As a shortcut for those
who are used to the traditional Make there is a Makefile provided that does the
right thing.

Install
-------

make install

You can control the DESTDIR when building packages and PREFIX if /usr is not right.

License
-------

DiskScan is licensed under the GPL version 3 or later.


 [1]: https://github.com/apenwarr/redo
