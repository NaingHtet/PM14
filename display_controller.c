#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <math.h>
#include "display_controller.h"
#include "i2c_controller.h"
#include "safety_checker.h"
#include <stdlib.h>

#define FPGABASE 0x30000000
#define SYSCON (0x0a /sizeof(unsigned short))
#define EN_V (0x402 /sizeof(unsigned short))
#define ODR (0x406 /sizeof(unsigned short))
#define DDR (0x40A /sizeof(unsigned short))
#define IDR (0x40E /sizeof(unsigned short))
 
#define MUXBUS 0x20
 
#define LCD_WR 0x100
#define LCD_RS 0x200
#define LCD_EN 0x400
 
#define LCD_PWR 0x1000
 
// These delay values are calibrated for the TS-4200
#define SETUP	200
#define PULSE	400
#define HOLD	200

#define FIRSTLINE 0x2
#define SECONDLINE 0xc0
#define THIRDLINE 0x94
#define FOURTHLINE 0xd4

#define COUNTDOWN(x)	asm volatile ( \
  "1:\n"\
  "subs %1, %1, #1;\n"\
  "bne 1b;\n"\
  : "=r" ((x)) : "r" ((x)) \
);
volatile unsigned short *syscon, *odr, *ddr, *idr, *en_v;

char error_msg[20];
double* voltage;
void command(unsigned int);
void writechars(unsigned char *);
unsigned int lcdwait(void);


int count;
pthread_mutex_t dlock;

 
void *display_LCD(void *arg) {
	sleep(2);

	int i = 0;
	double nr = ceil((double)(count/3.0));
	int norounds = (int) nr;
	while(1) {
		command(0x1);
		lcdwait();

		char wbuf[21];
		char xbuf[21];

		// double d[count];
		// int n = get_voltage_all(d);
		// if (n < 0) break;
		// double t[count];
		// get_temperature_all(t);
		// if (n < 0) break;


		double cur_I;
		double cur_V;
		int n = get_current(&cur_I);

		command(0x1);
		lcdwait();


		if ( n >= 0) {
			n = get_overall_voltage(&cur_V);
		}
		if ( n < 0 ) {
			command(THIRDLINE);
			writechars("E03:NO AMS");
			lcdwait();
		} else {			
			sprintf(wbuf, "C:%3.2f   V:%3.2f", cur_I, cur_V);
					// sprintf(xbuf, "T%d:%3.0f", x+1, t[x]);
			command(THIRDLINE);
			writechars(wbuf);
			lcdwait();

			// command(THIRDLINE);
			// writechars(xbuf);
			// lcdwait();
		}

		sprintf(xbuf, "SOC:%d   ", disp_SOC);
		if (charging_state) 
			sprintf(xbuf+ strlen(xbuf),"CHARGING");
		else sprintf(xbuf+ strlen(xbuf),"DISCHARGING");

		command(SECONDLINE);
		writechars(xbuf);
		lcdwait();


		command(FIRSTLINE);
		writechars("PACMAN 2014");
		lcdwait();

		if (!SYSTEM_SAFE) {
			command(FOURTHLINE);
			writechars("SYSTEM NOT SAFE");
		}

		// i = (i + 1)%norounds;
		sleep(2);
	}
}

void set_error_msg(char* msg) {
    pthread_mutex_lock(&dlock);
    //LOCKED
	strcpy(error_msg, msg);
    pthread_mutex_unlock(&dlock);
    //UNLOCKED
}

void initiate_dmutex() {
	if (pthread_mutex_init(&dlock, NULL) != 0)
    {
        printf("\n mutex init failed\n");
        exit(1);
    }
}
//xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
//BELOW IS THE CODE FROM TS8160-4200 WIKI
//xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

