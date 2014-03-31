/**
 * Test File. Not part of the system.	
 */

#include <stdio.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>

#include "i2c_controller.h"
#include "serial_controller.h"

int main() {
  //we are not communcting to address 0x02
  open_i2c_port();
  set_i2c_address(0x02);
  int count = 0;
  int err = 0;
  while(1) {
    //double d = get_voltage();
    //printf("%06.3f\n", d);
    count++;
    double d = get_testcode();
    if ( d != 66.0) {
        err++;
	printf("%f\n", d);
    }
    if (count%1000 == 0) {
        printf("Count:%d , err:%d\n", count, err);
    }
  }
}
