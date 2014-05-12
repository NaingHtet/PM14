#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <math.h>
#include "display_controller.h"
#include "i2c_controller.h"
#include "pack_controller.h"
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

void command(unsigned int);
void writechars(unsigned char *);
unsigned int lcdwait(void);

void display_error_msg(char* msg) {
	command(0x1);
	lcdwait();
	command(FIRSTLINE);
	writechars("PROGRAM FAILED!");
	lcdwait();
	command(SECONDLINE);
	writechars(msg);
	lcdwait();
}
 
void *display_LCD(void *arg) {
	sleep(2);

	int* PROGRAM_RUNNING= arg;


	while(*PROGRAM_RUNNING) {

		char wbuf[21];
		char xbuf[21];


		double cur_I;
		double cur_V;
		int n = get_current(&cur_I);


		if ( n >= 0) {
			n = get_overall_voltage(&cur_V);
		}
		if ( n < 0 ) {
			sprintf(wbuf, "E03:NO AMS");
		} else {			
			sprintf(wbuf, "C:%3.2f   V:%3.2f", cur_I, cur_V);
		}

		sprintf(xbuf, "SOC:%d  ", disp_SOC);
		if (charging_state) 
			sprintf(xbuf+ strlen(xbuf),"CHARGING");
		else sprintf(xbuf+ strlen(xbuf),"DISCHARGING");


		command(0x1);
		lcdwait();

		command(THIRDLINE);
		writechars(wbuf);
		lcdwait();

		command(SECONDLINE);
		writechars(xbuf);
		lcdwait();

		command(FIRSTLINE);
		writechars("PACMAN 2014 - NMH");
		lcdwait();

		if (!SYSTEM_SAFE) {
			command(FOURTHLINE);
			writechars("E05:SYSTEM UNSAFE");
		} else if ( BATTERY_LOW ) {
			command(FOURTHLINE);
			writechars("E06:BATTERY LOW");
		}

		// i = (i + 1)%norounds;
		sleep(2);
	}

	command(0x1);
	lcdwait();
	command(FIRSTLINE);
	writechars("PROGRAM STOPPED!");
	lcdwait();
}


//xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
//BELOW IS THE CODE FROM TS8160-4200 WIKI
//xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

void display_controller_initialize() {
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

