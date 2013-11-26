#include "system_id.h"

#include "sha1.h"

#include <memory.h>
#include <stdio.h>
#include <assert.h>
#include <sys/ioctl.h>
#include <net/if.h> 
#include <unistd.h>
#include <netinet/in.h>
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

static void mac_read(char *buf, int len)
{
	struct ifreq ifr;
	struct ifconf ifc;
	char data[1024];
	int success = 0;

	buf[0] = 0;

	int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
	if (sock == -1) {
		return;
	};

	ifc.ifc_len = sizeof(data);
	ifc.ifc_buf = data;
	if (ioctl(sock, SIOCGIFCONF, &ifc) == -1) {
		/* handle error */
		goto Exit;
	}

	struct ifreq* it = ifc.ifc_req;
	const struct ifreq* const end = it + (ifc.ifc_len / sizeof(struct ifreq));

	for (; it != end; ++it) {
		strcpy(ifr.ifr_name, it->ifr_name);
		if (ioctl(sock, SIOCGIFFLAGS, &ifr) == 0) {
			if (! (ifr.ifr_flags & IFF_LOOPBACK)) { // don't count loopback
				if (ioctl(sock, SIOCGIFHWADDR, &ifr) == 0) {
					success = 1;
					break;
				}
			}
		}
		else { /* handle error */ }
	}

	if (success) {
		sha1_calc((unsigned char*)ifr.ifr_hwaddr.sa_data, 6, buf, len);
	} else {
		memset(buf, 0, len);
	}

Exit:
	close(sock);
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
	mac_read(system_id->mac, sizeof(system_id->mac));
	return true;
}
