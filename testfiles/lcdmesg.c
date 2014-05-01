#include<unistd.h>
#include<sys/types.h>
#include<sys/mman.h>
#include<stdio.h>
#include<fcntl.h>
#include<string.h>
 
#define FPGABASE 0x30000000
#define SYSCON (0x0a /sizeof(unsigned short))
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
 
#define COUNTDOWN(x)	asm volatile ( \
  "1:\n"\
  "subs %1, %1, #1;\n"\
  "bne 1b;\n"\
  : "=r" ((x)) : "r" ((x)) \
);
 
volatile unsigned short *syscon, *odr, *ddr, *idr;
 
void command(unsigned int);
void writechars(unsigned char *);
unsigned int lcdwait(void);
void lcdinit(void);
 
/* This program takes lines from stdin and prints them to the
 * 2 line LCD connected to the TS-8100/TS-8160 LCD header.  e.g
 *
 *    echo "hello world" | lcdmesg
 * 
 * It may need to be tweaked for different size displays
 */
 
int main(int argc, char **argv) {
	int i = 0;
 
	lcdinit();
	if (argc == 2) {
		writechars(argv[1]);
	}
	if (argc > 2) {
		writechars(argv[1]);
		lcdwait();
		command(0xa8); // set DDRAM addr to second row
		writechars(argv[2]);
	}
	if (argc >= 2) return 0;
 
	while(!feof(stdin)) {
		unsigned char buf[512];
 
		lcdwait();
		if (i) {
			// XXX: this seek addr may be different for different
			// LCD sizes!  -JO
			command(0xa8); // set DDRAM addr to second row
		} else {
			command(0x2); // return home
		}
		i = i ^ 0x1;
		if (fgets(buf, sizeof(buf), stdin) != NULL) {
			unsigned int len;
			buf[0x27] = 0;
			len = strlen(buf);
			if (buf[len - 1] == '\n') buf[len - 1] = 0;
			writechars(buf);
		}
	}
	return 0;
}
 
void lcdinit(void) {
	int fd = open("/dev/mem", O_RDWR|O_SYNC);
 
	syscon = (unsigned short *)mmap(0, getpagesize(), 
	  PROT_READ|PROT_WRITE, MAP_SHARED, fd, FPGABASE);
 
 
	odr = &syscon[ODR];
	ddr = &syscon[DDR];
	idr = &syscon[IDR];
	syscon = &syscon[SYSCON];
 
	*syscon = *syscon | 0x1000;
 
	*ddr = *ddr & 0xFF00; // data lines to inputs
	*ddr = *ddr | LCD_WR | LCD_RS | LCD_EN; // control lines to outputs
	*odr &= ~(LCD_EN | LCD_RS);
	*odr |= LCD_WR;
 
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