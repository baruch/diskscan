#include "system_id.h"

#include "sha1.h"
#include "arch.h"

#include <memory.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

static void sha1_calc(const unsigned char *src, int src_len, char *out, int out_size)
{
	assert(out_size >= SHA1_DIGEST_SIZE*2+1);

	unsigned char digest[SHA1_DIGEST_SIZE];
	SHA1_CTX sha1_ctx;
	SHA1_Init(&sha1_ctx);
	SHA1_Update(&sha1_ctx, src, src_len);
	SHA1_Final(&sha1_ctx, digest);

	int i;
	for (i = 0; i < SHA1_DIGEST_SIZE; i++) {
		sprintf(out + i*2, "%02X", digest[i]);
	}
}

static bool cmd_str(const char *cmd, char *buf, int len)
{
	FILE *f = popen(cmd, "r");
	if (!f)
		return false;

	char *ret = fgets(buf, len, f);
	pclose(f);
	return ret != NULL;
}

static void dmidecode_read(const char *field_name, char *buf, int len)
{
	char cmd[128];

	memset(buf, 0, len);

	snprintf(cmd, sizeof(cmd), "dmidecode -s %s", field_name);
	if (!cmd_str(cmd, buf, len))
		return;

	int actual_len = strlen(buf);
	switch (buf[actual_len-1]) {
		case '\r':
		case '\n':
			buf[actual_len-1] = 0;
			actual_len--;
			break;
	}

	sha1_calc((unsigned char *)buf, actual_len, buf, len);
}

static void system_serial_read(char *buf, int len)
{
	dmidecode_read("system-serial-number", buf, len);
}

static void chassis_serial_read(char *buf, int len)
{
	dmidecode_read("chassis-serial-number", buf, len);
}

static void baseboard_serial_read(char *buf, int len)
{
	dmidecode_read("baseboard-serial-number", buf, len);
}

static void os_read(char *buf, int len)
{
	cmd_str("uname -o", buf, len);

	int i;
	for (i = strlen(buf) - 1; i >= 0; i--) {
		if (!isspace(buf[i]))
			break;
		buf[i] = 0;
	}
}

bool system_identifier_read(system_identifier_t *system_id)
{
	os_read(system_id->os, sizeof(system_id->os));
	system_serial_read(system_id->system, sizeof(system_id->system));
	chassis_serial_read(system_id->chassis, sizeof(system_id->chassis));
	baseboard_serial_read(system_id->baseboard, sizeof(system_id->baseboard));

	unsigned char mac[6];
	mac_read(mac, sizeof(mac));
	sha1_calc(mac, sizeof(mac), system_id->mac, sizeof(system_id->mac));

	return true;
}
