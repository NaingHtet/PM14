#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>


#include "i2c_controller.h"
#include "serial_controller.h"
#include "safety_checker.h"

int main() {

	enable_serial_fpga();
	open_serial_port();
	
	open_i2c_port();
	set_i2c_address(0x02);

	pthread_t safety_thread;
	pthread_create(&safety_thread, NULL, check_safety, NULL);
	pthread_join(safety_thread, NULL);


	
	// while(1) {
	// 	char buffer[100];
	// 	int n = read_serial(buffer, sizeof(buffer));
		
	// 	buffer[n] = '\0';
	// 	printf("Command from serial : %s\n", buffer);

	// 	char answer[] = "1 V? 1";
	// 	char wbuf[50];
	// 	int len = 0;
	// 	if (strcmp(buffer,answer) != 0) {
	// 		sprintf(wbuf, "1 EBADCMD");
	// 		len = 9;
	// 	} else {
	// 		write_serial("1 OK", 4);
	// 		double d = get_voltage();
	// 		printf("Voltage received from BMS. Voltage is:%06.3f\n", d);
	// 		sprintf(wbuf, "%06.3f", d);
	// 		len = 6;
	// 	}
	// 	write_serial(wbuf, len);
	// }
}
