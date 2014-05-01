#include "i2c_controller.h"
#include "safety_checker.h"
#include "errorhandler.h"

#include <stdio.h>

int charging_state;

double alpha_gain;



void *check_safety(void *arg){
	charging_state = 0;

	while(1) {

		while(1) {
			double d_v[i2c_count];
			double d_t[i2c_count];

			int n = get_voltage_all(d_v);
			if ( n < 0 ) break;

			n = get_temperature_all(d_t);
			if ( n < 0 ) break;

			int safe_state = 1;

			if (!charging_state) {
				//calculateSOC
				int low_state = 0;
				int i;
				for ( i = 0; i < i2c_count ; i++) {
					if ( d_v[i] > SAFE_V_HIGH) {
						safe_state = 0;
						fprintf(stderr, "Voltage = %f over threshold!\n", d_v[i]);
					}
					if ( d_v[i] < SAFE_V_LOW) {
						low_state = 1;
						fprintf(stderr, "Voltage = %f below threshold!\n", d_v[i]);
					}
					if ( d_t[i] > SAFE_T_HIGH) {
						safe_state = 0;
						fprintf(stderr, "Voltage = %f over threshold!\n", d_v[i]);
					}
				}

				//test mode
				if ( bound_test ) safe_state = 0;


				if (!safe_state) {
					SYSTEM_SAFE = 0;
					fprintf(stderr, "System not safe.\n");
				} else {
					SYSTEM_SAFE = 1;
				} 

				if ( low_state ) {
					fprintf(stderr, "Battery low\n");
				}
				usleep(1000);
			}
		}
		//wait till i2c issue is resolved
		ewait_i2c();
	}
}

/**


void calculateSOC(double current) {
	if (charging_state)
		ccount += (old-current + current) * (old time - new time) / 2;
	els 
		ccount -= (old-current + current) * (old time - new time) / 2;




	if SOC = -1 return;
	else 
		SOC = alpha_gain * ccount;
}
starting alpha = 0.0046

//0 for 0% , 1 for 100%
void adjustSOC(int status) {
	if (SOC != -1) {
		double alpha_new = 0.0;
		if (status) alpha_new = 100/ccount;
		else {
			if (ccount != 21600)	alpha_new = 21600/(21600-ccount);
			else alpha_new = 10000;
		}
		alpha_gain = 0.7*alpha_new + 0.3*alpha_gain;
	} else {
		ccount = status * 21600;
	}
	SOC = status * 100;
}


void manage_pack() {
	while(1) {
		wait for communication to connect back

		while(1) {
			getVoltage
			getTemperature
			getCurrent
			getTime

			if (any of those do not work) {
				if (charging state) {
					//stop charging();
					chargingstate = 0;
					close all charging stuffs;
				}
				break;
			}
			if (!charging state) {
				

				calculateSOC(discharage);
				checkSafety(V,T,C)
				if (not safe)
					open relay
					display message
					
				else if (Voltage low)
					adjustSOC;
					if (charger plugged in) 
						chargingstate = 1;
						turn on charger
						open charging relay etc
					else
						display error

				else
					sleep(safe_sleep);


			} else {
				//we are in charging state
				calculateSOC(charge);
				checkSafety( only temperature);
				if (not safe)
					open relay
					display message
					stopCharging;

				else if (Voltage high)
					adjustSOC();
					stopCharging;

				else
					sleep(charging_timestep);
			}
		}
	}
}
**/