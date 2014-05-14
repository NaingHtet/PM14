/** @file error_handler.c
 *  @brief The functions for handling errors
 *  
 *  The functions for handling errors. This is to effectively pause the effected threads and
 *	let main controller solve the problem. When the main controller finishes solving the problem,
 *	the paused threads will resume.
 *
 *  @author Naing Minn Htet <naingminhtet91@gmail.com>
 */


#include "error_handler.h"
#include <stdio.h>
#include <syslog.h>

//Initialize functions for erro handler. Mostly mutexes and condition variables
void error_handler_initialize() {
	pthread_mutex_init(&elock_main, NULL);
	pthread_cond_init(&econd_main, NULL);
	eflag_main = 0;

	pthread_mutex_init(&elock_i2c, NULL);
	pthread_cond_init(&econd_i2c, NULL);
	eflag_i2c = 0;
}

//Signal main thread to solve the problem. This wakes up the main thread.
void esignal_main() {
	pthread_mutex_lock(&elock_main);
	eflag_main= 1;
	syslog(LOG_ERR, "Signalled the main thread");
	pthread_cond_signal(&econd_main);
	pthread_mutex_unlock(&elock_main);
}

//Wait till i2c issue is resolved.
void ewait_i2c(){
	// now wait for i2c to resolve
	pthread_mutex_lock(&elock_i2c);
	while ( eflag_i2c ) pthread_cond_wait(&econd_i2c, &elock_i2c);
	pthread_mutex_unlock(&elock_i2c);
}

//Signals that i2c has encounted an error and signal main thread.
void esignal_i2c(){
	pthread_mutex_lock(&elock_i2c);
	eflag_i2c = 1;
	syslog(LOG_ERR, "I2C has encounted an error");
	pthread_mutex_unlock(&elock_i2c);
	esignal_main();
}

//Destroy mutexes and condition variables
void error_handler_destroy() {
	pthread_cond_destroy(&econd_main);
	pthread_mutex_destroy(&elock_main);
	pthread_cond_destroy(&econd_i2c);
	pthread_mutex_destroy(&elock_i2c);

}