#include <pthread.h>


pthread_mutex_t elock_i2c;
pthread_cond_t econd_i2c;
char eflag_i2c;

void ewait_i2c();
void esignal_i2c();