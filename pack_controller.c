/** @file main_controller.c
 *  @brief The main controller for Pack Manager 14
 *  
 *  This is the main controller for Pack Manager 14. This initializes the program parameters and starts the different functions of the program.  
 *
 *  @author Naing Minn Htet <naingminhtet91@gmail.com>
 */


#include "i2c_controller.h"
#include "pack_controller.h"
#include "error_handler.h"
#include "dio.h"
#include "adc.h"

#include <sys/time.h>
#include <string.h>
#include <syslog.h>
#include <pthread.h>
#include <stdlib.h>
/**
  * SOC parameters . START
  */
double alpha_gain;
double alpha_bias;
double learning_rate; //Learning rate is huge! Divide up a little bit

double ccount;

double old_time;
double old_current;
int charger_plugged;
double real_SOC;
double disp_SOC_bias;

void calculateSOC(double cur_current, double cur_time);
void adjustSOC(int status);
/**
  * SOC parameters. END
  */


//Charging timer
double charging_time_threshold;
double charging_coulomb_threshold;



// char html_str[2000];


int detect_charger();
double get_fuse_temp();
double get_shunt_temp();

// pthread_mutex_t html_str_lock;

char status_messages[10][1000];
int status_count;

FILE* monitor;

void add_message(char* msg) {

	syslog(LOG_NOTICE, msg);
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


// void get_html_str(char* cpy) {
// 	pthread_mutex_lock(&html_str_lock);
// 	strcpy(cpy, html_str);
// 	pthread_mutex_unlock(&html_str_lock);
// }

void initialize_monitor() {
	status_count=0;
	int k;
	for (k = 0; k < 9 ; ++k)  {
		sprintf(status_messages[status_count], "\n");
		status_count = (status_count+1)%10;;
	}
	add_message("Pack Controller started\n");

	monitor = fopen("monitor.txt", "w+");
}


void pack_controller_initialize() {
	charging_state = 0;
	SYSTEM_SAFE = 1;
	BATTERY_LOW = 0;
	CHARGING_FINISHED = 0;
	charger_plugged = 1;

	initialize_monitor();
	adc_initialize();

	mpeekpoke16(DIO_DIRECTION, DIO_SAFETY, DIO_ON);
	mpeekpoke16(DIO_DIRECTION, DIO_CHARGERPLUG , DIO_OFF);
	mpeekpoke16(DIO_DIRECTION, DIO_CHARGERELAY, DIO_ON);

	mpeekpoke16(DIO_OUTPUT, DIO_SAFETY, DIO_OFF);

	// if (pthread_mutex_init(&html_str_lock, NULL) != 0)
 //    {
 //        syslog(LOG_ERR, "i2c mutex init failed\n");
 //        display_error_msg("E07-UNEXPECTED");
 //        exit(1);
 //    }

	old_time = -1;
}


void *control_pack(void *arg){
	int* PROGRAM_RUNNING= arg;

	sleep(1);

	
	//variables for cell balancing algorithm
	double bypass_threshold[i2c_count];
	int bypass_on[i2c_count];
	int j;
	for (j = 0; j < i2c_count; ++j) {
		bypass_threshold[j] = 3.595;
		bypass_on[j] = 0;
	}

	
	//turn dio input for charger

	while(*PROGRAM_RUNNING) {
		int log_count = 0;
		int monitor_count = 0;

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

			double cur_I_arr[2];
			n = get_current(cur_I_arr);
			if ( n < 0 ) break;

			double cur_I = cur_I_arr[0] - cur_I_arr[1];

			double cur_V;
			n = get_overall_voltage(&cur_V);
			if ( n < 0 ) break;

			double fuse_T = get_fuse_temp();

			double shunt_T = get_shunt_temp();

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

				charging_time_threshold = cur_time + (CHARGING_TIME_LIMIT*3600.0);
				charging_coulomb_threshold = ccount + (CHARGING_COULOMB_LIMIT*3600.0);

				syslog(LOG_NOTICE, "Threshold set. Time threshold is %8.2f and coulomb threshold is %8.2f.", charging_time_threshold, charging_coulomb_threshold);

				BATTERY_LOW = 0;
				mpeekpoke16(DIO_OUTPUT, DIO_CHARGERELAY, 1);
				continue;
			}
			charger_plugged = temp_plugged;

			if ( !charger_plugged ) CHARGING_FINISHED = 0;

			/**
			  * MONITOR and LOGGING START
			  */
			char wbuf[200];
			char xbuf[200];
			char ybuf[100];
			char sbuf[100];

			sprintf(wbuf, "%3.2f ",  d_v[0]);
			sprintf(xbuf, "%3.2f ",d_t[0]);
			sprintf(ybuf, "%d ",d_b[0]);

			for ( j = 1; j < i2c_count ; j++) {
				sprintf(wbuf+ strlen(wbuf), "%3.2f ",  d_v[j]);
				sprintf(xbuf+ strlen(xbuf), "%3.2f ",  d_t[j]);
				sprintf(ybuf+ strlen(ybuf), "%d ",  d_b[j]);
			}

			if (++monitor_count == 3) {
				monitor_count = 0;
				rewind(monitor);
				fprintf(monitor,"Charging state:%d    Plugged:%d    Cur_time:%7.2f \n", charging_state, charger_plugged, cur_time );
				fprintf(monitor,"Coloumb count:%6.2f  real SOC:%6.2f   disp SOC:%d \n", ccount, real_SOC, disp_SOC );
				fprintf(monitor,"Current:%6.2f   Pack Voltage:%7.3f Fuse Temp:%7.3f Shunt Temp:%7.3f\n", cur_I, cur_V, fuse_T, shunt_T );
				fprintf(monitor,"Charging current:%6.4f   Discharging current:%6.4f\n", cur_I_arr[0], cur_I_arr[1]);

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
			}
			// pthread_mutex_lock(&html_str_lock);
			// sprintf(html_str,"Charging state:%d    Plugged:%d    Cur_time:%7.2f <br>", charging_state, charger_plugged, cur_time );
			// sprintf(html_str + strlen(html_str),"Coloumb count:%6.2f  real SOC:%6.2f   disp SOC:%d <br>", ccount, real_SOC, disp_SOC );
			// sprintf(html_str + strlen(html_str),"Current:%6.2f   Pack Voltage:%7.3f Fuse Temp:%7.3f Shunt Temp:%7.3f<br>", cur_I, cur_V, fuse_T, shunt_T );
			// sprintf(html_str + strlen(html_str),"Bypass = ");
			// sprintf(html_str + strlen(html_str),ybuf);
			// sprintf(html_str + strlen(html_str),"<br>");
			// sprintf(html_str + strlen(html_str),"Voltage = ");
			// sprintf(html_str + strlen(html_str),wbuf);
			// sprintf(html_str + strlen(html_str),"<br>");
			// sprintf(html_str + strlen(html_str),"Temperature = ");
			// sprintf(html_str + strlen(html_str),xbuf);
			// sprintf(html_str + strlen(html_str),"<br>");
			// pthread_mutex_unlock(&html_str_lock);




			if (LOG_FILE) {
				fprintf(log_fp, "%.2f %d %d %.2f %.2f %d %6.2f %6.2f %6.2f %6.2f %s %s %s\n",
					cur_time, charging_state, charger_plugged,
					ccount, real_SOC, disp_SOC, cur_I, cur_V, fuse_T, shunt_T,ybuf ,wbuf, xbuf);
			}

			if (++log_count == 200) {
				syslog(LOG_INFO, "%.2f %d %d %.2f %.2f %d %6.2f %6.2f %6.2f %6.2f %s %s %s\n",
					cur_time, charging_state, charger_plugged,
					ccount, real_SOC, disp_SOC, cur_I, cur_V, fuse_T, shunt_T,ybuf ,wbuf, xbuf);
				log_count = 0;
			}

			/**
			  * MONITOR and LOGGING END
			  */

			int safe_state = 1;
			int low_state = 0;

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
						mpeekpoke16(DIO_OUTPUT, DIO_SAFETY, 1);
						add_message("Safety loop open\n");
					}
					SYSTEM_SAFE = 0;
					add_message("System not safe.\n");
				} else {
					if (!SYSTEM_SAFE) {
						mpeekpoke16(DIO_OUTPUT, DIO_SAFETY, 0);
						add_message("Safety loop close\n");
					}
					SYSTEM_SAFE = 1;
				} 

				if ( low_state ) {
					if (charger_plugged && SYSTEM_SAFE) {
						BATTERY_LOW = 0;
						CHARGING_FINISHED = 0;
						adjustSOC(0);
						charging_state = 1;

						charging_time_threshold = cur_time + (CHARGING_TIME_LIMIT*3600.0);
						charging_coulomb_threshold = ccount + (CHARGING_COULOMB_LIMIT*3600.0);

						syslog(LOG_NOTICE, "Threshold set. Time threshold is %8.2f and coulomb threshold is %8.2f.", charging_time_threshold, charging_coulomb_threshold);

						mpeekpoke16(DIO_OUTPUT, DIO_CHARGERELAY, 1);
						add_message("Started charging because battery low\n");
					} else {
						//display error
						if (!BATTERY_LOW) adjustSOC(0);
						BATTERY_LOW = 1;
						CHARGING_FINISHED = 0;

						mpeekpoke16(DIO_OUTPUT, DIO_SAFETY, 1);
						add_message("Low but can't charge\n");
					}
				} 
				usleep(300000);
			} else {
				//WE ARE IN CHARGING_STATE!
				if ( !charger_plugged ) {
					mpeekpoke16(DIO_OUTPUT, DIO_SAFETY, 0);
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

				if ( (cur_time > charging_time_threshold) || (ccount > charging_coulomb_threshold )) {
					add_message("Charging time out. Stopping charging\n");
					mpeekpoke16(DIO_OUTPUT, DIO_SAFETY, 0);
					break;
				}

				if ( charging_complete) {
					adjustSOC(1);
					CHARGING_FINISHED = 1;
					mpeekpoke16(DIO_OUTPUT, DIO_SAFETY, 0);
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
				usleep(300000);
			}
		}

		add_message("Charging Complete or encountered error\n");
		charging_state = 0;
		mpeekpoke16(DIO_OUTPUT, DIO_CHARGERELAY, 0);
		for (j = 0; j < i2c_count; ++j)	bypass_on[j] = 0;
		if (set_bypass_state_all(0) < 0) { 
			ewait_i2c();
		}
	}

	if (LOG_FILE)fclose(log_fp);
	fclose(monitor);

}

