#include "errorhandler.h"
#include <stdio.h>
void error_handler_init() {
	pthread_mutex_init(&elock_main, NULL);
	pthread_cond_init(&econd_main, NULL);
	eflag_main = 0;

	pthread_mutex_init(&elock_i2c, NULL);
	pthread_cond_init(&econd_i2c, NULL);
	eflag_i2c = 0;
}

void esignal_main() {
	pthread_mutex_lock(&elock_main);
	eflag_main= 1;
	printf("signalled!\n");
	pthread_cond_signal(&econd_main);
	pthread_mutex_unlock(&elock_main);
}

void ewait_i2c(){
	// now wait for i2c to resolve
	pthread_mutex_lock(&elock_i2c);
	while ( eflag_i2c ) pthread_cond_wait(&econd_i2c, &elock_i2c);
	pthread_mutex_unlock(&elock_i2c);
}

void esignal_i2c(){
	pthread_mutex_lock(&elock_i2c);
	eflag_i2c = 1;
	pthread_mutex_unlock(&elock_i2c);
	esignal_main();
}
