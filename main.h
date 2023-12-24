/*
 * main.h
 *
 *  Created on: 04.01.2016
 *      Author: alexs
 */

#ifndef MAIN_H_
#define MAIN_H_

#define _PAUSE 200000
#define _NUM_MSEC 1000000/_PAUSE

#define _read_3 0x10
#define _read_5 0x11
#define _read_RouterA 0x12
#define _read_RouterB 0x13
#define _read_USBA 0x14
#define _read_USBB 0x15
#define _read_SmA 0x16
#define _read_SmB 0x17
#define _read_SmC 0x18
#define _read_SmAlarm 0x19
#define _read_Neptun 0x1a
#define _read_ProgRfid 0x1b
#define _read_sec_mode 0x1c
#define _read_RPIwatchdog 0x1d

#define _clear_irqA 0x80
#define _clear_irqB 0x81
#define _write_3 0x90
#define _write_5 0x91
#define _write_RouterA 0x92
#define _write_RouterB 0x93
#define _write_USBA 0x94
#define _write_USBB 0x95
#define _write_SmA 0x96
#define _write_SmB 0x97
#define _write_SmC 0x98
#define _write_SmAlarm 0x99
#define _write_Neptun 0x9a
#define _write_ProgRFID 0x9b
#define _write_sec_mode 0x9c
#define _write_RPIwatchdog 0x9d

#endif /* MAIN_H_ */
