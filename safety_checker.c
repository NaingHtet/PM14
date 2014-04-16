#include "i2c_controller.h"
#include "safety_checker.h"
#include <stdio.h>

void *check_safety(void *arg){
	while(1) {
		double d[i2c_count];
		get_voltage_all(d);
		//printf("Voltage received from BMS.\n");
		int i;
		for ( i = 0; i < i2c_count ; i++) {
			if ( d[i] > SAFE_V_HIGH || d[i] < SAFE_V_LOW ) {
				fprintf(stderr, "Voltage = %f over threshold!\n", d[i]);
			}
		}
		usleep(1000);
	}
}