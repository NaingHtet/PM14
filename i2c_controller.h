static const char I2C_PORTNAME[] = "/dev/i2c-0";

int i2c_count;
int *i2c_addr; //address array

void open_i2c_port();


int test_all_addresses(int* r_d);


double get_voltage(int addr);
void get_voltage_all(double* r_d);

double get_testcode(int addr);
double get_temperature(int addr);
void get_temperature_all(double* r_d);

void set_bypass_state(int addr, char state);
