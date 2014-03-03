/**
 * Test File. Not part of the system.	
 */

#include <stdio.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>

int main() {
	char *port = "/dev/i2c-0";

	int fd = open (port, O_RDWR);
	if (fd < 0)
	{
        printf ("error");
        return;
	}

	int addr = 0x02;
 	if (ioctl(fd, I2C_SLAVE, addr) < 0) {
      	printf ( "Address Error = %s\n", strerror( errno ) ); 
    	return;
  	}


	int n;
	/**
	n = write (fd, "S0217R02P", 9);
	if (n < 0)
      	printf ( "Write Error = %s\n", strerror( errno ) ); 
  	else
    	printf ("Write succeed n = %i\n", n );
  	
  	char *buf;
  	**/
  	char buf[2];
  	buf[0]= 'F';
  	buf[1]= 'E';
  	buf[2]= '\0';
  	n = read( fd, buf, 2 );
 
  	if ( n == -1 )
      	printf ( "Error = %s\n", strerror( errno ) ); 
  	printf ( "Number of bytes to be read = %i\n", n );
  	printf ( "Buf = %s\n", buf );
  	close( fd );

}