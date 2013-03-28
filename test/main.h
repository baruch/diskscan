#ifndef LIBSCSICMD_TEST_H
#define LIBSCSICMD_TEST_H

/** Do the command that we want to test on the open disk interface. */
void do_command(int fd);
bool submit_cmd(int fd, unsigned char *cdb, unsigned cdb_len, unsigned char *buf, unsigned buf_len, int dxfer_dir);
bool read_response(int fd, unsigned char **sense, unsigned *sense_len);

#endif
