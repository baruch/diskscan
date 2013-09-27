/*
 *  Copyright 2013 Baruch Even <baruch@ev-en.org>
 *
 *  This file is part of DiskScan.
 *
 *  DiskScan is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *  DiskScan is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with DiskScan.  If not, see <http://www.gnu.org/licenses/>.
 * 
 */

#include "diskscan.h"
#include "verbose.h"

#include <memory.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/fs.h>
#include <time.h>
#include <inttypes.h>
#include <errno.h>

int disk_open(disk_t *disk, const char *path, int fix)
{
	memset(disk, 0, sizeof(*disk));

	INFO("Validating path %s", path);
	if (access(path, F_OK)) {
		ERROR("Disk path %s does not exist: %m", path);
		return 1;
	}

	const int access_mode_flag = fix ? R_OK|W_OK : R_OK;
	if (access(path, access_mode_flag)) {
		ERROR("Disk path %s is inaccessible: %m", path);
		return 1;
	}

	const int open_mode_flag = fix ? O_RDWR : O_RDONLY;
	disk->fd = open(path, open_mode_flag|O_DIRECT);
	if (disk->fd < 0) {
		ERROR("Failed to open path %s: %m", path);
		return 1;
	}


	if (ioctl(disk->fd, BLKGETSIZE64, &disk->num_bytes) < 0) {
		ERROR("Can't get number of sectors of path %s: %m", path);
		return 1;
	}

	if (ioctl(disk->fd, BLKSSZGET, &disk->sector_size) < 0) {
		ERROR("Can't get sector size of path %s: %m", path);
		return 1;
	}

	if (disk->num_bytes == 0) {
		ERROR("Invalid number of sectors");
		return 1;
	}

	if (disk->sector_size == 0 || disk->sector_size % 512 != 0) {
		ERROR("Invalid sector size %d", disk->sector_size);
		return 1;
	}

	strncpy(disk->path, path, sizeof(disk->path));
	disk->path[sizeof(disk->path)-1] = 0;

	INFO("Opened disk %s", path);
	return 0;
}

int disk_close(disk_t *disk)
{
	INFO("Closed disk %s", disk->path);
	close(disk->fd);
	disk->fd = -1;
	return 0;
}

void disk_scan_stop(disk_t *disk)
{
	disk->run = 0;
}

static int decide_buffer_size(disk_t *disk)
{
	// TODO: Should use 1MB first and switch to sector size on errors, this will get the best speed overall and enough granularity.
	return 1024*1024;
}

static void *allocate_buffer(int buf_size)
{
	void *buf = mmap(NULL, buf_size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
	if (!buf)
		return NULL;

	madvise(buf, buf_size, MADV_DONTFORK);
	return buf;
}

static void free_buffer(void *buf, int buf_size)
{
	munmap(buf, buf_size);
}

static void disk_scan_part(disk_t *disk, uint64_t offset, void *data, int data_size)
{
	ssize_t ret;
	struct timespec t_start;
	struct timespec t_end;
	uint64_t t;

	clock_gettime(CLOCK_MONOTONIC, &t_start);
	ret = pread(disk->fd, data, data_size, offset);
	clock_gettime(CLOCK_MONOTONIC, &t_end);

	t = (t_end.tv_sec - t_start.tv_sec) * 1000000000 +
		t_end.tv_nsec - t_start.tv_nsec;

	if (ret != data_size) {
		ERROR("Error when reading at offset %" PRIu64 " size %d read %d: %m", offset, data_size, ret);
		report_scan_error(disk, offset, data_size, t);
		disk->num_errors++;
	}
	else {
		report_scan_success(disk, offset, data_size, t);
	}

	uint64_t t_msec = t / 1000000;
	int hist_idx = 0;
	while (t_msec >= histogram_time[hist_idx] && hist_idx < ARRAY_SIZE(disk->histogram) - 1) {
		hist_idx++;
	}
	disk->histogram[hist_idx]++;

	if (t_msec > 1000) {
		VERBOSE("Scanning at offset %" PRIu64 " took %llu msec", offset, t_msec);
	}

	if (t_msec > 3000) {
		INFO("Fixing region by rewriting, offset=%d size=%d", offset, data_size);
		ret = pwrite(disk->fd, data, data_size, offset);
		if (ret != 0) {
			ERROR("Error while attempting to rewrite the data! errno=%d: %m", errno);
		}
	}
}

int disk_scan(disk_t *disk)
{
	INFO("Scanning disk %s", disk->path);
	disk->run = 1;
	int data_size = decide_buffer_size(disk);
	void *data = allocate_buffer(data_size);

	VVVERBOSE("Using buffer of size %d", data_size);

	if (data == NULL) {
		ERROR("Failed to allocate data buffer: %m");
		return 1;
	}


	uint64_t offset;
	const uint64_t disk_size_bytes = disk->num_bytes;

	for (offset = 0; disk->run && offset < disk_size_bytes; offset += data_size) {
		VVERBOSE("Scanning at offset %llu out of %llu (%.2f%%)", offset, disk_size_bytes, (float)offset*100.0/(float)disk_size_bytes);
		uint64_t remainder = disk_size_bytes - offset;
		if (remainder < data_size) {
			data_size = remainder;
			VERBOSE("Last part scanning size %d", data_size);
		}
		disk_scan_part(disk, offset, data, data_size);
	}
	if (!disk->run) {
		INFO("Disk scan interrupted");
	}
	report_scan_done(disk);

	free_buffer(data, data_size);
	disk->run = 0;
	return 0;
}
