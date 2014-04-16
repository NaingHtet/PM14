#include "i2c_controller.h"
#include "safety_checker.h"
#include <stdio.h>

static const double SAFE_V_HIGH = 600.0;
static const double SAFE_V_LOW = 200.0;

static const double safe_T_high = 100;
static const double safe_T_low = 0;

void *check_safety(void *arg){
	while(1) {
		double d[i2c_count];
		get_voltage_all(d);
		printf("Voltage received from BMS.\n");
		int i;
		for ( i = 0; i < i2c_count ; i++) {
			if ( d[i] > SAFE_V_HIGH || d[i] < SAFE_V_LOW ) {
				fprintf(stderr, "Voltage = %f over threshold!\n", d[i]);
			}
		}
		usleep(1000);
	}
}