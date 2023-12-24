/*
 * rfid.h
 *
 *  Created on: 21.02.2016
 *      Author: alexs
 */

#ifndef RFID_H_
#define RFID_H_

#define RFID_led_off 0
#define RFID_led_slow 3
#define RFID_led_fast 2

int RFID_get_token(_token_t * token);
void RFID_clear_last(char led_mode);
void RFID_newkey();
void RFID_write_token(char * txt);
void RFID_write_token_cancel();
int RFID_write_stat();
void RFID_check_write();

#endif /* RFID_H_ */
