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
#include "arch.h"
#include "median.h"

#include <memory.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <time.h>
#include <inttypes.h>
#include <errno.h>
#include <assert.h>

struct scan_state {
	uint32_t latency_bucket;
	uint64_t latency_stride;
	uint32_t latency_count;
	uint32_t *latency;
};

int disk_open(disk_t *disk, const char *path, int fix, unsigned latency_graph_len)
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

	if (get_block_device_size(disk->fd, &disk->num_bytes, &disk->sector_size) < 0) {
		ERROR("Can't get block device size information for path %s: %m", path);
		goto Error;
	}

	if (disk->num_bytes == 0) {
		ERROR("Invalid number of sectors");
		goto Error;
	}

	if (disk->sector_size == 0 || disk->sector_size % 512 != 0) {
		ERROR("Invalid sector size %d", disk->sector_size);
		goto Error;
	}

#if 0
	const uint64_t new_bytes_raw = disk->num_bytes / 10;
	const uint64_t new_bytes_leftover = new_bytes_raw % 512;
	const uint64_t new_bytes = new_bytes_raw - new_bytes_leftover;
	disk->num_bytes = new_bytes;
#endif

	strncpy(disk->path, path, sizeof(disk->path));
	disk->path[sizeof(disk->path)-1] = 0;

	disk->latency_graph_len = latency_graph_len;
	disk->latency_graph = calloc(latency_graph_len, sizeof(latency_t));
	if (disk->latency_graph == NULL) {
		ERROR("Failed to allocate memory for latency graph data");
		goto Error;
	}

	INFO("Opened disk %s sector size %u num bytes %llu", path, disk->sector_size, disk->num_bytes);
	return 0;

Error:
	disk_close(disk);
	return 1;
}

int disk_close(disk_t *disk)
{
	INFO("Closed disk %s", disk->path);
	close(disk->fd);
	disk->fd = -1;
	if (disk->latency_graph) {
		free(disk->latency_graph);
		disk->latency_graph = NULL;
	}
	return 0;
}

void disk_scan_stop(disk_t *disk)
{
	disk->run = 0;
}

static int decide_buffer_size(disk_t *disk)
{
	// TODO: Should use 64KB first and switch to sector size on errors, this will get the best speed overall and enough granularity.
	return 64*1024;
}

static void *allocate_buffer(int buf_size)
{
	void *buf = mmap(NULL, buf_size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
	if (!buf)
		return NULL;

	return buf;
}

static void free_buffer(void *buf, int buf_size)
{
	munmap(buf, buf_size);
}

static void latency_bucket_prepare(disk_t *disk, struct scan_state *state, uint64_t offset)
{
	assert(state->latency_bucket < disk->latency_graph_len);
	latency_t *l = &disk->latency_graph[state->latency_bucket];
	const uint64_t start_sector = offset / disk->sector_size;

	VVERBOSE("bucket prepare bucket=%u", state->latency_bucket);

	l->start_sector = start_sector;
	l->latency_min_msec = UINT32_MAX;
	state->latency_count = 0;
}

static void latency_bucket_finish(disk_t *disk, struct scan_state *state, uint64_t offset)
{
	latency_t *l = &disk->latency_graph[state->latency_bucket];
	const uint64_t end_sector = offset / disk->sector_size;

	VVERBOSE("bucket finish bucket=%d", state->latency_bucket);

	l->end_sector = end_sector;
	l->latency_median_msec = median(state->latency, state->latency_count);

	state->latency_count = 0;
	state->latency_bucket++;
}

static void latency_bucket_step_if_needed(disk_t *disk, struct scan_state *state, uint64_t offset)
{
	const uint64_t bucket = offset / disk->sector_size / (state->latency_stride + 1);

	if (bucket != state->latency_bucket) {
		latency_bucket_finish(disk, state, offset);
		latency_bucket_prepare(disk, state, offset);
	}
}

static void latency_bucket_add(disk_t *disk, uint64_t latency, struct scan_state *state)
{
	latency_t *l = &disk->latency_graph[state->latency_bucket];

	if (latency < l->latency_min_msec)
		l->latency_min_msec = latency;
	if (l->latency_max_msec < latency)
		l->latency_max_msec = latency;

	// Collect info for median calculation later
	state->latency[state->latency_count++] = latency;
}

static void disk_scan_part(disk_t *disk, uint64_t offset, void *data, int data_size, uint64_t latency_stride, struct scan_state *state)
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

	latency_bucket_add(disk, t_msec, state);

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

static uint64_t calc_latency_stride(disk_t *disk)
{
	const uint64_t num_sectors = disk->num_bytes / disk->sector_size;
	const uint64_t stride_size = num_sectors / disk->latency_graph_len;
	// At this stage stride_size may have a reminder, we need to distribute the
	// latencies a bit more to avoid it Since the remainder can never be more
	// than the latency_graph_len we can just add one entry to all the buckets
	return stride_size + 1;
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

	struct scan_state state;
	uint64_t offset;
	const uint64_t disk_size_bytes = disk->num_bytes;
	const uint64_t latency_stride = calc_latency_stride(disk);
	VVERBOSE("latency stride is %llu", latency_stride);

	state.latency_bucket = 0;
	state.latency_stride = latency_stride;
	state.latency_count = 0;
	state.latency = malloc(sizeof(uint32_t) * latency_stride);

	latency_bucket_prepare(disk, &state, 0);
	for (offset = 0; disk->run && offset < disk_size_bytes; offset += data_size) {
		VVVERBOSE("Scanning at offset %llu out of %llu (%.2f%%)", offset, disk_size_bytes, (float)offset*100.0/(float)disk_size_bytes);
		latency_bucket_step_if_needed(disk, &state, offset);
		uint64_t remainder = disk_size_bytes - offset;
		if (remainder < data_size) {
			data_size = remainder;
			VERBOSE("Last part scanning size %d", data_size);
		}
		disk_scan_part(disk, offset, data, data_size, latency_stride, &state);
	}
	latency_bucket_finish(disk, &state, offset);
	free(state.latency);

	if (!disk->run) {
		INFO("Disk scan interrupted");
	}
	report_scan_done(disk);

	free_buffer(data, data_size);
	disk->run = 0;
	return 0;
}
