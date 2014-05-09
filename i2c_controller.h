static const char I2C_PORTNAME[] = "/dev/i2c-0";

int i2c_count;


int *i2c_addr; //address array
double *v_calib_a;
double *v_calib_b;
double *t_calib_a;
double *t_calib_b;



int charging_current_addr;
double cc_calib_a, cc_calib_b;

int discharging_current_addr;
double dc_calib_a, dc_calib_b;




void open_i2c_port();


int test_all_addresses(int* r_d);


double get_voltage(int addr);
int get_voltage_all(double* r_d);
int get_current(double* r_d);
int get_overall_voltage(double* r_d);
int get_bypass_state_all(int* r_d);
double get_testcode(int addr);
double get_temperature(int addr);
int get_temperature_all(double* r_d);

int set_bypass_state(int addr, char state);
int set_bypass_state_all(char state);
