#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <errno.h>

#include "i2c_controller.h"
#include "serial_controller.h"

int main() {
	enable_serial_fpga();
	open_serial_port();
	open_i2c_port();
	set_i2c_address(0x02);
	while(1) {
		char buffer[100];
		int n = read_serial(buffer, sizeof(buffer));
		buffer[n] = '\0';
		printf("%s\n", buffer);

		double d = get_voltage();

		char wbuf[50];
		printf("main: %x\n", d);
		sprintf(wbuf, "%06.3f", d);
		write_serial(wbuf, 6);
	}
}
