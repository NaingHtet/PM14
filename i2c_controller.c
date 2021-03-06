/** @file i2c_controller.c
 *  @brief The controller for i2c.
 *
 *	Functions to communicate with AMS and current and pack sensors through i2c
 *
 *  @author Naing Minn Htet <naingminhtet91@gmail.com>
 */


#include <stdio.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <fcntl.h>
#include <termios.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>
#include <pthread.h>
#include <syslog.h>

#include "i2c_controller.h"
#include "error_handler.h"
#include "display_controller.h"
#include "dio.h"

static const char I2C_PORTNAME[] = "/dev/i2c-0";


static const int VOLTAGE_BYTES = 2;
static const char VOLTAGE_CMD = 0x10;

static const int GET_BYPASS_BYTES = 2;
static const int GET_BYPASS_CMD = 0x14;

static const int TEST_BYTES = 2;
static const char TEST_CMD = 0x17;

static const int TEMP_BYTES = 2;
static const char TEMP_CMD = 0x11;

static const char BYPASS_CMD = 0x00;

static const char OVERALL_CURRENT_REG = 0x00;
static const char OVERALL_VOLTAGE_REG = 0x04;

int i2c_fd; //file descriptor for i2c
pthread_mutex_t lock; // mutex lock for i2c so that threads don't try to send i2c commands at the same time
int cur_addr; //The current address i2c is set to


//Initiate the mutex
void initiate_mutex() {
	if (pthread_mutex_init(&lock, NULL) != 0)
    {
        syslog(LOG_ERR, "i2c mutex init failed\n");
        display_error_msg("E07-UNEXPECTED");
        exit(1);
    }
}
//Open the i2c port
void open_i2c_port() {
	i2c_fd = open( I2C_PORTNAME, O_RDWR);
	if ( i2c_fd < 0 ) {
		syslog(LOG_ERR, "Error Opening i2c port");
        display_error_msg("E07-UNEXPECTED");
		exit(1);
	}
	initiate_mutex();
}

//Initiate i2c controller functions
void i2c_controller_initialize() {
	open_i2c_port();
	initiate_mutex();

	//Enable AMSReset
	mpeekpoke16(DIO_DIRECTION, DIO_AMSRESET, DIO_ON);
	mpeekpoke16(DIO_OUTPUT, DIO_AMSRESET, DIO_ON);
	cur_addr = -1;
}

//Free memory and destroy mutex
void i2c_controller_destroy() {
	close(i2c_fd);
	pthread_mutex_destroy(&lock);

    free(i2c_addr);
    free(v_calib_a);
    free(v_calib_b);
    free(t_calib_a);
    free(t_calib_b);
}

//Set the address of the i2c device we want to communicate
void set_i2c_address(int addr) {
	if ( cur_addr == addr) return;
	else cur_addr = addr;
	int io = ioctl(i2c_fd, I2C_SLAVE, addr);
	if (io < 0) {
        syslog(LOG_ERR, "Error setting i2c address");
        display_error_msg("E07-UNEXPECTED");
		exit(1);
	}
}

//Write data to i2c device
int write_i2c(char* buf, int no_wr_bytes) {
	int n = write(i2c_fd, buf, no_wr_bytes);
	if (n < 0) {
		syslog(LOG_ERR,"Error writing = %s\n", strerror(errno));
		return n;
	}
	return 1;
}

//Read data from i2c
int read_i2c(char* rd_buf, int no_rd_bytes) {

	int n = read(i2c_fd, rd_buf, no_rd_bytes);
	if (n < 0) {
		syslog(LOG_ERR, "Error reading = %s\n", strerror( errno));
		return n;
	}

	return 1;
}

//Write command and read immediately
int rdwr_i2c(char cmd, char* response, int no_rd_bytes, int zerocheck) {
	char wr_buf[1];
	wr_buf[0] = cmd;
	//wr_buf[1] = cmd;
	int allzero = 1;
	int zerocount = 0;


	do {
		int n = write_i2c( wr_buf, 1);
		if (n < 0) {
			return n;
		}

		n = read_i2c(response, no_rd_bytes);
		if (n < 0) {
			return n;
		}

		int i;
		for (i = 0; i < no_rd_bytes ; i++) {
			if ( response[i] != 0 ) {
				allzero = 0;
				break;
			} 
		}

		if (!zerocheck) allzero = 0;

		//So AMS firmware for 2013 has an error. Sometimes it reads all zero. Sometimes it reads some random numbers
		//So as any good programmer would do, I bashed how terrible the firmware was and thought I could do better.
		//You will probably do the same to me. Such is Karma.

		//Anyway, ahem. to patch the reading all zero problem, the program will send command and read again for 20 times.
		//After that, it will reset the board before it tries to read again. If this still goes on for 25 time, raise error.
		if (allzero) {
			syslog(LOG_ERR,"I2C has just read all zero.");
			usleep(++zerocount * 1000);
			if ( zerocount > 20 ) {
				syslog(LOG_ERR, "Resetting i2c");
				mpeekpoke16(DIO_OUTPUT, DIO_AMSRESET, DIO_OFF);
				usleep(100);
				mpeekpoke16(DIO_OUTPUT, DIO_AMSRESET, DIO_ON);

			} else if ( zerocount > 25 ){
				syslog(LOG_ERR, "Cannot read correct value from I2C");
				display_error_msg("E04:AMS ERROR");
				exit(1);
			}


		} else {
			zerocount = 0;
		}

	} while(allzero);

	//TODO(NAING) : remove when AMS firmware bug is gone
	usleep(100);

	return 1;
}


