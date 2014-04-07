#include "i2c_controller.h"
#include "safety_checker.h"
#include <stdio.h>

static const double SAFE_V_HIGH = 10.0;
static const double SAFE_V_LOW = 0.0;

static const double safe_T_high = 100;
static const double safe_T_low = 0;

void *check_safety(void *arg){
	while(1) {
		double d = get_voltage();
		printf("Voltage received from BMS. Voltage is:%f\n", d);
		if ( d > SAFE_V_HIGH || d < SAFE_V_LOW ) {
			fprintf(stderr, "Voltage over threshold!\n");
		}
		usleep(1000);
	}
}