#include "i2c_controller.h"
#include "safety_checker.h"
#include <stdio.h>

void *check_safety(void *arg){
	while(1) {

		//now wait for i2c to resolve
		// pthread_mutex_lock(&errl_i2c);
		// while ( errf_i2c ) pthread_cond_wait(&errc_i2c, &errl_i2c);
		// pthread_mutex_unlock(&errl_i2c);

		// pthread_mutex_unlock(&errl_i2c);
		while(1) {
			double d[i2c_count];
			int n = -1;//get_voltage_all(d);
			if ( n < 0 ) break;
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
}