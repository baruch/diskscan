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
};

void print_header(void)
{
	printf("diskscan version %s\n\n", TAG);
	VERBOSE("Verbosity set");
	VVERBOSE("High verbosity set");
	VVVERBOSE("Very high verbosity set");
}

int usage(void) {
	printf("diskscan version %s\n\n", TAG);
	printf("diskscan [options] /dev/sd\n");
	printf("Options:\n");
	printf("    -v, --verbose   - Increase verbosity, multiple uses for higher levels\n");
	printf("\n");
	return 1;
}


void report_scan_success(disk_t *disk, uint64_t offset_bytes, uint64_t data_size, uint64_t time)
{
}

void report_scan_error(disk_t *disk, uint64_t offset_bytes, uint64_t data_size, uint64_t time)
{
}

void report_scan_done(disk_t *disk)
{
	int hist_idx;
	
	printf("Access time histogram:\n");
	for (hist_idx = 0; hist_idx < ARRAY_SIZE(disk->histogram); hist_idx++)
	{
		if (hist_idx != ARRAY_SIZE(disk->histogram)-1)
			printf("%8" PRIu64 ": %" PRIu64 "\n", histogram_time[hist_idx], disk->histogram[hist_idx]);
		else
			printf("%8s: %" PRIu64 "\n", "above that", disk->histogram[hist_idx]);
	}
}

int parse_args(int argc, char **argv, options_t *opts)
{
	int c;
	int unknown = 0;

	while (1) {
		int option_index = 0;
		static struct option long_options[] = {
			{"verbose", no_argument, 0,  'v'},
			{0,         0,           0,  0}
		};

		c = getopt_long(argc, argv, "vr", long_options, &option_index);
		if (c == -1)
			break;

		switch (c) {
			case 0:
				unknown = 1;
				break;
			case 'v':
				opts->verbose++;
				break;

			default:
				printf("unknown arg\n");
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

int print_disk_info(disk_t *disk)
{
	return 0;
}

static void diskscan_cli_signal(int signal)
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
	options_t opts = {0};

	if (parse_args(argc, argv, &opts))
		return 1;
	verbose = opts.verbose;

	print_header();

	setup_signals();

	if (disk_open(&disk, opts.disk_path))
		return 1;

	if (print_disk_info(&disk))
		return 1;

	if (disk_scan(&disk))
		return 1;

	disk_close(&disk);
	return 0;
}
