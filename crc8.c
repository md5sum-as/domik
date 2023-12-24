/*
 * crc8.c
 *
 *  Created on: 27.12.2015
 *      Author: alexs
 */

#include "glob_vars.h"

__uint8_t _crc_ibutton_update(__uint8_t crc, __uint8_t data) {
	__uint8_t i;

#ifdef POINT
	//	point_add(_crc_ibutton_update_s);
#endif

	crc = crc ^ data;
	for (i = 0; i < 8; i++)
	{
		if (crc & 0x01)
			crc = (crc >> 1) ^ 0x8C;
		else
			crc >>= 1;
	}
#ifdef POINT
	//	point_add(_crc_ibutton_update_e1);
#endif

	return crc;
}

