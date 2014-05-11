#include "watchdog_feeder.h"
#include "dio.h"

#define WATCHDOG_INTERVAL 300000

void watchdog_feeder_initialize() {
	mpeekpoke16(DIO_DIRECTION, DIO_WATCHDOG, DIO_ON);
}

void *feed_watchdog(void *arg){
	int* PROGRAM_RUNNING= arg;

	while(*PROGRAM_RUNNING) {
		mpeekpoke16(DIO_OUTPUT, DIO_WATCHDOG, DIO_OFF);
		usleep(WATCHDOG_INTERVAL);
		mpeekpoke16(DIO_OUTPUT, DIO_WATCHDOG, DIO_ON);
		usleep(WATCHDOG_INTERVAL);
	}

}
