/** @file adc.c
 *  @brief The header file for adc.c
 *  
 *  @author Naing Minn Htet <naingminhtet91@gmail.com>
 */

//ADC Channels
#define FUSE_TEMP 5
#define SHUNT_TEMP 4

void adc_initialize();
double get_adc(int channel);