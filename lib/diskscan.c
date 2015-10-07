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
#include "disk.h"
#include "arch.h"
#include "median.h"
#include "compiler.h"
#include "data.h"
#include "libscsicmd/include/smartdb.h"
#include "libscsicmd/include/ata_smart.h"

#include <sched.h>
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

#define TEMP_THRESHOLD 65

struct scan_state {
	uint32_t latency_bucket;
	uint64_t latency_stride;
	uint32_t latency_count;
	uint32_t *latency;
	void *data;
	uint64_t progress_bytes;
	int progress_part;
	int progress_full;
	unsigned num_unknown_errors;
};

typedef int spinner_t;

static char spinner_form[] = {'|', '/', '-', '\\', '|', '/', '-', '\\'};

static void spinner_init(spinner_t *spinner)
{
	printf("%c\r", spinner_form[0]);
	*spinner = 1;
	fflush(stdout);
}

static void spinner_update(spinner_t *spinner)
{
	printf("\r%c\r", spinner_form[*spinner]);
	if (++(*spinner) == ARRAY_SIZE(spinner_form))
		*spinner = 0;
	fflush(stdout);
}

static void spinner_done(void)
{
	printf("\r                           \r");
	fflush(stdout);
}

const char *conclusion_to_str(enum conclusion conclusion)
{
	switch (conclusion) {
		case CONCLUSION_FAILED: return "failed";
		case CONCLUSION_PASSED: return "passed";
		case CONCLUSION_SCAN_PROBLEM: return "scan_problem";
		case CONCLUSION_ABORTED: return "scan_aborted";
	}

	return "unknown";
}

enum scan_mode str_to_scan_mode(const char *s)
{
	if (strcasecmp(s, "seq") == 0 || strcasecmp(s, "sequential") == 0)
		return SCAN_MODE_SEQ;
	if (strcasecmp(s, "random") == 0)
		return SCAN_MODE_RANDOM;
	return SCAN_MODE_UNKNOWN;
}

static void disk_ata_monitor_start(disk_t *disk)
{
	if (disk_smart_trip(&disk->dev) == 1) {
		ERROR("Disk has a SMART TRIP at the start of the test, it should be discarded anyhow");
		disk->state.ata.is_smart_tripped = true;
	} else {
		disk->state.ata.is_smart_tripped = false;
	}

	disk->state.ata.smart_table = smart_table_for_disk(disk->vendor, disk->model, disk->fw_rev);
	if (disk->state.ata.smart_table == NULL)
		ERROR("BUG! Failed to setup smart table for the disk.");

	disk->state.ata.smart_num = disk_smart_attributes(&disk->dev, disk->state.ata.smart, ARRAY_SIZE(disk->state.ata.smart));

	if (disk->state.ata.smart_num > 0) {
		int min_temp = -1;
		int max_temp = -1;
		int temp = ata_smart_get_temperature(disk->state.ata.smart, disk->state.ata.smart_num, disk->state.ata.smart_table, &min_temp, &max_temp);
		disk->state.ata.last_temp = temp;

		if (min_temp > 0 || max_temp > 0)
			INFO("Disk start temperature is %d (lifetime min %d and lifetime max %d)", temp, min_temp, max_temp);
		else
			INFO("Disk start temperature is %d", temp);
	} else {
		ERROR("Failed to read SMART attributes from device");
	}
}

static void ata_test_temp(disk_t *disk, ata_smart_attr_t *smart, int smart_num)
{
	int min_temp = -1;
	int max_temp = -1;
	int temp = ata_smart_get_temperature(smart, smart_num, disk->state.ata.smart_table, &min_temp, &max_temp);

	if (temp != disk->state.ata.last_temp) {
		INFO("Disk temperature changed from %d to %d", disk->state.ata.last_temp, temp);
		disk->state.ata.last_temp = temp;
	}

	if (temp >= TEMP_THRESHOLD) {
		spinner_t spinner;
		INFO("Pausing scan due to high disk temperature");
		spinner_init(&spinner);
		while (temp >= TEMP_THRESHOLD) {
			sleep(1);
			spinner_update(&spinner);
			smart_num = disk_smart_attributes(&disk->dev, smart, ARRAY_SIZE(smart));
			if (smart_num > 0) {
				temp = ata_smart_get_temperature(smart, smart_num, disk->state.ata.smart_table, &min_temp, &max_temp);
			} else {
				ERROR("Failed to read temperature while paused!");
				break;
			}
		}
		spinner_done();
		INFO("Finished pause, temperature is now %d", temp);
	}
}

