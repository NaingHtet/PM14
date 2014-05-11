#include "i2c_controller.h"
#include "safety_checker.h"
#include "error_handler.h"
#include "dio.h"
#include <sys/time.h>
#include <string.h>


double alpha_gain;
double alpha_bias;
double learning_rate; //Learning rate is huge! Divide up a little bit

double ccount;

double old_time;
double old_current;
int charger_plugged = 1;
double real_SOC = -1;


void calculateSOC(double cur_current, double cur_time);
void adjustSOC(int status);
int detect_charger();

char status_messages[10][1000];
int status_count;

FILE* monitor;

void set_state_of_charge(double SOC, double gain, double bias, double learning_rate_) {
	disp_SOC = (int)SOC;
	if ( SOC == -1.0) {
		ccount = 0;
		real_SOC = -1;
	} else {
		ccount = (SOC-bias)/gain;
	}

	alpha_gain = gain;
	alpha_bias = bias;
 	learning_rate = learning_rate_;

}

void get_state_of_charge(double* SOC, double* gain, double* bias) {
	*SOC = real_SOC;
	*gain = alpha_gain;
	*bias = alpha_bias;
}

void add_message(char* msg) {
	strcpy(status_messages[status_count], msg);
	status_count = (status_count+1)%10;
}

void print_all_messages() {
	fprintf(monitor,"\nStatus Messages\n______________________________\n");
	int i;

	fprintf(monitor,status_messages[(status_count+9)%10 ]);
	for ( i = (status_count+8)%10 ; i != (status_count+9)%10; i= (i+9)%10) {
		fprintf(monitor,status_messages[i]);
	}
}

double get_fuse_temp() {
	double x = (signed short)peek16(0x82 + 10);
	x = (x * 1006)/200;
	x = (x * 2048)/0x8000;

	x = x - 500;
	return x/10;
}


