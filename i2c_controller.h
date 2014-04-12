static const char I2C_PORTNAME[] = "/dev/i2c-0";

int i2c_count;
int i2c_addr[];

int open_i2c_port();
int set_i2c_address(int addr);
void read_i2c(char* rd_buf, int no_rd_bytes);
void write_i2c(char* buf, int no_wr_bytes);
void rdwr_i2c(char cmd, char* response, int no_rd_bytes);
double get_voltage();
double get_testcode();
double get_temperature();
void set_bypass_state(char state);
