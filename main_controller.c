/** @file main_controller.c
 *  @brief The main controller for Pack Manager 14
 *  
 *  This is the main controller for Pack Manager 14. This initializes the program parameters and starts the different functions of the program.  
 *
 *  @author Naing Minn Htet <naingminhtet91@gmail.com>
 */



#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <syslog.h>

#include "i2c_controller.h"
#include "serial_controller.h"
#include "pack_controller.h"
#include "display_controller.h"
#include "error_handler.h"
#include "lib/libconfig.h"
#include "watchdog_feeder.h"
#include "dio.h"
#include "httpserver.h"

int PROGRAM_RUNNING;

int load_config();
void save_config();

//The program is quiting. Abandon ship!
void quit_handler(int signum){
    PROGRAM_RUNNING = 0;
    esignal_main();
}

//This will log/unlog the data.
void log_handler(int signum){
    if (!LOG_FILE) {
        log_fp =  fopen("out.dat", "a");
        fprintf(log_fp, "Start Logging. The columns are:\n");
        fprintf(log_fp, "Curtime Charging Plugged Ccount realsoc dispsoc current packV packT Bypass_of_all_cells Voltage_of_all_cells Temperature_of_all_cells\n");
        LOG_FILE = 1;
    } else {
        fprintf(log_fp, "Stopped Logging.\n");
        fclose(log_fp);
        LOG_FILE = 0;
    }
}

//Initialize for main controller
void main_initialize() {

    //Load config file
    syslog(LOG_INFO, "Loading configuration file");
    if ( load_config() < 0) {
        display_error_msg("E02:BAD CONFIG");
        exit(1);
    }
    syslog(LOG_INFO, "Successfully loaded configuration file");


    //Handle QUIT signal
    PROGRAM_RUNNING = 1;
    struct sigaction SA1, SA2;
    SA1.sa_handler = quit_handler;
    if ( sigaction(SIGQUIT, &SA1, NULL) == -1) {
        syslog(LOG_ERR, "Failed to handle signal SIGQUIT");
        display_error_msg("E07:UNEXPECTED");
        exit(1);
    }

    //Handle SIGUSR1 (LOGGING) signal
    LOG_FILE = 0;
    SA2.sa_handler = log_handler;
    if ( sigaction(SIGUSR1, &SA2, NULL) == -1) {
        syslog(LOG_ERR, "Failed to handle signal SIGUSR1");
        display_error_msg("E07:UNEXPECTED");
        exit(1);
    }

}