static void disk_ata_monitor(disk_t *disk)
{
	ata_smart_attr_t smart[MAX_SMART_ATTRS];
	int smart_num;

	if (!disk->state.ata.is_smart_tripped && disk_smart_trip(&disk->dev) == 1) {
		ERROR("Disk has a SMART TRIP in the middle of the test, it should be discarded!");
		disk->state.ata.is_smart_tripped = true;
	}

	smart_num = disk_smart_attributes(&disk->dev, smart, ARRAY_SIZE(smart));

	if (smart_num > 0) {
		ata_test_temp(disk, smart, smart_num);
	} else {
		ERROR("Failed to read SMART attributes from device");
	}
}

static void disk_ata_monitor_end(disk_t *disk)
{
	if (disk_smart_trip(&disk->dev) == 1) {
		ERROR("Disk has a SMART TRIP at the end of the test, it should be discarded!");
	} else if (disk->state.ata.is_smart_tripped) {
		ERROR("Disk had a SMART TRIP during the test but it disappeared. This is super weird!!!");
	}
}

static void disk_scsi_monitor_start(disk_t *disk)
{
	(void)disk;
}

static void disk_scsi_monitor(disk_t *disk)
{
	(void)disk;
}

static void disk_scsi_monitor_end(disk_t *disk)
{
	(void)disk;
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

	if (disk_dev_identify(&disk->dev, disk->vendor, disk->model, disk->fw_rev, disk->serial, &disk->is_ata, disk->ata_buf, &disk->ata_buf_len) < 0) {
		ERROR("Can't identify disk for path %s: %m", path);
		goto Error;
	}

	strncpy(disk->path, path, sizeof(disk->path));
	disk->path[sizeof(disk->path)-1] = 0;

	disk->latency_graph_len = latency_graph_len;
	disk->latency_graph = calloc(latency_graph_len, sizeof(latency_t));
	if (disk->latency_graph == NULL) {
		ERROR("Failed to allocate memory for latency graph data");
		goto Error;
	}

	if (disk->is_ata)
		disk_ata_monitor_start(disk);
	else
		disk_scsi_monitor_start(disk);

	INFO("Opened disk %s sector size %"PRIu64" num bytes %"PRIu64, path, disk->sector_size, disk->num_bytes);
	return 0;

Error:
	disk_close(disk);
	return 1;
}

