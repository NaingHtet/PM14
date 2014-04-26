#include "i2c_controller.h"
#include "safety_checker.h"
#include "errorhandler.h"

#include <stdio.h>


void *check_safety(void *arg){
	while(1) {
		//wait till i2c issue is resolved
		ewait_i2c();
		while(1) {
			double d[i2c_count];
			int n = get_voltage_all(d);
			if ( n < 0 ) break;
			// printf("Voltage received from BMS.\n");
			int i;
			for ( i = 0; i < i2c_count ; i++) {
				if ( d[i] > SAFE_V_HIGH || d[i] < SAFE_V_LOW ) {
					fprintf(stderr, "Voltage = %f over threshold!\n", d[i]);
				}
			}
			usleep(1000);
		}
	}
}