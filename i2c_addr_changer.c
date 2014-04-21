#include <stdio.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <fcntl.h>
#include <termios.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>
static const char I2C_PORTNAME[] = "/dev/i2c-0";

int main(int argc, char **argv) {
	int i2c_fd;

	if (argc != 3) {
		printf("Usage: ./i2c_addr_changer current_addr addr\n");
		printf("Note: type '1' for '0x01' etc\n");
		exit(1);
	}

	i2c_fd = open( I2C_PORTNAME, O_RDWR);
	if ( i2c_fd < 0 ) {
		printf("Error Opening i2c port");
		exit(1);
	}
	int addr = atoi(argv[1]);
	int c_addr = atoi(argv[2]);
	c_addr = c_addr*2;

	int io = ioctl(i2c_fd, I2C_SLAVE, addr);
	if (io < 0) {
		printf("Error setting i2c address");
		exit(1);
	}
	usleep(100);

	char buf[3];
	buf[0] = 0x01;
	buf[1] = 0x00;
	buf[2] = (char)c_addr;

	int n = write(i2c_fd, buf, 3);
	if (n < 0) {
		printf("Error writing = %s\n", strerror(errno));
		exit(1);
	}

	close(i2c_fd);


}