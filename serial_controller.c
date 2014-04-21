#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "serial_controller.h"
#include "i2c_controller.h"

int serial_fd;

//Allows the fpga to control RDWR over RS485 port. This needs to be done to read write to serial port.
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

//Open the serial port
void open_serial_port() {
	serial_fd = open( SERIAL_PORTNAME, O_RDWR);
	if ( serial_fd < 0 ) {
		printf("Error Opening serial port");
		exit(1);
	}
}

//Write data to serial port
void write_serial(char* buf, int no_wr_bytes) {
	int n = write(serial_fd, buf, no_wr_bytes);
	if (n < 0) {
		printf("Error writing = %s\n", strerror( errno));
		exit(1);
	}
}

//Read data from serial port
int read_serial(char* rd_buf, int no_rd_bytes) {
	int n = read(serial_fd, rd_buf, no_rd_bytes);
	if (n < 0) {
		printf("Error reading = %s\n", strerror( errno));
		exit(1);
	}
	return n;
}

void send_ack() {
	char wbuf[5];
	sprintf(wbuf , "%d OK", PACK_NO);
	write_serial(wbuf, 4);
}

void *handle_serial(void *arg) {
	while(1) {
		char buffer[1000];
		char wbuf[100];
		printf("waiting for command \n");
		int n = read_serial(buffer, sizeof(buffer));
		buffer[n] = '\0';

		//printf("Command from serial : %s\n", buffer);

		//The message doesn't concern this pack
		if (buffer[0] - '0' != PACK_NO) {
			fprintf(stderr, "buf = %d, pack = %d \n", buffer[0] - '0', PACK_NO);
			continue;
		} 

		//Separate the commands into tokens
		char *token;
		const char del[1] = " ";
		//This is the address
		token = strtok(buffer, del);

		token = strtok(NULL, del);
		if (token == NULL) {
			sprintf(wbuf, "%d EBADCMD", PACK_NO);
		} 

		//VOLTAGE
		else if (strcmp(token, "V?") == 0) {
			token = strtok(NULL, del);
			if (token == NULL) {
				send_ack();
				//return all the voltages
				double d[i2c_count];
				get_voltage_all(d);
				sprintf(wbuf, "%d %06.3f", PACK_NO, d[0]);
				int i;
				for (i = 1; i < i2c_count ; i++) {
					sprintf(wbuf + strlen(wbuf)," %06.3f", d[i] );
				}
			} else {
				int cellno = atoi(token);
				if (cellno > 0 && cellno <= i2c_count ) {
					send_ack();
					double d= get_voltage(i2c_addr[cellno - 1]);
					sprintf(wbuf, "%d %06.3f", PACK_NO, d);
				} else {
					sprintf(wbuf, "%d ENOCELL", PACK_NO);
				}
			}
		} else if (strcmp(token, "BYPSON")) {
			token = strtok(NULL, del);
			if (token != NULL) {
				send_ack();
				int cellno = atoi(token);
				set_bypass_state(i2c_addr[cellno - 1], 0x1);
				sprintf(wbuf, "%d SUCCESS", PACK_NO);
			} else {
				sprintf(wbuf, "%d EBADARG", PACK_NO);
			}
		} else if (strcmp(token, "BYPSOFF")) {
			token = strtok(NULL, del);
			if (token != NULL) {
				int cellno = atoi(token);
				if (cellno > 0 && cellno <= i2c_count ) {
					send_ack();
					set_bypass_state(i2c_addr[cellno - 1], 0x0);
					sprintf(wbuf, "%d SUCCESS", PACK_NO);
				} else {
					sprintf(wbuf, "%d ENOCELL", PACK_NO);
				}
			} else {
				sprintf(wbuf, "%d EBADARG", PACK_NO);
			}
		} else {
			sprintf(wbuf, "%d EBADCMD", PACK_NO);
		}

		write_serial(wbuf, strlen(wbuf));
	}

}