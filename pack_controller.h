/** @file pack_controller.h
 *  @brief The header file for pack_controller
 *
 *  @author Naing Minn Htet <naingminhtet91@gmail.com>
 */


#include <stdio.h>

FILE* log_fp;
int LOG_FILE;

int SYSTEM_SAFE;
int BATTERY_LOW;
int CHARGING_FINISHED;
int bound_test;


double SAFE_V_HIGH;
double SAFE_V_LOW;

double SAFE_T_NORMAL;
double SAFE_T_CHARGING;

double CHARGING_TIME_LIMIT;
double CHARGING_COULOMB_LIMIT;


int disp_SOC; //SOC for display, never exceeds the limit
int charging_state;

void set_state_of_charge(double SOC, double gain, double bias, double learning_rate_);
void get_state_of_charge(double* SOC, double* gain, double* bias);

void pack_controller_initialize();
void *control_pack(void *arg);

void get_html_str(char* cpy);