/**
  *  SOC function. START
  */


//Initialization for SOC. Get the parameters from config file
void set_state_of_charge(double SOC, double gain, double bias, double learning_rate_) {
	disp_SOC = (int)SOC;
	disp_SOC_bias = 0;

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

//Return the state of charge parameters so that we can save it in config file
void get_state_of_charge(double* SOC, double* gain, double* bias) {
	*SOC = real_SOC;
	*gain = alpha_gain;
	*bias = alpha_bias;
}

//Calculate the state of charge. If SOC hasn't been initialized, do nothing until next cycle.
void calculateSOC(double cur_current, double cur_time) {
	if (disp_SOC == -1) return;

	if (old_time != -1)
		ccount += (old_current + cur_current) * (cur_time - old_time) / 2;		

	real_SOC =  ((alpha_gain * ccount) + alpha_bias);

	disp_SOC = (int) (real_SOC + disp_SOC_bias);
	if (disp_SOC > 100) disp_SOC = 100;
	else if (disp_SOC < 0) disp_SOC = 0;
}


//Readjust SOC
//How SOC algorithm works is that, we assume that couloumb counting needs no adjustment.
//Let the coulomb count be x.
//Then we want y = SOC.
//According to the specs. 0Ah is 0% SOC and 60AH(216000) is 100%SOC.
//But, the reality is gonna be a little bit off. We might have x=100, y = 0% or x=220000, y = 100%
//So we treat this as a linear regression problem.
//Now, let y = alpha_gain*x + alpha_bias.
//We then use gradient descent to adjust/correct the gain and bias value.
//Refer to SOC algorithm for more information.
void adjustSOC(int status) {
	// printf("Error SOC : %f\n" , real_SOC);
	add_message("Adjusted SOC\n");
	if (disp_SOC != -1) {
		double temp_error = (alpha_bias + (ccount*alpha_gain) - (100*status)) ;
		alpha_bias -= temp_error * learning_rate;
		alpha_gain -= ccount * temp_error * learning_rate /2000000000;
	} else {
		ccount = status * 216000;
	}
	disp_SOC = status * 100;
	disp_SOC_bias = disp_SOC - ((alpha_gain * ccount) + alpha_bias);
	syslog(LOG_NOTICE, "For SOC, new alpha_bias = %f, new alpha_gain = %f", alpha_bias, alpha_gain);

}

/**
  *  SOC function. END
  */


//detects if the charger is plugged in
int detect_charger() {
	uint16_t value = mpeek16(0xc);
	if ( (value & 0x0040) == 0x40) {
		return 0;
	} else return 1;
}


//get the fuse temperature
double get_fuse_temp() {
	return get_adc(FUSE_TEMP);
}

//get the shunt temperature
double get_shunt_temp() {
	return get_adc(SHUNT_TEMP);
}