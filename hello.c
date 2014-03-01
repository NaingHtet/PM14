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
	speed_t baud = B9600;


struct termios settings;
tcgetattr(fd, &settings);

cfsetospeed(&settings, baud); /* baud rate */
settings.c_cflag &= ~PARENB; /* no parity */
settings.c_cflag &= ~CSTOPB; /* 1 stop bit */
settings.c_cflag &= ~CSIZE;
settings.c_cflag |= CS8 | CLOCAL; /* 8 bits */
settings.c_lflag = ICANON; /* canonical mode */
settings.c_oflag &= ~OPOST; /* raw output */

tcsetattr(fd, TCSANOW, &settings); /* apply the settings */
tcflush(fd, TCOFLUSH);

/* — code to use the port here — */

	while (0) {
		write (fd, "hello!\n", 7);
	}
}