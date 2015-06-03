#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdio.h>

#define DO_SNAPSHOT 1
#define MAJOR_NUM 100
#define REQUEST _IO(MAJOR_NUM, 1)

int main(void) {
	int fd;
	int ret = 0;

	fd = open("/dev/ram0", O_RDWR);
	ret = ioctl(fd, REQUEST, DO_SNAPSHOT);
	if(ret == 0)
		printf("ioctl successfully sent\n");
	close(fd);

	return 0;
}
