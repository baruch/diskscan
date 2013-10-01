#ifndef _DISKSCAN_H_
#define _DISKSCAN_H_

#include <stdint.h>

#define ARRAY_SIZE(a) (sizeof(a)/sizeof(a[0]))

static const uint64_t histogram_time[] = {1, 10, 100, 500, 1000, 2000, 3000, 4000, 5000, 6000, 7000, 8000, 9000, 10000, 15000, 20000, 25000, 30000, UINT64_MAX};

typedef struct latency_t {
	uint64_t start_sector;
	uint64_t end_sector;
	uint32_t latency_min_msec;
	uint32_t latency_max_msec;
	uint32_t latency_median_msec;
} latency_t;

typedef struct disk_t {
	int fd;
	char path[128];
	char vendor[32];
	char model[32];
	char serial[32];
	uint64_t num_bytes;
	uint64_t sector_size;
	int run;

	uint64_t num_errors;
	uint64_t histogram[ARRAY_SIZE(histogram_time)];
	unsigned latency_graph_len;
	latency_t *latency_graph;
} disk_t;

int disk_open(disk_t *disk, const char *path, int fix, unsigned latency_graph_len);
int disk_scan(disk_t *disk);
int disk_close(disk_t *disk);
void disk_scan_stop(disk_t *disk);

/* Implemented by the user (gui/cli) */
void report_scan_success(disk_t *disk, uint64_t offset_bytes, uint64_t data_size, uint64_t time);
void report_scan_error(disk_t *disk, uint64_t offset_bytes, uint64_t data_size, uint64_t time);
void report_scan_done(disk_t *disk);

#endif
