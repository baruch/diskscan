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

#include "config.h"
#include "diskscan.h"
#include "verbose.h"
#include "arch.h"
#include "median.h"
#include "compiler.h"

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
	void *data;
};

enum scan_mode str_to_scan_mode(const char *s)
{
	if (strcasecmp(s, "seq") == 0 || strcasecmp(s, "sequential") == 0)
		return SCAN_MODE_SEQ;
	if (strcasecmp(s, "random") == 0)
		return SCAN_MODE_RANDOM;
	return SCAN_MODE_UNKNOWN;
}

int disk_open(disk_t *disk, const char *path, int fix, unsigned latency_graph_len)
{
	memset(disk, 0, sizeof(*disk));
	disk->fix = fix;

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

	if (!disk_dev_open(&disk->dev, path)) {
		ERROR("Failed to open path %s: %m", path);
		return 1;
	}

	if (disk_dev_read_cap(&disk->dev, &disk->num_bytes, &disk->sector_size) < 0) {
		ERROR("Can't get block device size information for path %s: %m", path);
		goto Error;
	}

	if (disk->num_bytes == 0) {
		ERROR("Invalid number of sectors");
		goto Error;
	}

	if (disk->sector_size == 0 || disk->sector_size % 512 != 0) {
		ERROR("Invalid sector size %" PRIu64, disk->sector_size);
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

	INFO("Opened disk %s sector size %"PRIu64" num bytes %"PRIu64, path, disk->sector_size, disk->num_bytes);
	return 0;

Error:
	disk_close(disk);
	return 1;
}

int disk_close(disk_t *disk)
{
	INFO("Closed disk %s", disk->path);
	disk_dev_close(&disk->dev);
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

static void disk_scan_part(disk_t *disk, uint64_t offset, void *data, int data_size, struct scan_state *state)
{
	ssize_t ret;
	struct timespec t_start;
	struct timespec t_end;
	uint64_t t;
	int error = 0;

	clock_gettime(CLOCK_MONOTONIC, &t_start);
	ret = disk_dev_read(&disk->dev, offset, data_size, data);
	clock_gettime(CLOCK_MONOTONIC, &t_end);

	t = (t_end.tv_sec - t_start.tv_sec) * 1000000000 +
		t_end.tv_nsec - t_start.tv_nsec;

	if (ret != data_size) {
		int s_errno = errno;
		ERROR("Error when reading at offset %" PRIu64 " size %d read %zd: %m", offset, data_size, ret);
		report_scan_error(disk, offset, data_size, t);
		disk->num_errors++;
		error = 1;

		if (s_errno != EIO && s_errno != 0)
			abort();
		// TODO: What to do when no everything was read but errno is zero?
	}
	else {
		report_scan_success(disk, offset, data_size, t);
	}

	uint64_t t_msec = t / 1000000;
	unsigned hist_idx = 0;
	while (t_msec >= histogram_time[hist_idx] && hist_idx < ARRAY_SIZE(disk->histogram) - 1) {
		hist_idx++;
	}
	disk->histogram[hist_idx]++;

	latency_bucket_add(disk, t_msec, state);

	if (t_msec > 1000) {
		VERBOSE("Scanning at offset %" PRIu64 " took %"PRIu64" msec", offset, t_msec);
	}

	if (disk->fix && (t_msec > 3000 || error)) {
		INFO("Fixing region by rewriting, offset=%"PRIu64" size=%d", offset, data_size);
		ret = disk_dev_write(&disk->dev, offset, data_size, data);
		if (ret != data_size) {
			ERROR("Error while attempting to rewrite the data! ret=%zd errno=%d: %m", ret, errno);
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

static uint32_t *calc_scan_order_seq(disk_t *disk, uint64_t stride_size, int read_size_sectors)
{
	uint64_t num_reads = stride_size / read_size_sectors + 2;
	uint32_t *order = malloc(sizeof(uint32_t) * num_reads);

	uint64_t i;
	for (i = 0; i < num_reads-1; i++)
		order[i] = i * read_size_sectors * disk->sector_size;
	order[i] = UINT32_MAX;

	return order;
}

static uint32_t *calc_scan_order_random(disk_t *disk, uint64_t stride_size, int read_size_sectors)
{
	uint64_t num_reads = stride_size / read_size_sectors + 2;
	uint32_t *order = malloc(sizeof(uint32_t) * num_reads);

	// Fill sequential data
	uint64_t i;
	for (i = 0; i < num_reads - 1; i++)
		order[i] = i * read_size_sectors * disk->sector_size;
	order[i] = UINT32_MAX;

	// Shuffle it
	srand(time(NULL));
	for (i = 0; i < num_reads - 1; i++) {
		uint64_t j = rand() % num_reads;
		if (i == j)
			continue;

		uint32_t tmp = order[i];
		order[i] = order[j];
		order[j] = tmp;
	}

	return order;
}

static uint32_t *calc_scan_order(disk_t *disk, enum scan_mode mode, uint64_t stride_size, int read_size)
{
	int read_size_sectors = read_size / disk->sector_size;

	if (mode == SCAN_MODE_SEQ)
		return calc_scan_order_seq(disk, stride_size, read_size_sectors);
	else if (mode == SCAN_MODE_RANDOM)
		return calc_scan_order_random(disk, stride_size, read_size_sectors);
	else
		return NULL;
}

static void disk_scan_latency_stride(disk_t *disk, struct scan_state *state, uint64_t base_offset, uint64_t data_size, uint32_t *scan_order)
{
	unsigned i;

	for (i = 0; disk->run && scan_order[i] != UINT32_MAX; i++) {
		uint64_t offset = base_offset + scan_order[i];
		VVVERBOSE("Scanning at offset %"PRIu64" index %u", offset, i);
		uint64_t remainder = base_offset + state->latency_stride * disk->sector_size - offset;
		if (remainder < data_size) {
			data_size = remainder;
			VERBOSE("Last part scanning size %"PRIu64, data_size);
		}
		disk_scan_part(disk, offset, state->data, data_size, state);
	}
}

int disk_scan(disk_t *disk, enum scan_mode mode, unsigned data_size)
{
	disk->run = 1;
	void *data = allocate_buffer(data_size);
	uint32_t *scan_order = NULL;
	int result = 0;
	struct scan_state state = {.latency = NULL};
	struct timespec ts_start;
	struct timespec ts_end;

	if (data_size % disk->sector_size != 0) {
		data_size -= data_size % disk->sector_size;
		if (data_size == 0)
			data_size = disk->sector_size;
		ERROR("Cannot scan data not in multiples of the sector size, adjusted scan size to %u", data_size);
	}

	clock_gettime(CLOCK_MONOTONIC, &ts_start);

	INFO("Scanning disk %s in %u byte steps", disk->path, data_size);
	VVVERBOSE("Using buffer of size %d", data_size);

	if (data == NULL) {
		ERROR("Failed to allocate data buffer: %m");
		result = 1;
		goto Exit;
	}

	uint64_t offset;
	const uint64_t disk_size_bytes = disk->num_bytes;
	const uint64_t latency_stride = calc_latency_stride(disk);
	VVERBOSE("latency stride is %"PRIu64, latency_stride);

	state.latency_bucket = 0;
	state.latency_stride = latency_stride;
	state.latency_count = 0;
	state.latency = malloc(sizeof(uint32_t) * latency_stride);
	state.data = data;

	scan_order = calc_scan_order(disk, mode, latency_stride, data_size);
	if (!scan_order) {
		result = 1;
		ERROR("Failed to generate scan order");
		goto Exit;
	}

	for (offset = 0; disk->run && offset < disk_size_bytes; offset += latency_stride * disk->sector_size) {
		VVERBOSE("Scanning stride starting at %"PRIu64, offset);
		latency_bucket_prepare(disk, &state, offset);
		disk_scan_latency_stride(disk, &state, offset, data_size, scan_order);
		latency_bucket_finish(disk, &state, offset + latency_stride * disk->sector_size);
	}

	if (!disk->run) {
		INFO("Disk scan interrupted");
	}
	report_scan_done(disk);

Exit:
	free(scan_order);
	free_buffer(data, data_size);
	free(state.latency);
	disk->run = 0;
	clock_gettime(CLOCK_MONOTONIC, &ts_end);
	INFO("Scan took %d second", (int)(ts_end.tv_sec - ts_start.tv_sec));
	return result;
}