void *check_safety(void *arg){
	printf("Safety Checker starting\n");
	charging_state = 0;
	int* PROGRAM_RUNNING= arg;
	poke16(0x80, 0x18); 
 
	poke16(0x82, 0x3c); 

	sleep(1);
	status_count=0;
	int k;
	for (k = 0; k < 9 ; ++k)  add_message("\n");
	add_message("Program started\n");

	SYSTEM_SAFE = 1;
	//variables for cell balancing algorithm
	double bypass_threshold[i2c_count];
	int bypass_on[i2c_count];
	int j;
	for (j = 0; j < i2c_count; ++j) {
		bypass_threshold[j] = 3.595;
		bypass_on[j] = 0;
	}

	old_time = -1;
	//turn dio input for charger
	mpeekpoke16(0x0008, 0x0040 , 0);

	mpeekpoke16(0x0008, DIO_CHARGERELAY, 1);
	mpeekpoke16(0x0004, DIO_SAFETY, 0);


	monitor = fopen("monitor.txt", "w+");

	while(*PROGRAM_RUNNING) {
		int bypass_rank = 0;
		//the first data is ussually buggy. call it and eliminate it.
		double temp[i2c_count];
		get_voltage_all(temp);
		get_temperature_all(temp);

		while(*PROGRAM_RUNNING) {

			//Get all the necessary system parameters
			double d_v[i2c_count];
			double d_t[i2c_count];
			int d_b[i2c_count];

			int n = get_voltage_all(d_v);
			if ( n < 0 ) break;
		
			n = get_temperature_all(d_t);
			if ( n < 0 ) break;

			n = get_bypass_state_all(d_b);
			if ( n < 0 ) break;

			double cur_I;
			n = get_current(&cur_I);
			if ( n < 0 ) break;

			double cur_V;
			n = get_overall_voltage(&cur_V);
			if ( n < 0 ) break;

			double fuse_T = get_fuse_temp();

			int temp_plugged = detect_charger();

			struct timeval tim;
			gettimeofday(&tim, NULL);
			double cur_time = tim.tv_sec + (tim.tv_usec/1000000.0);

			//Calculate SOC and then update the old values with new ones
			calculateSOC(cur_I, cur_time);
			old_time = cur_time;
			old_current = cur_I;

			if (!charger_plugged && temp_plugged ) {
				charger_plugged = temp_plugged;
				// fprintf(stderr, "Started Charging because plugged and unplugged\n");
				add_message("Started Charging because plugged and unplugged\n");
				charging_state = 1;
				mpeekpoke16(0x0004, DIO_CHARGERELAY, 1);
				continue;
			}
			charger_plugged = temp_plugged;

			int safe_state = 1;
			int low_state = 0;

			char wbuf[1000];
			char xbuf[1000];
			char ybuf[100];
			char sbuf[100];

			fprintf(stderr, "\033[2J");

			rewind(monitor);
			fprintf(monitor,"Charging state:%d    Plugged:%d    Cur_time:%7.2f \n", charging_state, charger_plugged, cur_time );
			fprintf(monitor,"Coloumb count:%6.2f  real SOC:%6.2f   disp SOC:%d \n", ccount, real_SOC, disp_SOC );
			fprintf(monitor,"Current:%6.2f   Pack Voltage:%7.3f Fuse Temp:%7.3f \n", cur_I, cur_V, fuse_T );

			sprintf(wbuf, "%3.2f ",  d_v[0]);
			sprintf(xbuf, "%3.1f ",d_t[0]);
			sprintf(ybuf, "%d ",d_b[0]);

			for ( j = 1; j < i2c_count ; j++) {
				sprintf(wbuf+ strlen(wbuf), "%3.2f ",  d_v[j]);
				sprintf(xbuf+ strlen(xbuf), "%3.1f ",  d_t[j]);
				sprintf(ybuf+ strlen(ybuf), "%d ",  d_b[j]);
			}

			fprintf(monitor,"Bypass = ");
			fprintf(monitor,ybuf);
			fprintf(monitor,"\n");

			fprintf(monitor,"Voltage = ");
			fprintf(monitor,wbuf);
			fprintf(monitor,"\n");

			fprintf(monitor,"Temperature = ");
			fprintf(monitor,xbuf);
			fprintf(monitor,"\n");

			print_all_messages();

			fflush(stdout);

			if (LOG_FILE) {
				fprintf(log_fp, "%.2f %d %d %.2f %.2f %d %6.2f %6.2f %6.2f %s %s %s\n",
					cur_time, charging_state, charger_plugged,
					ccount, real_SOC, disp_SOC, cur_I, cur_V, fuse_T, ybuf ,wbuf, xbuf);
			}

			if (!charging_state) {

				int i;
				for ( i = 0; i < i2c_count ; i++) {

					if ( d_v[i] > SAFE_V_HIGH) {
						safe_state = 0;

						sprintf(sbuf,"Voltage of cell %d = %f over threshold!\n",i, d_v[i]);
						add_message(sbuf);
					}
					if ( d_v[i] < SAFE_V_LOW) {
						low_state = 1;
						sprintf(sbuf,"Voltage of cell %d = %f below threshold!\n",i, d_v[i]);
						add_message(sbuf);
					}
					if ( d_t[i] > SAFE_T_NORMAL) {
						safe_state = 0;
						sprintf(sbuf,"Temperature of cell %d = %f over threshold!\n",i, d_t[i]);
						add_message(sbuf);
					}

				}
				//test mode
				if ( bound_test ) safe_state = 0;

				if (!safe_state) {
					if (SYSTEM_SAFE) {
						mpeekpoke16(0x0004, DIO_SAFETY, 1);
						add_message("Safety loop open\n");
					}
					SYSTEM_SAFE = 0;
					fprintf(stderr, "System not safe.\n");
				} else {
					if (!SYSTEM_SAFE) {
						mpeekpoke16(0x0004, DIO_SAFETY, 0);
						add_message("Safety loop close\n");
					}
					SYSTEM_SAFE = 1;
				} 

				if ( low_state ) {
					fprintf(stderr, "Battery low\n");
					if (charger_plugged && SYSTEM_SAFE) {
						adjustSOC(0);
						charging_state = 1;
						mpeekpoke16(0x0004, DIO_CHARGERELAY, 1);
						add_message("Started charging because low\n");
					} else {
						//display error
						mpeekpoke16(0x0004, DIO_SAFETY, 1);
						add_message("Low but can't charge\n");
					}
				}
				usleep(1000);
			} else {
				//WE ARE IN CHARGING_STATE!
				if ( !charger_plugged ) {
					mpeekpoke16(0x0004, DIO_SAFETY, 0);
					break;
				}
				int charging_complete = 0;

				int i;
				for ( i = 0; i < i2c_count ; i++) {

					if ( bypass_on[i] != d_b[i]) {
						n = set_bypass_state(i2c_addr[i], bypass_on[i]);
						if ( n < 0 ) break;
					}


					if ( d_v[i] > SAFE_V_HIGH) {
						charging_complete = 1;
						sprintf(sbuf,"Voltage = %f. Charging Complete!\n", d_v[i]);
						add_message(sbuf);
					}
					else if ( (d_v[i] > bypass_threshold[i]) && !bypass_on[i]) {
						n = set_bypass_state(i2c_addr[i], 1);
						sprintf(sbuf, "Bypassed on for %d, V=%f\n", i, d_v[i] );
						add_message(sbuf);
						if (n < 0) break;
						bypass_on[i] = 1;
						bypass_threshold[i] -= 0.001 * (((i2c_count-1)/2) - bypass_rank);
						if (bypass_threshold[i] > 3.606) bypass_threshold[i] = 3.606;
						else if (bypass_threshold[i] < 3.585) bypass_threshold[i] = 3.585;
						bypass_rank++;
					}

					if ( d_t[i] > SAFE_T_CHARGING) {
						safe_state = 0;
						sprintf(sbuf,"Temperature = %f over threshold!\n", d_t[i]);
						add_message(sbuf);
					}
				}

				if ( n < 0 ) break;

				if ( !safe_state) {
					SYSTEM_SAFE = 0;
					add_message("System not safe, stopping charging\n");
					break;
				} else {
					SYSTEM_SAFE = 1;
				}
					
			printf("Charging state:%d    Plugged:%d    Cur_time:%7.2f \n", charging_state, charger_plugged, cur_time );
			printf("Coloumb count:%6.2f  real SOC:%6.2f   disp SOC:%d \n", ccount, real_SOC, disp_SOC );
			printf("Current:%6.2f   Pack Voltage:%7.3f \n", cur_I, cur_V );

			sprintf(wbuf, "Voltage = %3.2f  ",  d_v[0]);
			sprintf(xbuf, "Temperature = %3.1f   ",d_t[0]);

				if ( charging_complete) {
					adjustSOC(1);
					mpeekpoke16(0x0004, DIO_SAFETY, 0);
					add_message("Charging complete\n");
					for ( i = 0; i < i2c_count ; i++) {
						if(!bypass_on[i]) {
							bypass_threshold[i] -= 0.001 * (((i2c_count-1)/2) - i2c_count);
						}
						sprintf(sbuf,"New bypass for %d is %f\n", i, bypass_threshold[i]);
						add_message(sbuf);
					}
					break; //Charging complete
				}
				usleep(1000);
			}
		}

		add_message("Charging Complete or encountered error\n");
		charging_state = 0;
		mpeekpoke16(0x0004, DIO_CHARGERELAY, 0);
		for (j = 0; j < i2c_count; ++j)	bypass_on[j] = 0;
		if (set_bypass_state_all(0) < 0) { 
			//wait till i2c issue is resolved
			ewait_i2c();
		}
	}
	if (LOG_FILE)fclose(log_fp);
}



void calculateSOC(double cur_current, double cur_time) {
	if (disp_SOC == -1) return;

	if (old_time != -1)
		ccount += (old_current + cur_current) * (cur_time - old_time) / 2;		

	real_SOC =  ((alpha_gain * ccount) + alpha_bias);

	disp_SOC = (int) real_SOC;
	if (disp_SOC > 100) disp_SOC = 100;
	else if (disp_SOC < 0) disp_SOC = 0;
}

void adjustSOC(int status) {
	// printf("Error SOC : %f\n" , real_SOC);
	add_message("Adjusted SOC\n");
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


int detect_charger() {
	uint16_t value = mpeek16(0xc);
	if ( (value & 0x0040) == 0x40) {
		return 0;
	} else return 1;
}

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
