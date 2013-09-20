% diskscan(1) DiskScan %VERSION%
% Baruch Even <baruch@ev-en.org>
% %DATE%

# NAME

diskscan - scan a disk for failed and near failure sectors


# SYNOPSIS

diskscan [options...] block_device


# DESCRIPTION

diskscan reads the entire block device and notes the time it took to read a
block. When there is an error it is immediately noted and also when there is a
higher latency to read a block. A histogram of the block latency times is also
given to assess the health of the disk.


# OPTIONS

-v
:   display verbose information from the workings of the scan
    use multiple times for increased verbosity.
    

# SEE ALSO

`badblocks`(1), `fsck`(1)
