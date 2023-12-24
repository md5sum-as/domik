/*
 * spi.c
 *
 *  Created on: 13.12.2015
 *      Author: alexs
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

//#include "linuxgpio.h"
#include "bcm_spi_io.h"
#include "crc8.h"

//static int use_bcm2835=0;

/*
void spi_use_bcm2835(int a) {
	use_bcm2835=a?1:0;
}
*/

int spi_init(){
	return bcm_spi_init();
}

void spi_release() {
		bcm_spi_release();
}

static void spi_xmit(pin_t * dev, __uint8_t * buff, int len) {
		bcm_spi_xmit(dev,buff,len);
}

int spi_read(pin_t * dev, __uint8_t * buff, int len) {
	__uint8_t crc=0;
	int i;
	__uint8_t tmp[64];
	int err=10;

	if (len>=sizeof(tmp)) return 1;

	while (err) {
		crc=0;
		for (i=0;i<len;i++) tmp[i]=buff[i];
		spi_xmit(dev,tmp,len);
		for (i=0;i<len;i++) {
			crc=_crc_ibutton_update(crc,tmp[i]);
//			printf("%d %d %d\n",i,tmp[i],crc);
		}
		if (!crc) break;
		err--;
	}
	if (!err) return 1;
	for (i=0;i<len;i++) {
		buff[i]=tmp[i];
	}
	return 0;
}

int spi_write(pin_t * dev, __uint8_t * const buff, int len) {
	__uint8_t crc=0;
	__uint8_t crc_save;
	int i;
	__uint8_t tmp[64];
	int err=10;

	if (len>sizeof(tmp)-3) return 1;

	while (err) {
		crc=0;
		for (i=0;i<len;i++) {
			crc=_crc_ibutton_update(crc,buff[i]);
			tmp[i]=buff[i];
		}
		tmp[len]=crc;
		crc_save=crc;
//		printf("%d %d %d\n",tmp[0],tmp[1],tmp[2]);
		spi_xmit(dev,tmp,len+2);
//		printf("%d %d %d %d\n",tmp[0],tmp[1],tmp[2],tmp[3]);
		crc=_crc_ibutton_update(0,tmp[0]);
		for (i=0;i<len;i++) {
			crc=_crc_ibutton_update(crc,buff[i]);
		}
		crc=_crc_ibutton_update(crc,crc_save);
		crc=_crc_ibutton_update(crc,tmp[len+1]);
//		printf("%d\n",crc);
		if (!crc) break;
	}
	if (!err) return 1;
	return 0;
}

void spi_set_clock(int clk) {
	bcm_spi_set_clock(clk);
}

void spi_on_off_5V(int on) {
		bcm_spi_5V(on);
}

void spi_on_off_3V(int on) {
		bcm_spi_3V(on);
}
