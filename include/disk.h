#ifndef DISKSCAN_DISK_H
#define DISKSCAN_DISK_H

#include "arch.h"

/** Check if the disk had a smart trip, only relevant for ATA disks.
 *
 * Returns -1 on error, 0 if there is no smart trip and 1 if there is a smart trip.
 */
int disk_smart_trip(disk_dev_t *dev);

#endif
