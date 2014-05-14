/** @file dio.c
 *  @brief The functions to access the dio channels on PacMan
 *  
 *  @author Naing Minn Htet <naingminhtet91@gmail.com>
 */

#include "dio.h"
#include "display_controller.h"
#include <stdlib.h>
#include <syslog.h>

volatile uint16_t *muxbus = 0;

//mutex lock to make sure that threads don't read and write at the same time
pthread_mutex_t dio_lock;

//initiate the mutex
void initiate_dio_mutex() {
	if (pthread_mutex_init(&dio_lock, NULL) != 0)
    {
        syslog(LOG_ERR, "Failed to initiate dio mutex");
        display_error_msg("E07:UNEXPECTED");
        exit(1);
    }
}

//initialize for dio function calls
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

//destroy for dio
void dio_destroy(){
	pthread_mutex_destroy(&dio_lock);
}

//Peek the value at addr for adc
uint16_t peek16(uint16_t addr) 
{
	return muxbus[addr/2];
}
 
//Write value at addr for adc
void poke16(uint16_t addr, uint16_t value)
{
	muxbus[addr/2] = value;
}

//Write value at addr in 0x400 (dio) range
void mpoke16(uint16_t addr, uint16_t value)
{
	muxbus[(addr + 0x400)/2] = value;
}

//Peek value at addr in 0x400 (dio) range
uint16_t mpeek16(uint16_t addr) 
{
	return muxbus[(addr + 0x400)/2];
}

//Only change the bit at bitposition to onoff value. Mutex protected to avoid race conditions
void mpeekpoke16(uint16_t addr, uint16_t bitposition, int onoff) {
	pthread_mutex_lock(&dio_lock);
	int tries = 0;

	uint16_t value = mpeek16(addr);
	if (onoff) {
		while ( (mpeek16(addr) & bitposition) != bitposition){
			if (tries++ > 20) {
				syslog(LOG_ERR, "Cannot write to dio addr= 0x%x", addr);
				display_error_msg("E07:UNEXPECTED - DIO");
				exit(1);
			}

			mpoke16(addr, value | bitposition);
			usleep(100);
		};
	} else {
		while ( (mpeek16(addr) & bitposition) != 0x0){
			if (tries++ > 20) {
				syslog(LOG_ERR, "Cannot write to dio addr= 0x%x", addr);
				display_error_msg("E07:UNEXPECTED - DIO");
				exit(1);
			}

			mpoke16(addr, value & ~bitposition);
			usleep(100);
		};
	}

	pthread_mutex_unlock(&dio_lock);
}

