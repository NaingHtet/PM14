

int fd;
static const char SERIAL_PORTNAME[] = "/dev/ttyS1";
void enable_serial_fpga();
void open_serial_port();
void write_serial(char* buf, int no_wr_bytes);
int read_serial(char* rd_buf, int no_rd_bytes);