int main() {
    
    /**
     * Making program daemon process. START
     */

    pid_t pid, sid;
    /* Fork off the parent process */
    pid = fork();
    if (pid < 0) {
        exit(EXIT_FAILURE);
    }
    /* If we got a good PID, then
       we can exit the parent process. */
    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }

    // Write PID to file.
    pid = getpid();
    FILE* fp;
    fp = fopen("/var/run/pm14.pid","w");
    fprintf(fp,"%d",pid);
    fclose(fp);


    /* Change the file mode mask */
    umask(0);       
    
    /* Open any logs here */
    openlog("pm14", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
    syslog(LOG_NOTICE, "Program started by User %d", getuid());

    /* Create a new SID for the child process */
    sid = setsid();
    if (sid < 0) {
            syslog(LOG_ERR, "Cannot create SID for child process.");
            exit(EXIT_FAILURE);
    }
    
    
    /* Change the current working directory */
    if ((chdir("/root/PM14")) < 0) {
            syslog(LOG_ERR, "Cannot change working directory.");
            exit(EXIT_FAILURE);
    }

    /* Close out the standard file descriptors */
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    /**
     * Making program daemon process. END
     */


     //initialize everything!
    display_controller_initialize();

    main_initialize();

    dio_initialize();

    error_handler_initialize();

    serial_controller_initialize();

    i2c_controller_initialize();

    watchdog_feeder_initialize();

    pack_controller_initialize();


    //CREATE ALL THE THREADS!
    pthread_t watchdog_thread, pack_thread, serial_thread, display_thread, server_thread;

    pthread_create(&watchdog_thread, NULL, feed_watchdog, &PROGRAM_RUNNING);
    pthread_create(&pack_thread, NULL, control_pack, &PROGRAM_RUNNING);
    pthread_create(&serial_thread, NULL, handle_serial, NULL);
    pthread_create(&display_thread, NULL, display_LCD, &PROGRAM_RUNNING);
    // pthread_create(&server_thread, NULL, handle_server, NULL);


    //WAIT AND SOLVE i2c problem. 
    while (PROGRAM_RUNNING) {
        pthread_mutex_lock(&elock_main);
        while ( !eflag_main ) pthread_cond_wait(&econd_main, &elock_main);
        pthread_mutex_unlock(&elock_main);

        mpeekpoke16(DIO_OUTPUT, DIO_SAFETY, DIO_ON);

        int n = -1;
        while (PROGRAM_RUNNING && (n < 0)) {
            n = test_all_addresses();            
            sleep(1);
        }

        mpeekpoke16(DIO_OUTPUT, DIO_SAFETY, DIO_OFF);

        pthread_mutex_lock(&elock_i2c);
        eflag_i2c = 0;
        pthread_cond_signal(&econd_i2c);
        pthread_mutex_unlock(&elock_i2c);

        pthread_mutex_lock(&elock_main);
        eflag_main = 0;
        pthread_mutex_unlock(&elock_main);

        syslog(LOG_NOTICE, "Reconnected to AMS successfully.");

    }
    mpeekpoke16(DIO_OUTPUT, DIO_SAFETY, DIO_ON);
    //Program ending!!

    //Cancel or wait for all the threads
    pthread_cancel(serial_thread);
    pthread_join(pack_thread,NULL);
    pthread_join(display_thread,NULL);
    pthread_join(watchdog_thread,NULL);

    //Save the config and destroy things that need to be
    close_serial();
    save_config();
    error_handler_destroy();
    dio_destroy();
    i2c_controller_destroy();

    syslog(LOG_NOTICE, "Quit successfully");

    closelog();
}

