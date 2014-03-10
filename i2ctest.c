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

	int addr = 0x00;
 	if (ioctl(fd, I2C_SLAVE, addr) < 0) {
      	printf ( "Address Error = %s\n", strerror( errno ) ); 
    	return;
  	}

	char buf[3];
	buf[0] = 0x01;
	buf[1] = 0x02;

buf[2] = 0x02;
	int n = write (fd,buf,3);
	if (n < 0)
      	printf ( "Write Error = %s\n", strerror( errno ) ); 
  	else
    	printf ("Write succeed n = %i\n", n );
  	/**
  	char rbuf[2];
  	rbuf[0]= 'F';
  	rbuf[1]= 'E';
  	rbuf[2]= '\0';
  	n = read( fd, rbuf, 2);
 
  	if ( n == -1 )
      	printf ( "Error = %s\n", strerror( errno ) ); 
  	printf ( "Number of bytes to be read = %i\n", n );
  	printf ( "Buf = %02x\n", rbuf[0] );
	printf ( "2Buf = %02x\n", rbuf[1] );
	**/
close( fd );

}
