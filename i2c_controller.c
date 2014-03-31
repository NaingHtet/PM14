#include <stdio.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <fcntl.h>
#include <termios.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>

#include "i2c_controller.h"

//Open the i2c port
int open_i2c_port() {
	i2c_fd = open( I2C_PORTNAME, O_RDWR);
	if ( i2c_fd < 0 ) {
		printf("Error Opening i2c port");
		exit(1);
	}
}

//Set the address of the i2c device we want to communicate
int set_i2c_address(int _addr) {
	addr = 0x03;
	int io = ioctl(i2c_fd, I2C_SLAVE, addr);
	if (io < 0) {
		printf("Error setting i2c address");
		exit(1);
	}
}

//Write command and read immediately
void rdwr_i2c(char cmd, char* response, int no_rd_bytes) {
	char wr_buf[1];
	wr_buf[0] = cmd;
	//wr_buf[1] = cmd;

	write_i2c( wr_buf, 1);
	read_i2c(response, no_rd_bytes); 
}

//Write data to i2c device
void write_i2c(char* buf, int no_wr_bytes) {
	int n = write(i2c_fd, buf, no_wr_bytes);
	if (n < 0) {
		printf("Error writing = %s\n", strerror(errno));
		exit(1);
	}
}

//Read data from i2c
void read_i2c(char* rd_buf, int no_rd_bytes) {
	int n = read(i2c_fd, rd_buf, no_rd_bytes);
	if (n < 0) {
		printf("Error reading = %s\n", strerror( errno));
		exit(1);
	}
}

//Get the voltage from device
double get_voltage() {
	char v_str[VOLTAGE_BYTES];
	rdwr_i2c(VOLTAGE_CMD, v_str, VOLTAGE_BYTES);

	double v_d = ((uint16_t)v_str[0] << 8) + v_str[1];
	v_d = v_d * 2 / 1000;
	//uint16_t v_int = ((uint16_t)v_str[0] << 8) + v_str[1];
	printf("%x\n", v_d);
	return v_d;
}

//Get test code from device
double get_testcode() {
	char test_str[TEST_BYTES];
	rdwr_i2c(TEST_CMD, test_str, TEST_BYTES);

	double t_d = ((uint16_t)test_str[0] << 8) + test_str[1];
	return t_d;
}
