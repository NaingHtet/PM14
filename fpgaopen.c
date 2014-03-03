/**
 * Test File. Not part of the system.	
 */

#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/mman.h>
 
volatile uint16_t *fpga = 0;
uint16_t peek16(uint16_t addr) 
{
	uint16_t value;
 
	if(fpga == 0) {
		int mem = open("/dev/mem", O_RDWR|O_SYNC);
		fpga = mmap(0,
			getpagesize(),
			PROT_READ|PROT_WRITE,
			MAP_SHARED,
			mem,
			0x30000000);
	}
 
	return fpga[addr/2];
}
 
void poke16(uint16_t addr, uint16_t value)
{
	if(fpga == 0) {
		int mem = open("/dev/mem", O_RDWR|O_SYNC);
		fpga = mmap(0,
			getpagesize(),
			PROT_READ|PROT_WRITE,
			MAP_SHARED,
			mem,
			0x30000000);
	}
 
	fpga[addr/2] = value;
}
 
int main()
{
	int x, i;
	// Select TS-81XX, 15hz, 16-bit resolution with 0x gain
	poke16(0x80, 0x18); 
 
	// Select TS-8390, 15hz, 16-bit resolution with 0x gain
	//poke16(0x80, 0x28); 
 
	// unmaks to enable all 6 channels
	poke16(0x82, 0x3f); 
 
	usleep(500000); // allow time for conversions
	for (i = 1; i <= 6; i++) {
		x = (signed short)peek16(0x82 + 2*i);
		if (i > 2) x = (x * 1006)/200;
		x = (x * 2048)/0x8000;
		printf("adc%d=%d\n", i, x);
	}
 
	return 0;
}