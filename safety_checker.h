
double SAFE_V_HIGH;
double SAFE_V_LOW;

double SAFE_T_NORMAL;
double SAFE_T_CHARGING;
int bound_test;

int SYSTEM_SAFE;
int LOG_FILE;

int disp_SOC; //SOC for display, never exceeds the limit
int charging_state;

void set_state_of_charge(double SOC, double gain, double bias, double learning_rate_);
void get_state_of_charge(double* SOC, double* gain, double* bias);

void *check_safety(void *arg);