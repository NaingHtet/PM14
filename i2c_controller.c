#include <stdio.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <fcntl.h>
#include <termios.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>

#include "i2c_controller.h"

static const int VOLTAGE_BYTES = 2;
static const int VOLTAGE_CMD = 0x10;

static const int TEST_BYTES = 2;
static const int TEST_CMD = 0x17;

static const int TEMP_BYTES = 2;
static const int TEMP_CMD = 0x17;

int i2c_fd;

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
	addr = _addr;
	int io = ioctl(i2c_fd, I2C_SLAVE, _addr);
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

	//TODO(NAING) : remove when AMS firmware bug is gone
	usleep(100);
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
	int allzero = 1;
	do {
		int n = read(i2c_fd, rd_buf, no_rd_bytes);
		if (n < 0) {
			printf("Error reading = %s\n", strerror( errno));
			exit(1);
		}

		//TODO(NAING) : remove when AMS firmware bug is gone
		//Basically read again if the returned data is all 0
		int i;
		for (i = 0; i < no_rd_bytes ; i++) {
			if ( rd_buf[i] != 0 ) {
				allzero = 0;
				break;
			} 
		}
		if (allzero) fprintf(stderr, "I2C has just read all zero.\n");
	} while (allzero);
}

//Get the voltage from device
double get_voltage(int addr) {
	set_i2c_address(addr);
	char r_str[VOLTAGE_BYTES];
	rdwr_i2c(VOLTAGE_CMD, r_str, VOLTAGE_BYTES);

	double r_d = ((uint16_t)r_str[0] << 8) + r_str[1];

	//CALIBRATION

	// v_d = v_d * 2 / 1000;
	// //uint16_t v_int = ((uint16_t)v_str[0] << 8) + v_str[1];
	// printf("%x\n", v_d);


	return r_d;
}

double* get_voltage_all() {
	double r_d[i2c_count];
	int i;
	for ( i = 0 ; i < i2c_count ; i++ ) {
		r_d[i] = get_voltage(i2c_addr[i]) ;
	}
	return r_d;
}

//Get test code from device
double get_testcode() {
	char r_str[TEST_BYTES];
	rdwr_i2c(TEST_CMD, r_str, TEST_BYTES);

	double r_d = ((uint16_t)r_str[0] << 8) + r_str[1];
	return r_d;
}

double get_temperature() {
	char r_str[TEMP_BYTES];
	rdwr_i2c(TEMP_CMD, r_str, TEMP_BYTES);

	double r_d = ((uint16_t)r_str[0] << 8) + r_str[1];
	return r_d;
}
