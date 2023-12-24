/*
 * power.h
 *
 *  Created on: 21.02.2016
 *      Author: alexs
 */

#ifndef POWER_H_
#define POWER_H_

void PWR_get_power();
void PWR_write_power_onoff(const char diff_onoff,const char diff_s2c);
char PWR_get_wc_lamp();
void PWR_power_off_abb(char num);
void PWR_get_sens_curr();
void PWR_set_fan(char num);

#endif /* POWER_H_ */

/* делитель верх плечо 3К6, нижнее 9К1 */