// Load the configuration file
int load_config() {
    config_t cfg;
    config_setting_t *setting;
    const char *str;

    config_init(&cfg);

    /* Read the file. If there is an error, report it and exit. */
    if(! config_read_file(&cfg, "pm14.cfg")) {
        syslog(LOG_ERR, "%s:%d - %s", config_error_file(&cfg),
                config_error_line(&cfg), config_error_text(&cfg));
        config_destroy(&cfg);
        display_error_msg("E01:NO CONFIG");
        exit(1);
    }

    //Read i2c_addresses
  	setting = config_lookup(&cfg, "i2c_parameters");
  	if(setting != NULL)
  	{
    	int count = config_setting_length(setting);
    	int i;

    	i2c_count = count;
		i2c_addr = malloc(i2c_count*sizeof(int));
        v_calib_a = malloc(i2c_count*sizeof(double));
        v_calib_b = malloc(i2c_count*sizeof(double));
        t_calib_a = malloc(i2c_count*sizeof(double));
        t_calib_b = malloc(i2c_count*sizeof(double));

        config_setting_t *i2c_param;
        config_setting_t *calib_param;

    	for ( i = 0 ; i < count ; i++ ) {
    		i2c_param = config_setting_get_elem(setting, i);
            if (i2c_param != NULL) {
                if (config_setting_lookup_int(i2c_param, "address", &i2c_addr[i]) == CONFIG_FALSE) {
                    syslog(LOG_ERR, "Cannot lookup address");
                    return -1;
                }
                calib_param = config_setting_get_member(i2c_param, "voltage_calib");
                if (calib_param != NULL) {
                    if (config_setting_lookup_float(calib_param, "a", &v_calib_a[i]) == CONFIG_FALSE) {
                        syslog(LOG_ERR, "Cannot lookup voltage_calib.a");
                        return -1;
                    }
                    if (config_setting_lookup_float(calib_param, "b", &v_calib_b[i]) == CONFIG_FALSE) {
                        syslog(LOG_ERR, "Cannot lookup voltage_calib.b");
                        return -1;
                    }
                }
                calib_param = config_setting_get_member(i2c_param, "temperature_calib");
                if (calib_param != NULL) {
                    if (config_setting_lookup_float(calib_param, "a", &t_calib_a[i]) == CONFIG_FALSE) {
                        syslog(LOG_ERR, "Cannot lookup temperature_calib.a");
                        return -1;
                    }
                    if (config_setting_lookup_float(calib_param, "b", &t_calib_b[i]) == CONFIG_FALSE) {
                        syslog(LOG_ERR, "Cannot lookup temperature_calib.b");
                        return -1;
                    }
                }
            }
    	}
    }
    else {
        syslog(LOG_ERR, "No 'i2c_parameters' in configuration file");
        return -1;
    }


    setting = config_lookup(&cfg, "safebounds");
    if (setting != NULL)
    {
    	config_setting_t *bounds;
    	bounds = config_setting_get_member(setting, "voltage");
    	if (bounds != NULL) {
    		if (config_setting_lookup_float(bounds, "high", &SAFE_V_HIGH) == CONFIG_FALSE) {
    			syslog(LOG_ERR, "Cannot lookup voltage.high");
                return -1;
            }
    		if (config_setting_lookup_float(bounds, "low", &SAFE_V_LOW) == CONFIG_FALSE) {
    			syslog(LOG_ERR, "Cannot lookup voltage.low");
                return -1;
            }
    	}
    	else {
            syslog(LOG_ERR, "No voltage bounds in configuration file");
            return -1;
        }

    	bounds = config_setting_get_member(setting, "temperature");
    	if (bounds != NULL) {
    		if (config_setting_lookup_float(bounds, "normal_high", &SAFE_T_NORMAL) == CONFIG_FALSE) {
    			syslog(LOG_ERR, "Cannot lookup temperature.normal_high");
                return -1;
            }
    		if (config_setting_lookup_float(bounds, "charging_high", &SAFE_T_CHARGING) == CONFIG_FALSE) {
    			syslog(LOG_ERR, "Cannot lookup temperature.charging_high");
                return -1;
            }
    	}
    	else {
            syslog(LOG_ERR, "No temperature bounds in configuration file");
            return -1;
        }

        if (config_setting_lookup_float(setting, "charging_coulomb_limit", &CHARGING_COULOMB_LIMIT) == CONFIG_FALSE) {
            syslog(LOG_ERR, "Cannot lookup voltage.high");
            return -1;
        }
        if (config_setting_lookup_float(setting, "charging_time_limit", &CHARGING_TIME_LIMIT) == CONFIG_FALSE) {
            syslog(LOG_ERR, "Cannot lookup voltage.low");
            return -1;
        }
    }
    else {
        syslog(LOG_ERR, "No 'safebounds' in configuration file");
        return -1;
    }

    setting = config_lookup(&cfg, "state_of_charge_parameters");
    if (setting != NULL) {

        double SOC, gain, bias, learning_rate;

        if (config_setting_lookup_float(setting, "SOC", &SOC) == CONFIG_FALSE) {
            syslog(LOG_ERR, "Cannot lookup SOC");
            return -1;
        }
        if (config_setting_lookup_float(setting, "gain", &gain) == CONFIG_FALSE) {
            syslog(LOG_ERR, "Cannot lookup gain");
            return -1;
        }

        if (config_setting_lookup_float(setting, "bias", &bias) == CONFIG_FALSE) {
            syslog(LOG_ERR, "Cannot lookup bias");
            return -1;
        }

        if (config_setting_lookup_float(setting, "learning_rate", &learning_rate) == CONFIG_FALSE) {
            syslog(LOG_ERR, "Cannot lookup learning_rate");
            return -1;
        }

        set_state_of_charge(SOC, gain, bias, learning_rate);
    }
    else {
        syslog(LOG_ERR, "No state of charge parameters");
        return -1;
    }

    setting = config_lookup(&cfg, "charging_current_parameters");
    if (setting != NULL) {
        
        if (config_setting_lookup_int(setting, "address", &charging_current_addr) == CONFIG_FALSE) {
            syslog(LOG_ERR, "Cannot lookup charging_current_addr");
            return -1;
        }

        config_setting_t *params;
        params = config_setting_get_member(setting, "current_calib");
        if (params != NULL) {
            if (config_setting_lookup_float(params, "a", &cc_calib_a) == CONFIG_FALSE) {
                syslog(LOG_ERR, "Cannot lookup cc_calib_a");
                return -1;
            }
            if (config_setting_lookup_float(params, "b", &cc_calib_b) == CONFIG_FALSE) {
                syslog(LOG_ERR, "Cannot lookup cc_calib_b");
                return -1;
            }

        } else {
            syslog(LOG_ERR, "No calibration for current");
            return -1;
        }
    }
    else {
        syslog(LOG_ERR, "No charging current parameters");
        return -1;
    }

    setting = config_lookup(&cfg, "discharging_current_parameters");
    if (setting != NULL) {
        
        if (config_setting_lookup_int(setting, "address", &discharging_current_addr) == CONFIG_FALSE) {
            syslog(LOG_ERR, "Cannot lookup discharging_current_addr");
            return -1;
        }

        config_setting_t *params;
        params = config_setting_get_member(setting, "current_calib");
        if (params != NULL) {
            if (config_setting_lookup_float(params, "a", &dc_calib_a) == CONFIG_FALSE) {
                syslog(LOG_ERR, "Cannot lookup dc_calib_a");
                return -1;
            }
            if (config_setting_lookup_float(params, "b", &dc_calib_b) == CONFIG_FALSE) {
                syslog(LOG_ERR, "Cannot lookup dc_calib_b");
                return -1;
            }

        } else {
            syslog(LOG_ERR, "No calibration for discharging current");
            return -1;
        }
    }
    else {
        syslog(LOG_ERR, "No discharging current parameters");
        return -1;
    }

    if (config_lookup_int(&cfg, "pack_no", &PACK_NO) == CONFIG_FALSE) {
        syslog(LOG_ERR, "Cannot lookup pack_no");
        return -1;
    }

	config_destroy(&cfg);
    return 0;
}

