#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <memory.h>

bool disk_dev_open(disk_dev_t *dev, const char *path)
{
	dev->fd = open(path, O_RDWR|O_DIRECT);
	return dev->fd >= 0;
}

void disk_dev_close(disk_dev_t *dev)
{
	close(dev->fd);
	dev->fd = -1;
}

ssize_t disk_dev_read(disk_dev_t *dev, uint64_t offset_bytes, uint32_t len_bytes, void *buf, io_result_t *io_res)
{
	ssize_t ret = pread(dev->fd, buf, len_bytes, offset_bytes);
	if (ret == len_bytes) {
		io_res->data = DATA_FULL;
		io_res->error = ERROR_NONE;
		return ret;
	} else if (ret > 0) {
		io_res->data = DATA_PARTIAL;
		io_res->error = ERROR_NONE;
		return ret;
	} else if (ret == 0) {
		io_res->data = DATA_NONE;
		io_res->error = ERROR_NONE;
		return ret;
	} else {
		// ret < 0, i.e. error
		io_res->data = DATA_NONE;
		io_res->error = ERROR_UNCORRECTED;
		io_res->sense_len = 0;
		memset(&io_res->info, 0, sizeof(io_res->info));
		return -1;
	}

	//TODO: Handle EINTR with a retry
}

ssize_t disk_dev_write(disk_dev_t *dev, uint64_t offset_bytes, uint32_t len_bytes, void *buf, io_result_t *io_res)
{
	ssize_t ret = pwrite(dev->fd, buf, len_bytes, offset_bytes);
	if (ret == len_bytes) {
		io_res->data = DATA_FULL;
		io_res->error = ERROR_NONE;
		return ret;
	} else if (ret > 0) {
		io_res->data = DATA_PARTIAL;
		io_res->error = ERROR_NONE;
		return ret;
	} else if (ret == 0) {
		io_res->data = DATA_NONE;
		io_res->error = ERROR_NONE;
		return ret;
	} else {
		// ret < 0, i.e. error
		io_res->data = DATA_NONE;
		io_res->error = ERROR_UNCORRECTED;
		io_res->sense_len = 0;
		memset(&io_res->info, 0, sizeof(io_res->info));
		return -1;
	}

	//TODO: Handle EINTR with a retry
}
