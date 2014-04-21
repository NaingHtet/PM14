#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>


#include "i2c_controller.h"
#include "serial_controller.h"
#include "safety_checker.h"
#include "display_controller.h"
#include "lib/libconfig.h"


void loadConfig() {
    config_t cfg;
    config_setting_t *setting;
    const char *str;

    config_init(&cfg);

    /* Read the file. If there is an error, report it and exit. */
    if(! config_read_file(&cfg, "pm14.cfg")) {
        fprintf(stderr, "%s:%d - %s\n", config_error_file(&cfg),
                config_error_line(&cfg), config_error_text(&cfg));
        config_destroy(&cfg);
        return;
    }

    //Read i2c_addresses
  	setting = config_lookup(&cfg, "i2c_address");
  	if(setting != NULL)
  	{
    	int count = config_setting_length(setting);
    	int i;
    	i2c_count = count;
		i2c_addr = malloc(i2c_count*sizeof(int));
    	for ( i = 0 ; i < count ; i++ ) {
    		i2c_addr[i] = config_setting_get_int_elem(setting, i);
    	}
    }
    else fprintf (stderr, "No 'i2c_address' in configuration file");

    setting = config_lookup(&cfg, "safebounds");
    if (setting != NULL)
    {
    	config_setting_t *bounds;
    	bounds = config_setting_get_member(setting, "voltage");
    	if (bounds != NULL) {
    		if (config_setting_lookup_float(bounds, "high", &SAFE_V_HIGH) == CONFIG_FALSE) 
    			fprintf (stderr, "Cannot lookup voltage.high\n");
    		if (config_setting_lookup_float(bounds, "low", &SAFE_V_LOW) == CONFIG_FALSE) 
    			fprintf (stderr, "Cannot lookup voltage.low\n");
    	}
    	else fprintf (stderr, "No voltage bounds in configuration file");

    	bounds = config_setting_get_member(setting, "temperature");
    	if (bounds != NULL) {
    		if (config_setting_lookup_float(bounds, "high", &SAFE_T_HIGH) == CONFIG_FALSE) 
    			fprintf (stderr, "Cannot lookup temperature.high\n");
    		if (config_setting_lookup_float(bounds, "low", &SAFE_T_LOW) == CONFIG_FALSE) 
    			fprintf (stderr, "Cannot lookup temperature.low\n");
    	}
    	else fprintf (stderr, "No temperature bounds in configuration file");
    }
    else fprintf (stderr, "No 'safebounds' in configuration file");

    printf("%f %f %f %f\n" , SAFE_V_HIGH, SAFE_V_LOW, SAFE_T_HIGH, SAFE_T_LOW);
	PACK_NO = 1;

	config_destroy(&cfg);
}

int main() {

	loadConfig();
	lcdinit(i2c_count);
	enable_serial_fpga();
	open_serial_port();
	
	open_i2c_port();
	printf("%d, %d", i2c_count, i2c_addr[0]);
	
	pthread_t safety_thread, serial_thread, display_thread;
	pthread_create(&safety_thread, NULL, check_safety, NULL);
	

	pthread_create(&serial_thread, NULL, handle_serial, NULL);
	pthread_create(&display_thread, NULL, display_LCD, NULL);

	pthread_join(safety_thread, NULL);
	
	

}
