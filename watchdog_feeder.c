#include "watchdog_feeder.h"
#include "dio.h"

void *feed_watchdog(void *arg){
	mpeekpoke16(0x0008, DIO_WATCHDOG, 1);

	while(1) {
		mpeekpoke16(0x0004, DIO_WATCHDOG, 0);
		usleep(200000);
		mpeekpoke16(0x0004, DIO_WATCHDOG, 1);
		usleep(200000);
	}
}
