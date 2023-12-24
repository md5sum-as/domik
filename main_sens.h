/*
 * main_sens.h
 *
 *  Created on: 21.02.2016
 *      Author: alexs
 */

#ifndef MAIN_SENS_H_
#define MAIN_SENS_H_

void MS_get_main_on_off();
void MS_set_main_on_off(unsigned char addr, unsigned char val);
void MS_get_main_sens();
int MS_get_door_mode(unsigned char * door_mode);
void MS_store_sens_to_file();
int MS_most_sensor_check(sensor_t * sens, const char * desc);
int MS_smoke_sensor_check(smoke_sensor_t * sens, const char * desc);
int MS_run_event(const char * const_cmdi, const char * const_desc);
int MS_write_on_off();
void MS_humidity();
void MS_get_debug8();

#endif /* MAIN_SENS_H_ */
