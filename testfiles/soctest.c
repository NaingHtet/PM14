#include <sys/time.h>
#include <stdio.h>

double old_time = 0;
double old_current;
double real_SOC = -1;
int disp_SOC = -1; //SOC for display, never exceeds the limit


//These values might need adjustment.
double alpha_gain = 0.00046;
double alpha_bias = 0;
double learning_rate = 0.03; //Learning rate is huge! Divide up a little bit


int charging_state = 1;
double ccount = 0;



void calculateSOC(double cur_current, double cur_time) {
	if (disp_SOC == -1) return;

	if (charging_state) 
		ccount += (old_current + cur_current) * (cur_time - old_time) / 2;
	else 
		ccount -= (old_current + cur_current) * (cur_time - old_time) / 2;
		

	real_SOC =  ((alpha_gain * ccount) + alpha_bias);

	disp_SOC = (int) real_SOC;
	if (disp_SOC > 100) disp_SOC = 100;
	else if (disp_SOC < 0) disp_SOC = 0;
}

void adjustSOC(int status) {
	printf("Error SOC : %f\n" , real_SOC);

	if (disp_SOC != -1) {
		double temp_error = (alpha_bias + (ccount*alpha_gain) - (100*status))/10000 ;
		printf("%f\n", temp_error);
		alpha_bias -= temp_error * learning_rate * 300000;
		alpha_gain -= ccount * temp_error * learning_rate /100000;
	} else {
		ccount = status * 216000;
		disp_SOC = status * 100;
	}
	real_SOC = status * 100;
}


int main() {
	int c = 0;
	int i = 0;

	for (c = 0; c < 200; ++c) {


		double cur_I = 30 ;

		double cur_time = old_time + 864;

		calculateSOC(cur_I, cur_time);
		old_time = cur_time;
		old_current = cur_I;

		if ( i == 9 ) {
			charging_state = 0;
			adjustSOC(1);
		} else if ( i == 19) {
			charging_state = 1;
			adjustSOC(0);
		}

		i = (i++)%20;

		printf("%f %f %.6f %.6f %d\n", real_SOC, ccount, alpha_gain, alpha_bias, charging_state);
	}
}

/**

	for each cell {
		if ( !bypassed && v[i] > soft_bypass_threshold ) {
			v[i].bypass()
			soft_bypass_threshold -=   0.001 * ((i2c_count-1)/2 - rank);
			if (threshold > 3.606) threshold = 3.606;
			else if (threashold < 3.585) threshold = 3.585;
			rank++;
		}
	}



**/

	/**
void manage_pack() {
	while(1) {
		wait for communication to connect back

		while(1) {
			getVoltage
			getTemperature
			getCurrent
			getTime
			
			getCharger

			if (nocharger && getCharger) {
				charging state = 1;
			}
			nocharger = getcharger;

			if (any of those do not work && no charger) {
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


			if (charging state)
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
