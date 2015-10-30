#include "arch.h"

#include <sys/disk.h>
#include <sys/ioctl.h>
#include <sys/types.h>

#include "arch-posix.c"
#include <net/if.h>

int disk_dev_identify(disk_dev_t *dev, char *vendor, char *model, char *fw_rev, char *serial, bool *is_ata, unsigned char *ata_buf, unsigned *ata_buf_len)
{
	(void)dev;
	strcpy(vendor, "UNKNOWN");
	strcpy(model, "UNKNOWN");
	strcpy(fw_rev, "UNKN");
	strcpy(serial, "UNKNOWN");
	*is_ata = 0;
	*ata_buf_len = 0;
	*ata_buf = 0;
	return 0;
}

int disk_dev_read_cap(disk_dev_t *dev, uint64_t *size_bytes, uint64_t *sector_size)
{
	if (ioctl(dev->fd, DIOCGMEDIASIZE, size_bytes) < 0) {
		return -1;
	}

	if (ioctl(dev->fd, DIOCGSECTORSIZE, sector_size) < 0) {
		return -1;
	}

	return 0;
}

void mac_read(unsigned char *buf, int len)
{
	struct ifreq ifr;
	struct ifconf ifc;
	char data[1024];
	int success = 0;

	buf[0] = 0;

	int sock = socket(AF_INET, SOCK_DGRAM, 0);
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
				if (ioctl(sock, SIOCGIFMAC, &ifr) == 0) {
					success = 1;
					break;
				}
			}
		}
		else { /* handle error */ }
	}

	if (success) {
		memcpy(buf, ifr.ifr_ifru.ifru_data, len >= 6 ? 6 : len);
	} else {
		memset(buf, 0, len);
	}

Exit:
	close(sock);
}
