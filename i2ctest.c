/**
 * Test File. Not part of the system.	
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
int main() {
	char *port = "/dev/i2c-0";

	int fd = open (port, O_RDWR | O_NOCTTY | O_SYNC);
	if (fd < 0)
	{
        printf ("error");
        return;
	}

	write (fd, "S0217R02P", 9);
  	char *buf;
 
  	int n = read( fd, buf, 1 );
 
  	if ( n == -1 )
	    printf ( "Error reading" );
 
  	printf ( "Number of bytes to be read = %i\n", n );
  	printf ( "Buf = %s\n", buf );
 
  	close( fd );

}