//Save the new SOC parameters to the file
void save_config() {
    config_t cfg;
    config_setting_t *setting;
    const char *str;

    config_init(&cfg);

    /* Read the file. If there is an error, report it and exit. */
    if(! config_read_file(&cfg, "pm14.cfg")) {
        syslog(LOG_ERR, "%s:%d - %s", config_error_file(&cfg),
                config_error_line(&cfg), config_error_text(&cfg));
        config_destroy(&cfg);
        display_error_msg("E01:NO CONFIG");
        exit(1);
    }

    setting = config_lookup(&cfg, "state_of_charge_parameters");
    if (setting != NULL) {

        double SOC, gain, bias;
        get_state_of_charge(&SOC, &gain, &bias);

        config_setting_t *params;
        params = config_setting_get_member(setting, "SOC");
        if (params == NULL) {
            syslog(LOG_ERR, "Cannot lookup SOC");
            display_error_msg("E02:BAD CONFIG");
            exit(1);
        }
        config_setting_set_float(params, SOC);

        params = config_setting_get_member(setting, "gain");
        if (params == NULL) {
            syslog(LOG_ERR, "Cannot lookup gain");
            display_error_msg("E02:BAD CONFIG");
            exit(1);
        }
        config_setting_set_float(params, gain);

        params = config_setting_get_member(setting, "bias");
        if (params == NULL) {
            syslog(LOG_ERR, "Cannot lookup bias");
            display_error_msg("E02:BAD CONFIG");
            exit(1);
        }
        config_setting_set_float(params, bias);

    }
    else {
        syslog(LOG_ERR, "No state of charge parameters");
        display_error_msg("E02:BAD CONFIG");
        exit(1);
    }
    
    if(! config_write_file(&cfg, "pm14.cfg"))
    {
        syslog(LOG_ERR, "Error while writing file.");
        display_error_msg("E02:BAD CONFIG");
        config_destroy(&cfg);
        exit(1);
    }

    config_destroy(&cfg);

}
