#include <stdio.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <fcntl.h>
#include <termios.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>

static const char PORTNAME[] = "/dev/i2c-0";

static const int VOLTAGE_BYTES = 2;
static const int VOLTAGE_CMD = 0x17;

int addr;
int fd;

int open_i2c_port();
int set_i2c_address(int _addr);
void read_i2c(char* rd_buf, int no_rd_bytes);
void write_i2c(char* buf, int no_wr_bytes);
void rdwr_i2c(char cmd, char* response, int no_rd_bytes);
double get_voltage();