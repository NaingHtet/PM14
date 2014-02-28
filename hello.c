#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
int main() {
	char *port = "/dev/i2c-0";

	int fd = open (port, O_RDWR | O_NOCTTY | O_SYNC);
	if (fd < 0)
	{
        printf ("error");
        return;
	}
	while (0) {
		write (fd, "hello!\n", 7);
	}
}