//Check if all the addresses are connected.
int test_all_addresses() {
    pthread_mutex_lock(&lock);
    int i;
    int fail = 0;
    for (i = 0 ; i < i2c_count ; i++) {
    	set_i2c_address(i2c_addr[i]);
		int n = read(i2c_fd, NULL, 0);
		if ( n < 0 ) {
			fail = -1;
			syslog(LOG_ERR, "Cannot connect to i2c address 0x%x", i2c_addr[i]);
		}
    }

    set_i2c_address(charging_current_addr);
	int n = read(i2c_fd, NULL, 0);
	if ( n < 0 ){
		syslog(LOG_ERR, "Cannot connect to i2c address 0x%x", charging_current_addr);
		fail = -1;
	} 

    set_i2c_address(discharging_current_addr);
	n = read(i2c_fd, NULL, 0);
	if ( n < 0 ){
		syslog(LOG_ERR, "Cannot connect to i2c address 0x%x", discharging_current_addr);
		fail = -1;
	} 


    pthread_mutex_unlock(&lock);

    return fail;

}


//Get the voltage from device
double get_voltage(int cellno) {

	//check flag
    pthread_mutex_lock(&elock_i2c);
    if (eflag_i2c) {
    	pthread_mutex_unlock(&elock_i2c);
    	return -1.0;
    }
    pthread_mutex_unlock(&elock_i2c);


    pthread_mutex_lock(&lock);
    //LOCKED
	set_i2c_address(i2c_addr[cellno]);
	char r_str[VOLTAGE_BYTES];
	int n = rdwr_i2c(VOLTAGE_CMD, r_str, VOLTAGE_BYTES, 1);
	//UNLOCKED
    pthread_mutex_unlock(&lock);

    if ( n < 0) {
    	esignal_i2c();
    	return -1.0;
    }


	double r_d = ((uint16_t)r_str[0] << 8) + r_str[1];
	r_d = r_d * 6 / 1000;

	//calibration
	r_d = (r_d * v_calib_a[cellno]) + v_calib_b[cellno];

	return r_d;
}

//Get charging current and discharging current from device
//current1 is charging and current2 is discharging
int get_current(double* r_d) {
	//check flag
    pthread_mutex_lock(&elock_i2c);
    if (eflag_i2c) {
    	pthread_mutex_unlock(&elock_i2c);
    	return -1.0;
    }
    pthread_mutex_unlock(&elock_i2c);

    pthread_mutex_lock(&lock);
    //LOCKED
	set_i2c_address(charging_current_addr);
	char r_str[2];
	int n = rdwr_i2c(OVERALL_CURRENT_REG, r_str, 2, 0);
	//UNLOCKED
    pthread_mutex_unlock(&lock);

    if ( n < 0) {
    	esignal_i2c();
    	return -1;
    }


	double current1 = ((uint16_t)r_str[0] << 4) + ((uint16_t)r_str[1] >> 4);
	current1 = current1 / 6.0;
	current1 = (current1 * cc_calib_a) + cc_calib_b;

    pthread_mutex_lock(&lock);
    //LOCKED
	set_i2c_address(discharging_current_addr);
	n = rdwr_i2c(OVERALL_CURRENT_REG, r_str, 2, 0);
	//UNLOCKED
    pthread_mutex_unlock(&lock);

    if ( n < 0) {
    	esignal_i2c();
    	return -1;
    }

	double current2 = ((uint16_t)r_str[0] << 4) + ((uint16_t)r_str[1] >> 4);
	current2 = current2 / 6.0;
	current2 = (current2 * dc_calib_a) + dc_calib_b;

	r_d[0] = current1;
	r_d[1] = current2;
    pthread_mutex_unlock(&lock);

}

