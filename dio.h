/** @file dio.h
 *  @brief The header for dio.c
 *  
 *  @author Naing Minn Htet <naingminhtet91@gmail.com>
 */

#include <stdio.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

//DIO addresses
#define DIO_DIRECTION 0x0008
#define DIO_OUTPUT 0x0004
#define DIO_INPUT 0x000c

//DIO values
#define DIO_ON 1
#define DIO_OFF 0

//DIO channel bit positions
#define DIO_WATCHDOG 0x0008
#define DIO_SAFETY 0x0004
#define DIO_CHARGERELAY 0x0001
#define DIO_SERIAL 0x0020
#define DIO_CHARGERPLUG 0x0040
#define DIO_AMSRESET 0x0002

void mpoke16(uint16_t addr, uint16_t value);
uint16_t mpeek16(uint16_t addr);
uint16_t peek16(uint16_t addr);
void poke16(uint16_t addr, uint16_t value);

void dio_initialize();
void dio_destroy();

void mpeekpoke16(uint16_t addr, uint16_t bitposition, int onoff);
