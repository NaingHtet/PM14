#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <errno.h>

#include "serial_controller.h"

void enable_serial_fpga() {
	int mem = open("/dev/mem", O_RDWR|O_SYNC);
	uint16_t* fpga = mmap(0,
		getpagesize(),
		PROT_READ|PROT_WRITE,
		MAP_SHARED,
		mem,
		0x30000000);
	uint16_t addr = 0x000a;
	uint16_t value = 0x0756;
	fpga[addr/2] = value;
	close(fpga);
}

void open_serial_port() {
	i2c_fd = open( SERIAL_PORTNAME, O_RDWR);
	if ( i2c_fd < 0 ) {
		printf("Error Opening serial port");
		exit(1);
	}
}
void write_serial(char* buf, int no_wr_bytes) {
	int n = write(i2c_fd, buf, no_wr_bytes);
	if (n < 0) {
		printf("Error writing = %s\n", strerror( errno));
		exit(1);
	}
}

int read_serial(char* rd_buf, int no_rd_bytes) {
	int n = read(i2c_fd, rd_buf, no_rd_bytes);
	if (n < 0) {
		printf("Error reading = %s\n", strerror( errno));
		exit(1);
	}
	return n;
}

