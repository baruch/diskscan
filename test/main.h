#ifndef LIBSCSICMD_TEST_H
#define LIBSCSICMD_TEST_H

#include <stdio.h>

extern int debug;

/** Do the command that we want to test on the open disk interface. */
void do_command(int fd);
bool submit_cmd(int fd, unsigned char *cdb, unsigned cdb_len, unsigned char *buf, unsigned buf_len, int dxfer_dir);
bool read_response_buf(int fd, unsigned char **sense, unsigned *sense_len, unsigned *buf_read);

static inline bool read_response(int fd, unsigned char **sense, unsigned *sense_len)
{
	return read_response_buf(fd, sense, sense_len, NULL);
}

#endif
