#include <stdio.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
volatile uint16_t *muxbus = 0;

void mpoke16(uint16_t addr, uint16_t value)
{
	if(muxbus == 0) {
		int mem = open("/dev/mem", O_RDWR|O_SYNC);
		muxbus = mmap(0,
			getpagesize(),
			PROT_READ|PROT_WRITE,
			MAP_SHARED,
			mem,
			0x30000000);
	}
 
	muxbus[(addr + 0x400)/2] = value;
}


int main() {
	mpoke16(0x8, 0x00ff);
	while(1) {
		mpoke16(0x4, 0x00ff);
		usleep(50000);
		mpoke16(0x4, 0x0000);
		usleep(50000);
	}


}