#include "dio.h"
#include <stdlib.h>

volatile uint16_t *muxbus = 0;

pthread_mutex_t dio_lock;

void initiate_dio_mutex() {
	if (pthread_mutex_init(&dio_lock, NULL) != 0)
    {
        printf("\n mutex init failed\n");
        exit(1);
    }
}

void dio_initialize() {

	int mem = open("/dev/mem", O_RDWR|O_SYNC);
	muxbus = mmap(0,
		getpagesize(),
		PROT_READ|PROT_WRITE,
		MAP_SHARED,
		mem,
		0x30000000);

	initiate_dio_mutex();
}

void dio_destroy(){
	pthread_mutex_destroy(&dio_lock);
}

uint16_t peek16(uint16_t addr) 
{
	return muxbus[addr/2];
}
 
void poke16(uint16_t addr, uint16_t value)
{
	muxbus[addr/2] = value;
}

void mpoke16(uint16_t addr, uint16_t value)
{
	muxbus[(addr + 0x400)/2] = value;
}

uint16_t mpeek16(uint16_t addr) 
{
	return muxbus[(addr + 0x400)/2];
}


void mpeekpoke16(uint16_t addr, uint16_t bitposition, int onoff) {
	pthread_mutex_lock(&dio_lock);

	uint16_t value = mpeek16(addr);
	if (onoff) {
		while ( (mpeek16(addr) & bitposition) != bitposition){
			mpoke16(addr, value | bitposition);
			usleep(100);
		};
	} else {
		while ( (mpeek16(addr) & bitposition) != 0x0){
			mpoke16(addr, value & ~bitposition);
			usleep(100);
		};
	}

	pthread_mutex_unlock(&dio_lock);
}

