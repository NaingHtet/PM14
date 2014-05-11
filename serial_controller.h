int PACK_NO;
static const char SERIAL_PORTNAME[] = "/dev/ttyS3";


void serial_controller_initialize();

void *handle_serial(void *arg);
void close_serial();