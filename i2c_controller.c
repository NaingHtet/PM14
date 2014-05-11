#include <stdio.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <fcntl.h>
#include <termios.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>
#include <pthread.h>

#include "i2c_controller.h"
#include "error_handler.h"

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

int i2c_fd;
pthread_mutex_t lock;
int cur_addr;

void initiate_mutex() {
	if (pthread_mutex_init(&lock, NULL) != 0)
    {
        printf("\n mutex init failed\n");
        exit(1);
    }
}
//Open the i2c port
void open_i2c_port() {
	i2c_fd = open( I2C_PORTNAME, O_RDWR);
	if ( i2c_fd < 0 ) {
		printf("Error Opening i2c port");
		exit(1);
	}
	initiate_mutex();
}

void i2c_controller_initialize() {
	open_i2c_port();
	initiate_mutex();
	cur_addr = -1;
}

void i2c_controller_destroy() {
	close(i2c_fd);
	pthread_mutex_destroy(&lock);

    free(i2c_addr);
    free(v_calib_a);
    free(v_calib_b);
    free(t_calib_a);
    free(t_calib_b);
}

int cur_addr = 0;
//Set the address of the i2c device we want to communicate
void set_i2c_address(int addr) {
	if ( cur_addr == addr) return;
	else cur_addr = addr;
	int io = ioctl(i2c_fd, I2C_SLAVE, addr);
	if (io < 0) {
		printf("Error setting i2c address");
		exit(1);
	}
}

//Write data to i2c device
int write_i2c(char* buf, int no_wr_bytes) {
	int n = write(i2c_fd, buf, no_wr_bytes);
	if (n < 0) {
		fprintf(stderr,"Error writing = %s\n", strerror(errno));
		return n;
	}
	return 1;
}

//Read data from i2c
int read_i2c(char* rd_buf, int no_rd_bytes) {

	int n = read(i2c_fd, rd_buf, no_rd_bytes);
	if (n < 0) {
		fprintf(stderr, "Error reading = %s\n", strerror( errno));
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
			fprintf(stderr, "error in writing\n");
			return n;
		}

		n = read_i2c(response, no_rd_bytes);
		if (n < 0) {
			fprintf(stderr, "error in reading\n");
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

		if (allzero) {
			fprintf(stderr, "I2C has just read all zero.\n");
			usleep(++zerocount * 1000);
		} else {
			zerocount = 0;
		}

	} while(allzero);

	//TODO(NAING) : remove when AMS firmware bug is gone
	usleep(100);

	return 1;
}

int test_all_addresses(int* r_d) {
    pthread_mutex_lock(&lock);
    int i;
    int fail = 0;
    for (i = 0 ; i < i2c_count ; i++) {
    	set_i2c_address(i2c_addr[i]);
		int n = read(i2c_fd, NULL, 0);
		if ( n < 0 ) {
			fail = -1;
			r_d[i] = -1;
		} else r_d[i] = 0;
    }

    set_i2c_address(charging_current_addr);
	int n = read(i2c_fd, NULL, 0);
	if ( n < 0 ){
		printf("no c current sensor\n");
		fail = -1;
	} 

    set_i2c_address(discharging_current_addr);
	n = read(i2c_fd, NULL, 0);
	if ( n < 0 ){
		printf("no d current sensor\n");
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
	printf("charging current = %f\n", current1);

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

	printf("discharging current = %f\n", current2);

	*r_d = current1 - current2;
    pthread_mutex_unlock(&lock);

}

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
    	fprintf(stderr, "Error in voltage i2c\n");
    	esignal_i2c();
    	return -1;
    }

    double voltage = ((uint16_t)r_str[4] << 4) + ((uint16_t)r_str[5] >> 4);
    voltage = voltage * 0.0005 * 16.0;
    *r_d = voltage;
    pthread_mutex_unlock(&lock);

}

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

double get_temperature(int addr) {

	//check flag
    pthread_mutex_lock(&elock_i2c);
    if (eflag_i2c) {
    	pthread_mutex_unlock(&elock_i2c);
    	return -1.0;
    }
    pthread_mutex_unlock(&elock_i2c);


    pthread_mutex_lock(&lock);
  	set_i2c_address(addr);
	char r_str[TEMP_BYTES];
	int n = rdwr_i2c(TEMP_CMD, r_str, TEMP_BYTES, 1);
    pthread_mutex_unlock(&lock);

    if ( n < 0) {
    	esignal_i2c();
    	return -1.0;
    }

	double r_d = ((uint16_t)r_str[0] << 8) + r_str[1];
	r_d = (r_d-250)/5;

	return r_d;
}


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

int get_bypass_state_all(int* r_d) {
	int i;
	for ( i = 0 ; i < i2c_count ; i++ ) {
		r_d[i] = get_bypass_state(i2c_addr[i]) ;
		if (r_d[i] == -1) {
			return -1;
		}
	}

}

int get_temperature_all(double* r_d) {
	int i;
	for ( i = 0 ; i < i2c_count ; i++ ) {
		r_d[i] = get_temperature(i2c_addr[i]) ;
		if (r_d[i] == -1.0) {
			return -1;
		}
	}
}

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

int set_bypass_state_all(char state) {
	int i;
	for ( i = 0 ; i < i2c_count ; i++ ) {
		if (set_bypass_state(i2c_addr[i], state) < 0) {
			return -1;
		}
	}
}
