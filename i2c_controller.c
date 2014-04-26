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

static const int VOLTAGE_BYTES = 2;
static const char VOLTAGE_CMD = 0x10;

static const int TEST_BYTES = 2;
static const char TEST_CMD = 0x17;

static const int TEMP_BYTES = 2;
static const char TEMP_CMD = 0x11;

static const char BYPASS_CMD = 0x00;

int i2c_fd;
pthread_mutex_t lock;

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
		printf("Error writing = %s\n", strerror(errno));
		return n;
	}
	return 1;
}

//Read data from i2c
int read_i2c(char* rd_buf, int no_rd_bytes) {
	int allzero = 1;
	int zerocount = 0;
	do {
		int n = read(i2c_fd, rd_buf, no_rd_bytes);
		if (n < 0) {
			printf("Error reading = %s\n", strerror( errno));
			return n;
		}

		//TODO(NAING) : remove when AMS firmware bug is gone
		//Basically read again if the returned data is all 0
		int i;
		for (i = 0; i < no_rd_bytes ; i++) {
			if ( rd_buf[i] != 0 ) {
				allzero = 0;
				break;
			} 
		}
		if (allzero) {
			fprintf(stderr, "I2C has just read all zero.\n");
			usleep(++zerocount * 1000);
		} else {
			zerocount = 0;
		}
	} while (allzero);
}

//Write command and read immediately
int rdwr_i2c(char cmd, char* response, int no_rd_bytes) {
	char wr_buf[1];
	wr_buf[0] = cmd;
	//wr_buf[1] = cmd;
	int n = write_i2c( wr_buf, 1);
	if (n < 0) {
		return n;
	}

	n = read_i2c(response, no_rd_bytes);
	if (n < 0) {
		return n;
	}
	//TODO(NAING) : remove when AMS firmware bug is gone
	usleep(100);
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
    if ( fail == 0 ) printf("Not fail anymore\n"); 
    pthread_mutex_unlock(&lock);

    return fail;

}

//Get the voltage from device
double get_voltage(int addr) {

    pthread_mutex_lock(&lock);
    //LOCKED
	set_i2c_address(addr);
	char r_str[VOLTAGE_BYTES];
	int n = rdwr_i2c(VOLTAGE_CMD, r_str, VOLTAGE_BYTES);
	//UNLOCKED
    pthread_mutex_unlock(&lock);

    if ( n < 0) {
    	esignal_i2c();
    	return -1.0;
    }


	double r_d = ((uint16_t)r_str[0] << 8) + r_str[1];
	r_d = r_d * 6 / 1000;
	//CALIBRATION
	// v_d = v_d * 2 / 1000;
	// //uint16_t v_int = ((uint16_t)v_str[0] << 8) + v_str[1];
	// printf("%x\n", v_d);


	return r_d;
}


int get_voltage_all(double* r_d) {
	int i;
	for ( i = 0 ; i < i2c_count ; i++ ) {
		r_d[i] = get_voltage(i2c_addr[i]) ;
		if (r_d[i] == -1.0) {
			return -1;
		}
	}
	return 0;
}

//Get test code from device
double get_testcode(int addr) {
    pthread_mutex_lock(&lock);
    //LOCKED
    set_i2c_address(addr);
	char r_str[TEST_BYTES];
	rdwr_i2c(TEST_CMD, r_str, TEST_BYTES);
	//UNLOCKED
    pthread_mutex_unlock(&lock);

	double r_d = ((uint16_t)r_str[0] << 8) + r_str[1];
	return r_d;
}

double get_temperature(int addr) {
    pthread_mutex_lock(&lock);
    //LOCKED
  	set_i2c_address(addr);
	char r_str[TEMP_BYTES];
	rdwr_i2c(TEMP_CMD, r_str, TEMP_BYTES);
	//UNLOCKED
    pthread_mutex_unlock(&lock);

	double r_d = ((uint16_t)r_str[0] << 8) + r_str[1];
	r_d = (r_d-250)/5;

	return r_d;
}
void get_temperature_all(double* r_d) {
	int i;
	for ( i = 0 ; i < i2c_count ; i++ ) {
		r_d[i] = get_temperature(i2c_addr[i]) ;
	}
}

void set_bypass_state(int addr, char state) {
    pthread_mutex_lock(&lock);
    //LOCKED
	set_i2c_address(addr);
	char buf[3];
	buf[0] = BYPASS_CMD;
	buf[1] = 0x00;
	buf[2] = state;
	write_i2c(buf, 3);
	//UNLOCKED
    pthread_mutex_unlock(&lock);

}