//Get the pack voltage
int get_overall_voltage(double* r_d) {
	//check flag
    pthread_mutex_lock(&elock_i2c);
    if (eflag_i2c) {
    	pthread_mutex_unlock(&elock_i2c);
    	return -1.0;
    }
    pthread_mutex_unlock(&elock_i2c);

    pthread_mutex_lock(&lock);
    //LOCKED
	set_i2c_address(0x6c);
	char r_str[6];
	int n = rdwr_i2c(0x00, r_str, 6, 1);
	//UNLOCKED
    pthread_mutex_unlock(&lock);

    if ( n < 0) {
    	esignal_i2c();
    	return -1;
    }

    double voltage = ((uint16_t)r_str[4] << 4) + ((uint16_t)r_str[5] >> 4);
    voltage = voltage * 0.0005 * 16.0;

    //Pack voltage is not calibrated because 1) we don't need to 2)senioritis
    *r_d = voltage;
    pthread_mutex_unlock(&lock);

}

//Get all the voltage from all cells
int get_voltage_all(double* r_d) {
	int i;
	for ( i = 0 ; i < i2c_count ; i++ ) {
		r_d[i] = get_voltage(i) ;

		if (r_d[i] == -1.0) {
			return -1;
		}
	}
	return 0;
}

// //Get test code from device
// double get_testcode(int addr) {
//     pthread_mutex_lock(&lock);
//     //LOCKED
//     set_i2c_address(addr);
// 	char r_str[TEST_BYTES];
// 	rdwr_i2c(TEST_CMD, r_str, TEST_BYTES);
// 	//UNLOCKED
//     pthread_mutex_unlock(&lock);

// 	double r_d = ((uint16_t)r_str[0] << 8) + r_str[1];
// 	return r_d;
// }

//Get the temperature
double get_temperature(int cellno) {
	//check flag
    pthread_mutex_lock(&elock_i2c);
    if (eflag_i2c) {
    	pthread_mutex_unlock(&elock_i2c);
    	return -1.0;
    }
    pthread_mutex_unlock(&elock_i2c);


    pthread_mutex_lock(&lock);
  	set_i2c_address(i2c_addr[cellno]);
	char r_str[TEMP_BYTES];
	int n = rdwr_i2c(TEMP_CMD, r_str, TEMP_BYTES, 1);
    pthread_mutex_unlock(&lock);

    if ( n < 0) {
    	esignal_i2c();
    	return -1.0;
    }

	double r_d = ((uint16_t)r_str[0] << 8) + r_str[1];
	r_d = (r_d-250)/5;

	//calibration
	r_d = (r_d * t_calib_a[cellno]) + t_calib_b[cellno];
	return r_d;
}

//Check the state of the bypass for the cell
int get_bypass_state(int addr) {

	//check flag
    pthread_mutex_lock(&elock_i2c);
    if (eflag_i2c) {
    	pthread_mutex_unlock(&elock_i2c);
    	return -1;
    }
    pthread_mutex_unlock(&elock_i2c);


    pthread_mutex_lock(&lock);
  	set_i2c_address(addr);
	char r_str[GET_BYPASS_BYTES];
	int n = rdwr_i2c(GET_BYPASS_CMD, r_str, GET_BYPASS_BYTES, 0);
    pthread_mutex_unlock(&lock);

    if ( n < 0) {
    	esignal_i2c();
    	return -1;
    }

	int r_d = r_str[1];

	return r_d;
}

//Get bypass states of all the cells
int get_bypass_state_all(int* r_d) {
	int i;
	for ( i = 0 ; i < i2c_count ; i++ ) {
		r_d[i] = get_bypass_state(i2c_addr[i]) ;
		if (r_d[i] == -1) {
			return -1;
		}
	}

}

//Get temperatures of all cells
int get_temperature_all(double* r_d) {
	int i;
	for ( i = 0 ; i < i2c_count ; i++ ) {
		r_d[i] = get_temperature(i) ;
		if (r_d[i] == -1.0) {
			return -1;
		}
	}
}

//Set the bypass state of the cell
int set_bypass_state(int addr, char state) {
	//check flag
    pthread_mutex_lock(&elock_i2c);
    if (eflag_i2c) {
    	pthread_mutex_unlock(&elock_i2c);
    	return -1;
    }
    pthread_mutex_unlock(&elock_i2c);

    pthread_mutex_lock(&lock);
    //LOCKED
	set_i2c_address(addr);
	char buf[3];
	buf[0] = BYPASS_CMD;
	buf[1] = 0x00;
	buf[2] = state;
	int n = write_i2c(buf, 3);
	//UNLOCKED
    pthread_mutex_unlock(&lock);

    if ( n < 0) {
    	esignal_i2c();
    	return -1;
    }
    return n;
}

//Set all cells to a bypass state
int set_bypass_state_all(char state) {
	int i;
	for ( i = 0 ; i < i2c_count ; i++ ) {
		if (set_bypass_state(i2c_addr[i], state) < 0) {
			return -1;
		}
	}
}
