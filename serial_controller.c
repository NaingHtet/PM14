#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/mman.h>

void enable_serial_fpga() {
	int mem = open("/dev/mem", O_RDWR|O_SYNC);
	uint16_t* fpga = mmap(0,
		getpagesize(),
		PROT_READ|PROT_WRITE,
		MAP_SHARED,
		mem,
		0x30000000);
	uint16_t addr = 0x000a;
	uint16_t value = 0x0756;
	fpga[addr/2] = value;
	close(fpga);
}

int main()	{
	enable_serial_fpga();
}