int disk_close(disk_t *disk)
{
	if (disk->is_ata)
		disk_ata_monitor_end(disk);
	else
		disk_scsi_monitor_end(disk);

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

static const char *error_to_str(enum result_error_e err)
{
	switch (err)
	{
		case ERROR_NONE: return "none";
		case ERROR_CORRECTED: return "corrected";
		case ERROR_UNCORRECTED: return "uncorrected";
		case ERROR_NEED_RETRY: return "need_retry";
		case ERROR_FATAL: return "fatal";
		case ERROR_UNKNOWN: return "unknown";
	}

	return "unknown";
}

static const char *data_to_str(enum result_data_e data)
{
	switch (data)
	{
		case DATA_FULL: return "full";
		case DATA_PARTIAL: return "partial";
		case DATA_NONE: return "none";
	}

	return "unknown";
}

static bool disk_scan_part(disk_t *disk, uint64_t offset, void *data, int data_size, struct scan_state *state)
{
	ssize_t ret;
	struct timespec t_start;
	struct timespec t_end;
	uint64_t t;
	int error = 0;
	io_result_t io_res;

	clock_gettime(CLOCK_MONOTONIC, &t_start);
	ret = disk_dev_read(&disk->dev, offset, data_size, data, &io_res);
	clock_gettime(CLOCK_MONOTONIC, &t_end);

	t = (t_end.tv_sec - t_start.tv_sec) * 1000000000 +
		t_end.tv_nsec - t_start.tv_nsec;
	const uint64_t t_msec = t / 1000000;

	// Perform logging
	data_log_raw(&disk->data_raw, offset/disk->sector_size, data_size/disk->sector_size, &io_res, t);
	data_log(&disk->data_log, offset/disk->sector_size, data_size/disk->sector_size, &io_res, t);

	// Handle error or incomplete data
	if (io_res.data != DATA_FULL || io_res.error != ERROR_NONE) {
		int s_errno = errno;
		ERROR("Error when reading at offset %" PRIu64 " size %d read %zd: %m", offset, data_size, ret);
		ERROR("Details: error=%s data=%s %02X/%02X/%02X", error_to_str(io_res.error), data_to_str(io_res.data),
				io_res.info.sense_key, io_res.info.asc, io_res.info.ascq);
		report_scan_error(disk, offset, data_size, t);
		disk->num_errors++;
		error = 1;
		if (io_res.error == ERROR_FATAL) {
			ERROR("Fatal error occurred, bailing out.");
			return false;
		}
		if (io_res.error == ERROR_UNKNOWN) {
			if (state->num_unknown_errors++ > 500) {
				ERROR("%u unknown errors occurred, assuming fatal issue.", state->num_unknown_errors);
				return false;
			}
			ERROR("Unknown error occurred, possibly untranslated error by storage layers, trying to continue.");

		}

		if (s_errno != EIO && s_errno != 0)
			abort();
		// TODO: What to do when no everything was read but errno is zero?
	}
	else {
		state->num_unknown_errors = 0; // Clear non-consecutive unknown errors
		report_scan_success(disk, offset, data_size, t);
	}

	unsigned hist_idx = 0;
	while (t_msec >= histogram_time[hist_idx].top_val && hist_idx < ARRAY_SIZE(disk->histogram) - 1) {
		hist_idx++;
	}
	disk->histogram[hist_idx]++;

	latency_bucket_add(disk, t_msec, state);

	if (t_msec > 1000) {
		VERBOSE("Scanning at offset %" PRIu64 " took %"PRIu64" msec", offset, t_msec);
	}

	if (disk->fix && (t_msec > 3000 || error)) {
		INFO("Fixing region by rewriting, offset=%"PRIu64" size=%d", offset, data_size);
		ret = disk_dev_write(&disk->dev, offset, data_size, data, &io_res);
		if (ret != data_size) {
			ERROR("Error while attempting to rewrite the data! ret=%zd errno=%d: %m", ret, errno);
		}
	}

	return true;
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

static void progress_calc(disk_t *disk, struct scan_state *state, uint64_t add)
{
	bool do_update;

	if (add != 0) {
		state->progress_bytes += add;
		int progress_part_new = state->progress_bytes * state->progress_full / disk->num_bytes;
		do_update = progress_part_new != state->progress_part;
		state->progress_part = progress_part_new;
	} else {
		do_update = true;
	}

	if (do_update) {
		report_progress(disk, state->progress_part, state->progress_full);
	}
}

static bool disk_scan_latency_stride(disk_t *disk, struct scan_state *state, uint64_t base_offset, uint64_t data_size, uint32_t *scan_order)
{
	unsigned i;

	for (i = 0; disk->run && scan_order[i] != UINT32_MAX; i++) {
		uint64_t offset = base_offset + scan_order[i];

		progress_calc(disk, state, data_size);

		VVVERBOSE("Scanning at offset %"PRIu64" index %u", offset, i);
		uint64_t stride_end = base_offset + state->latency_stride * disk->sector_size;
		if (stride_end > disk->num_bytes)
			stride_end = disk->num_bytes;
		uint64_t remainder = stride_end - offset;
		if (remainder < data_size) {
			data_size = remainder;
			VERBOSE("Last part scanning size %"PRIu64, data_size);
		}
		if (!disk_scan_part(disk, offset, state->data, data_size, state))
			return false;
	}

	return true;
}

static void set_realtime(bool realtime)
{
	struct sched_param param;
	memset(&param, 0, sizeof(param));
	param.sched_priority = 1;

	if (realtime)
		sched_setscheduler(0, SCHED_RR, &param);
	else
		sched_setscheduler(0, SCHED_OTHER, &param);
}

static enum conclusion conclusion_calc(disk_t *disk)
{
	unsigned i;
	uint64_t total;

	if (disk->num_errors > 0)
		return CONCLUSION_FAILED;

	for (i = 0, total = 0; i < ARRAY_SIZE(histogram_time); i++) {
		total += disk->histogram[i];
	}

	const double total_d = (double)total;
	for (i = 0; i < ARRAY_SIZE(histogram_time); i++) {
		double percent = ((double)disk->histogram[i]) / total_d;
		if (percent > histogram_time[i].percent_bad) {
			const uint64_t low_end = i == 0 ? 0 : histogram_time[i-1].top_val;
			const uint64_t high_end = histogram_time[i].top_val;
			ERROR("Disk has %"PRIu64" (%f%%) IOs from %"PRIu64" to %"PRIu64" msecs, it is a bad disk!", disk->histogram[i], percent, low_end, high_end);
			return CONCLUSION_FAILED;
		}
	}

	VERBOSE("Disk has passed the test");
	return CONCLUSION_PASSED;
}

int disk_scan(disk_t *disk, enum scan_mode mode, unsigned data_size)
{
	disk->run = 1;
	void *data = allocate_buffer(data_size);
	uint32_t *scan_order = NULL;
	int result = 0;
	struct scan_state state = {.latency = NULL, .progress_bytes = 0, .progress_full = 1000};
	struct timespec ts_start;
	struct timespec ts_end;
	time_t scan_time;

	disk->conclusion = CONCLUSION_SCAN_PROBLEM;

	if (data_size % disk->sector_size != 0) {
		data_size -= data_size % disk->sector_size;
		if (data_size == 0)
			data_size = disk->sector_size;
		ERROR("Cannot scan data not in multiples of the sector size, adjusted scan size to %u", data_size);
	}

	set_realtime(true);
	clock_gettime(CLOCK_MONOTONIC, &ts_start);

	INFO("Scanning disk %s in %u byte steps", disk->path, data_size);
	scan_time = time(NULL);
	INFO("Scan started at: %s", ctime(&scan_time));
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
		VERBOSE("Scanning stride starting at %"PRIu64" done %"PRIu64"%%", offset, offset*100/disk_size_bytes);
		progress_calc(disk, &state, 0);
		latency_bucket_prepare(disk, &state, offset);
		if (!disk_scan_latency_stride(disk, &state, offset, data_size, scan_order))
			break;
		latency_bucket_finish(disk, &state, offset + latency_stride * disk->sector_size);

		if (disk->is_ata)
			disk_ata_monitor(disk);
		else
			disk_scsi_monitor(disk);
	}

	if (!disk->run) {
		INFO("Disk scan interrupted");
		disk->conclusion = CONCLUSION_ABORTED;
	} else {
		disk->conclusion = conclusion_calc(disk);
	}
	report_scan_done(disk);

Exit:
	clock_gettime(CLOCK_MONOTONIC, &ts_end);
	set_realtime(false);
	free(scan_order);
	free_buffer(data, data_size);
	free(state.latency);
	disk->run = 0;
	scan_time = time(NULL);
	INFO("Scan ended at: %s", ctime(&scan_time));
	INFO("Scan took %d second", (int)(ts_end.tv_sec - ts_start.tv_sec));
	return result;
}
