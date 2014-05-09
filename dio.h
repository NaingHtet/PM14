#include <stdio.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define DIO_WATCHDOG 0x0008
#define DIO_SAFETY 0x0004
#define DIO_CHARGERELAY 0x0001

void initiate_dio_mutex();
void mpoke16(uint16_t addr, uint16_t value);
uint16_t mpeek16(uint16_t addr);
uint16_t peek16(uint16_t addr);
void poke16(uint16_t addr, uint16_t value);
