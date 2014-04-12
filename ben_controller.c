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

int main() {

	open_i2c_port();
	set_i2c_address(0x02);
	int i = 0;
	char bypass_state = 0x00;

	while(1) {
		if (i%10 == 0) {
			if (bypass_state == 0x00) bypass_state = 0x01;
			else bypass_state = 0x00;
			set_bypass_state(bypass_state);
		}
		i++;
		double d = get_voltage();
		
		if ( d < 466) {
			set_bypass_state(0x00);
			exit(1);
		}


		fprintf(stdout, "%d %06.2f\n", bypass_state, d);
		fflush(stdout);
		sleep(30);
	}
}
