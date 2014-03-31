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
  
  while(1) {
    double d = get_voltage();
    printf("%06.3f\n", d);
  
    double d = get_testcode();
    printf("%x\n", d);
  }
}
