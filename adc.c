/** @file adc.c
 *  @brief The functions to access the adc channel on PacMan
 *  
 *  @author Naing Minn Htet <naingminhtet91@gmail.com>
 */

#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "adc.h"
#include "dio.h"



//Inititalize ADC function calls
void adc_initialize(){
	poke16(0x80, 0x18); 
	poke16(0x82, 0x3c); 
}

//Get the value from specified adc channel.
double get_adc(int channel){
	double x = (signed short)peek16(0x82 + (2*channel));
	x = (x * 1006)/200;
	x = (x * 2048)/0x8000;

	x = x - 500;
	return x/10;
}