void lcdinit(int i2c_count) {
	count = i2c_count;
	initiate_dmutex();

	int fd = open("/dev/mem", O_RDWR|O_SYNC);
 
	syscon = (unsigned short *)mmap(0, getpagesize(), 
	  PROT_READ|PROT_WRITE, MAP_SHARED, fd, FPGABASE);
 
 
	odr = &syscon[ODR];
	ddr = &syscon[DDR];
	idr = &syscon[IDR];
	en_v = &syscon[EN_V];
	syscon = &syscon[SYSCON];
 
	*syscon = *syscon | 0x1000;
 
	*ddr = *ddr & 0xFF00; // data lines to inputs
	*ddr = *ddr | LCD_WR | LCD_RS | LCD_EN; // control lines to outputs
	*odr &= ~(LCD_EN | LCD_RS);
	*odr |= LCD_WR;
 	*en_v |= 0x1000;

	usleep(15000);
	command(0x38); // two rows, 5x7, 8 bit
	usleep(4100);
	command(0x38); // two rows, 5x7, 8 bit
	usleep(100);
	command(0x38); // two rows, 5x7, 8 bit
	command(0x6); // cursor increment mode
	lcdwait();
	command(0x1); // clear display
	lcdwait();
	command(0xc); // display on, blink off, cursor off
	lcdwait();
	command(0x2); // return home
}
 
unsigned int lcdwait(void) {
	int i, dat, tries = 0;
	unsigned short ctrl;
 
	*ddr &= 0xff00; // data lines to inputs
	ctrl = *odr;
 
	do {
		// step 1, apply RS & WR
		ctrl |= LCD_WR; // de-assert WR
		ctrl &= ~LCD_RS; // de-assert RS
		*odr = ctrl;
 
		// step 2, wait
		i = SETUP;
		COUNTDOWN(i);
 
		// step 3, assert EN
		ctrl |= LCD_EN;
		*odr = ctrl;
 
		// step 4, wait
		i = PULSE;
		COUNTDOWN(i);
 
		// step 5, de-assert EN, read result
		dat = *idr & 0xff;
		ctrl &= ~LCD_EN; // de-assert EN
		*odr = ctrl;
 
		// step 6, wait
		i = HOLD;
		COUNTDOWN(i);
	} while (dat & 0x80 && tries++ < 1000);
	return dat;
}
 
void command(unsigned int cmd) {
	int i;
	unsigned short ctrl;
 
	*ddr = *ddr | 0x00ff; // set port A to outputs
	ctrl = *odr;
 
	// step 1, apply RS & WR, send data
 
	ctrl = ctrl & 0xFF00;
	ctrl = ctrl | (cmd & 0xFF);
	ctrl &= ~(LCD_RS | LCD_WR); // de-assert RS, assert WR
	*odr = ctrl;
 
 
	// step 2, wait
	i = SETUP;
	COUNTDOWN(i);
 
	// step 3, assert EN
	ctrl = ctrl | LCD_EN;
	*odr = ctrl;
 
	// step 4, wait
	i = PULSE;
	COUNTDOWN(i);
 
	// step 5, de-assert EN	
	ctrl = ctrl & ~LCD_EN; // de-assert EN
	*odr = ctrl;
 
	// step 6, wait 
	i = HOLD;
	COUNTDOWN(i);
}
 
void writechars(unsigned char *dat) {
	int i;
	unsigned short ctrl = *odr;
 
	do {
		lcdwait();
 
		*ddr = *ddr | 0x00FF; // set data lines to outputs
 
		// step 1, apply RS & WR, send data
 
		ctrl = ctrl & 0xFF00;
		ctrl = ctrl | *dat++;
		ctrl = ctrl | LCD_RS; // assert RS
		ctrl = ctrl & ~LCD_WR; // assert WR
		*odr = ctrl;
 
		// step 2
		i = SETUP;
		COUNTDOWN(i);
 
		// step 3, assert EN
		ctrl = ctrl | LCD_EN;
		*odr = ctrl;
 
		// step 4, wait 800 nS
		i = PULSE;
		COUNTDOWN(i);
 
		// step 5, de-assert EN	
		ctrl = ctrl & ~LCD_EN; // de-assert EN
		*odr = ctrl;
 
		// step 6, wait
		i = HOLD;
		COUNTDOWN(i);
	} while(*dat);
}

