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

#include "verbose.h"
#include "diskscan.h"
#include "version.h"
#include "compiler.h"
#include "cli.h"

#include <stdio.h>
#include <signal.h>
#include <inttypes.h>
#include <unistd.h>
#include <getopt.h>

static disk_t disk;

typedef struct options_t options_t;
struct options_t {
	char *disk_path;
	int verbose;
	int fix;
	enum scan_mode mode;
};

static void print_header(void)
{
	printf("diskscan version %s\n\n", TAG);
	VERBOSE("Verbosity set");
	VVERBOSE("High verbosity set");
	VVVERBOSE("Very high verbosity set");
}

static int usage(void) {
	printf("diskscan version %s\n\n", TAG);
	printf("diskscan [options] /dev/sd\n");
	printf("Options:\n");
	printf("    -v, --verbose     - Increase verbosity, multiple uses for higher levels\n");
	printf("    -f, --fix         - Attempt to fix near failures, nothing can be done for unreadable sectors\n");
	printf("    -s, --scan <mode> - Scan in order (seq, random)\n");
	printf("\n");
	return 1;
}


void report_scan_success(disk_t *UNUSED(disk), uint64_t UNUSED(offset_bytes), uint64_t UNUSED(data_size), uint64_t UNUSED(time))
{
}

void report_scan_error(disk_t *UNUSED(disk), uint64_t UNUSED(offset_bytes), uint64_t UNUSED(data_size), uint64_t UNUSED(time))
{
}

static void print_latency(latency_t *latency_graph, unsigned latency_graph_len)
{
	unsigned i;

	const uint32_t height = 30; // number of lines to fill
	const uint32_t min_val = 0;
	uint32_t max_val = 1;

	for (i = 0; i < latency_graph_len; i++) {
		if (max_val < latency_graph[i].latency_max_msec)
			max_val = latency_graph[i].latency_max_msec;
	}

	uint32_t height_interval = (max_val - min_val + 1) / (height - 3);
	if (height_interval == 0)
		height_interval = 1;
	else if (height_interval > 10000)
		height_interval = 10000;

	uint32_t j;
	for (j = height; j > 0; j--) {
		if (j % 5 == 0)
			printf("%5u | ", j * height_interval);
		else
			printf("      | ");

		for (i = 0; i < latency_graph_len; i++) {
			uint32_t max_height = latency_graph[i].latency_max_msec / height_interval + 1;
			uint32_t med_height = latency_graph[i].latency_median_msec / height_interval + 1;
			uint32_t min_height = latency_graph[i].latency_min_msec / height_interval + 1;

			if (max_height == med_height) {
				max_height++;
			}
			if (med_height == min_height) {
				med_height++;
				if (max_height == med_height)
					max_height++;
			}

			if (max_height != j && med_height != j && min_height != j) {
				printf(" ");
				continue;
			}

			if (max_height == j)
				printf("^");
			else if (med_height == j)
				printf("*");
			else
				printf("_");
		}
		printf("\n");
	}
	printf("      +-");
	for (i = 0; i < latency_graph_len; i++) {
		printf("-");
	}
	printf("\n");

}

void report_scan_done(disk_t *disk)
{
	unsigned hist_idx;
	
	printf("Access time histogram:\n");
	for (hist_idx = 0; hist_idx < ARRAY_SIZE(disk->histogram); hist_idx++)
	{
		if (hist_idx != ARRAY_SIZE(disk->histogram)-1)
			printf("%8" PRIu64 ": %" PRIu64 "\n", histogram_time[hist_idx], disk->histogram[hist_idx]);
		else
			printf("%8s: %" PRIu64 "\n", "above that", disk->histogram[hist_idx]);
	}

	print_latency(disk->latency_graph, disk->latency_graph_len);
}

static int parse_args(int argc, char **argv, options_t *opts)
{
	int c;
	int unknown = 0;

	while (1) {
		int option_index = 0;
		static struct option long_options[] = {
			{"verbose", no_argument,       0,  'v'},
			{"fix",     no_argument,       0,  'f'},
			{"scan",    required_argument, 0,  's'},
			{0,         0,                 0,  0}
		};

		c = getopt_long(argc, argv, "vfs:", long_options, &option_index);
		if (c == -1)
			break;

		switch (c) {
			case 0:
				break;

			case 'v':
				opts->verbose++;
				break;
			case 'f':
				opts->fix = 1;
				break;
			case 's':
				opts->mode = str_to_scan_mode(optarg);
				if (opts->mode == SCAN_MODE_UNKNOWN) {
					opts->mode = SCAN_MODE_SEQ;
					printf("Unknown scan mode %s given, using sequential\n", optarg);
				}
				break;

			default:
				unknown = 1;
				break;
		}
	}

	if (optind == argc) {
		printf("No disk path provided to scan!\n");
		return usage();
	}
	if (optind < argc - 1) {
		printf("Too many disk paths provided to scan, can only scan one disk!\n");
		return usage();
	}

	if (unknown) {
		printf("Unknown option provided");
		return usage();
	}

	opts->disk_path = argv[optind];
	return 0;
}

static int print_disk_info(disk_t *UNUSED(disk))
{
	return 0;
}

static void diskscan_cli_signal(int UNUSED(signal))
{
	disk_scan_stop(&disk);
}

static void setup_signals(void)
{
	struct sigaction act = {
		.sa_handler = diskscan_cli_signal,
		.sa_flags = SA_RESTART,
	};

	sigaction(SIGINT, &act, NULL);
	sigaction(SIGTERM, &act, NULL);
}

int diskscan_cli(int argc, char **argv)
{
	options_t opts;
	memset(&opts, 0, sizeof(opts));

	if (parse_args(argc, argv, &opts))
		return 1;
	verbose = opts.verbose;

	print_header();

	setup_signals();

	if (disk_open(&disk, opts.disk_path, opts.fix, 70))
		return 1;

	if (print_disk_info(&disk))
		return 1;

	if (disk_scan(&disk, opts.mode))
		return 1;

	disk_close(&disk);
	return 0;
}
