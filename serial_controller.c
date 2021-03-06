/** @file serial_controller.c
 *  @brief The serial for Pack Manager 14
 *  
 *  This is the serial controller for Pack Manager 14.
 *  This uses RS232 port to communicate to central SCADA. We are using an isolator chip to convert RS232 to RS485.
 *  Due to this, we need to set the DIO to indicate whether we are sending or receiving.  
 *
 *  @author Naing Minn Htet <naingminhtet91@gmail.com>
 */

#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <termios.h>
#include <syslog.h>

#include "dio.h"
#include "serial_controller.h"
#include "i2c_controller.h"
#include "pack_controller.h"
#include "display_controller.h"

int serial_fd;

// TestMode params
int test_mode;

//Open the Serial port
void open_serial_port() {
	serial_fd = open( SERIAL_PORTNAME, O_RDWR | O_NOCTTY );
	if ( serial_fd < 0 ) {
		syslog(LOG_ERR, "Error Opening serial port");
		display_error_msg("E07:UNEXPECTED");
		exit(1);
	}
}

//Initialize. Open the port and enable dio for serial.
void serial_controller_initialize() {
	//enable dio direction
	mpeekpoke16(DIO_DIRECTION, DIO_SERIAL, DIO_ON);
	open_serial_port();
	test_mode = 0;
	bound_test = 0;
}

//Set the dio to write mode
void set_serial_diowrite() {
	mpeekpoke16(DIO_OUTPUT, DIO_SERIAL, DIO_ON);
}

//Set the dio to read mode
void set_serial_dioread() {
	mpeekpoke16(DIO_OUTPUT, DIO_SERIAL, DIO_OFF);
}


//Write data to serial port
void write_serial(char* buf, int no_wr_bytes) {

	set_serial_diowrite();
	int n = write(serial_fd, buf, no_wr_bytes);
	if (n < 0) {
		syslog(LOG_ERR, "Error writing for serial");
		display_error_msg("E07:UNEXPECTED");
		exit(1);
	}

	//Wait till serial data has actually been sent.
	usleep(1000*no_wr_bytes + 1000);

  }

//Read data from serial port
int read_serial(char* rd_buf, int no_rd_bytes) {

	set_serial_dioread();

	int n = read(serial_fd, rd_buf, no_rd_bytes);
	if (n < 0) {
		syslog(LOG_ERR, "Error reading for serial");
		display_error_msg("E07:UNEXPECTED");
		exit(1);
	}
	return n;
}

//Remove all the send_ack later, they can be annoying and not necessary
// void send_ack() {
// 	char wbuf[5];
// 	sprintf(wbuf , "%d OK", PACK_NO);
// 	write_serial(wbuf, 4);
// }

//Close the serial port
void close_serial() {
	close(serial_fd);
}


//Thread function for serial_controller.
void *handle_serial(void *arg) {
	while(1) {
		char buffer[1000];
		char wbuf[200];

		int n = read_serial(buffer, sizeof(buffer));
		buffer[n] = '\0';


		//The message doesn't concern this pack
		if (buffer[0] - '0' != PACK_NO) {
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
				// send_ack();
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
					// send_ack();
					double d= get_voltage(cellno - 1);
					sprintf(wbuf, "%d %06.3f", PACK_NO, d);
				} else {
					sprintf(wbuf, "%d ENOCELL", PACK_NO);
				}
			}
		} 

		//TESTMODE
		else if (strcmp(token, "TESTMODE") == 0)  {
			token = strtok(NULL, del);
			if (token != NULL) {
				if (token[0] == '1') {
					// send_ack();
					test_mode = 1;
					sprintf(wbuf, "%d SUCCESS", PACK_NO);
				}
				else if (token[0] == '0') {
					// send_ack();
					test_mode = 0;
					sprintf(wbuf, "%d SUCCESS", PACK_NO);
				}
				else sprintf(wbuf, "%d EBADARG", PACK_NO);
			} else {
				sprintf(wbuf, "%d EBADARG", PACK_NO);
			}

		}

		//Out of bounds
		else if (strcmp(token, "TOB") == 0) {
			if (test_mode) {
				token = strtok(NULL, del);
				if (token != NULL) {
					if (token[0] == '1') {
						bound_test = 1;
						sprintf(wbuf, "%d SUCCESS", PACK_NO);
					}
					else if (token[0] == '0') {
						bound_test = 0;
						sprintf(wbuf, "%d SUCCESS", PACK_NO);
					}
					else sprintf(wbuf, "%d EBADARG", PACK_NO);
				} else {
					sprintf(wbuf, "%d EBADARG", PACK_NO);
				}
			} else {
				sprintf(wbuf, "%d ENOTALLOWED", PACK_NO);
			}
		}


		//BYPASS ON
		else if (strcmp(token, "BYPSON") == 0) {
			token = strtok(NULL, del);
			if (token != NULL) {
				// send_ack();
				int cellno = atoi(token);
				set_bypass_state(i2c_addr[cellno - 1], 0x1);
				sprintf(wbuf, "%d SUCCESS", PACK_NO);
			} else {
				sprintf(wbuf, "%d EBADARG", PACK_NO);
			}
		} 

		//BYPASS OFF
		else if (strcmp(token, "BYPSOFF") == 0) {
			token = strtok(NULL, del);
			if (token != NULL) {
				int cellno = atoi(token);
				if (cellno > 0 && cellno <= i2c_count ) {
					// send_ack();
					set_bypass_state(i2c_addr[cellno - 1], 0x0);
					sprintf(wbuf, "%d SUCCESS", PACK_NO);
				} else {
					sprintf(wbuf, "%d ENOCELL", PACK_NO);
				}
			} else {
				sprintf(wbuf, "%d EBADARG", PACK_NO);
			}
		} 

		else if (strcmp(token, "TEST?") == 0) {
			sprintf(wbuf, "%d 42", PACK_NO);
		}

		else if (strcmp(token, "CELLCNT?") == 0) {
			sprintf(wbuf, "%d %d", PACK_NO, i2c_count);
		}


		//ELSE COMMAND NOT RECOGNIZED
		else {
			sprintf(wbuf, "%d EBADCMD", PACK_NO);
		}

		write_serial(wbuf, strlen(wbuf));
	}

}