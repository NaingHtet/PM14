void *display_LCD(void *arg);
void set_error_msg(char* msg);
void set_display_voltages(double* v);
void set_display_temperatures(double* t, int count);
void lcdinit(int i2c_count);