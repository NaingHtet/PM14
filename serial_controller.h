/** @file i2c_controller.h
 *  @brief The header file for serial_controller
 *
 *  @author Naing Minn Htet <naingminhtet91@gmail.com>
 */


int PACK_NO;
static const char SERIAL_PORTNAME[] = "/dev/ttyS3";


void serial_controller_initialize();

void *handle_serial(void *arg);
void close_serial();