#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdio.h>

#define DO_ROLLBACK 2
#define MAJOR_NUM 101
#define REQUEST _IO(MAJOR_NUM, 1)

int main(void) {
	int fd;
	int ret = 0;

	fd = open("/dev/ram0", O_RDWR);
	ret = ioctl(fd, REQUEST, DO_ROLLBACK);
	if(ret == 0)
		printf("ioctl successfully sent\n");
	close(fd);

	return 0;
}
