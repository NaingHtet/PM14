int PACK_NO;
static const char SERIAL_PORTNAME[] = "/dev/ttyS3";

void enable_serial_fpga();
void open_serial_port();
void *handle_serial(void *arg);