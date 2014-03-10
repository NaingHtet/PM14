#include <stdio.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <fcntl.h>
#include <termios.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>

#include "i2c_controller.h"

int open_i2c_port() {
	i2c_fd = open( I2C_PORTNAME, O_RDWR);
	if ( i2c_fd < 0 ) {
		printf("Error Opening i2c port");
		exit(1);
	}
}

int set_i2c_address(int _addr) {
	addr = 0x01;
	int io = ioctl(i2c_fd, I2C_SLAVE, addr);
	if (io < 0) {
		printf("Error setting i2c address");
		exit(1);
	}
}

void rdwr_i2c(char cmd, char* response, int no_rd_bytes) {
	char wr_buf[1];
	wr_buf[0] = cmd;
	//wr_buf[1] = cmd;

	write_i2c( wr_buf, 1);
	read_i2c(response, no_rd_bytes); 
}

void write_i2c(char* buf, int no_wr_bytes) {
	int n = write(i2c_fd, buf, no_wr_bytes);
	if (n < 0) {
		printf("Error writing = %s\n", strerror(errno));
		exit(1);
	}
}

void read_i2c(char* rd_buf, int no_rd_bytes) {
	int n = read(i2c_fd, rd_buf, no_rd_bytes);
	if (n < 0) {
		printf("Error reading = %s\n", strerror( errno));
		exit(1);
	}
}

double get_voltage() {
	char v_str[VOLTAGE_BYTES];
	rdwr_i2c(VOLTAGE_CMD, v_str, VOLTAGE_BYTES);

	uint16_t v_int = ((uint16_t)v_str[0] << 8) + v_str[1];
	printf("%x\n", v_int);
	printf("double = %f\n", v_int);
	return (double)v_int;
}

