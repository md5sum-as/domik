/*
 * bcm_spi.c
 *
 *  Created on: 22.11.2015
 *      Author: alexs
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "glob_vars.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include "bcm2835.h"
#include "crc8.h"

static int bcm_spi_clk=0;

void spi_set_clock(int clk) {
	bcm_spi_clk=clk;
}

int spi_init() {

#ifdef POINT
	point_add(spi_init_s);
#endif

	if (!bcm2835_init()) return 1;

	bcm2835_gpio_fsel(17, BCM2835_GPIO_FSEL_INPT);
	bcm2835_gpio_fsel(18, BCM2835_GPIO_FSEL_INPT);
	/*
	bcm2835_gpio_set_eds(17);
	bcm2835_gpio_set_eds(18);
	bcm2835_gpio_ren(17);
	bcm2835_gpio_ren(18);
	 */


	bcm2835_gpio_fsel(9, BCM2835_GPIO_FSEL_ALT0); /* MISO */
	bcm2835_gpio_fsel(10, BCM2835_GPIO_FSEL_ALT0); /* MOSI */
	bcm2835_gpio_fsel(11, BCM2835_GPIO_FSEL_ALT0); /* CLK */

	if (dev_pins.main.pin) {
		bcm2835_gpio_fsel(dev_pins.main.pin,BCM2835_GPIO_FSEL_OUTP);
		bcm2835_gpio_set(dev_pins.main.pin);
	}
	if (dev_pins.powe.pin) {
		bcm2835_gpio_fsel(dev_pins.powe.pin,BCM2835_GPIO_FSEL_OUTP);
		bcm2835_gpio_set(dev_pins.powe.pin);
	}
	if (dev_pins.rfid.pin) {
		bcm2835_gpio_fsel(dev_pins.rfid.pin,BCM2835_GPIO_FSEL_OUTP);
		bcm2835_gpio_set(dev_pins.rfid.pin);
	}
	if (dev_pins.sens.pin) {
		bcm2835_gpio_fsel(dev_pins.sens.pin,BCM2835_GPIO_FSEL_OUTP);
		bcm2835_gpio_set(dev_pins.sens.pin);
	}

	bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST);      /*The default*/
	bcm2835_spi_setDataMode(BCM2835_SPI_MODE0);                   /*The default*/
	bcm2835_spi_setClockDivider(bcm_spi_clk); /*The default*/
	bcm2835_spi_chipSelect(BCM2835_SPI_CS_NONE);                      /*The default*/
#ifdef POINT
	point_add(spi_init_e1);
#endif

	return 0;
}

void spi_release() {
#ifdef POINT
	point_add(spi_release_s);
#endif

	if (!bcm2835_init()) return;
	bcm2835_gpio_fsel(RPI_GPIO_P1_21, BCM2835_GPIO_FSEL_INPT); /* MISO */
	bcm2835_gpio_fsel(RPI_GPIO_P1_19, BCM2835_GPIO_FSEL_INPT); /* MOSI */
	bcm2835_gpio_fsel(RPI_GPIO_P1_23, BCM2835_GPIO_FSEL_INPT); /* CLK */
	if (dev_pins.main.pin) {
		bcm2835_gpio_fsel(dev_pins.main.pin,BCM2835_GPIO_FSEL_INPT);
	}
	if (dev_pins.powe.pin) {
		bcm2835_gpio_fsel(dev_pins.powe.pin,BCM2835_GPIO_FSEL_INPT);
	}
	if (dev_pins.rfid.pin) {
		bcm2835_gpio_fsel(dev_pins.rfid.pin,BCM2835_GPIO_FSEL_INPT);
	}
	if (dev_pins.sens.pin) {
		bcm2835_gpio_fsel(dev_pins.sens.pin,BCM2835_GPIO_FSEL_INPT);
	}
	bcm2835_close();
#ifdef POINT
point_add(spi_release_e1);
#endif

}

void spi_xmit (pin_t * dev, __uint8_t * buff, int len){

#ifdef POINT
	point_add(spi_xmit_s);
#endif

	bcm2835_gpio_clr(dev->pin);
	bcm2835_spi_transfern((char *)buff,len);
	//	usleep(100);
	bcm2835_gpio_set(dev->pin);
	usleep(100);
#ifdef POINT
	point_add(spi_xmit_e1);
#endif


}
void spi_5V(int on) {
#ifdef POINT
	point_add(spi_5V_s);
#endif

	if (on) {
		bcm2835_gpio_set(dev_pins.powe.pin);
		bcm2835_gpio_set(dev_pins.sens.pin);
	}else{
		bcm2835_gpio_clr(dev_pins.powe.pin);
		bcm2835_gpio_clr(dev_pins.sens.pin);
	}
#ifdef POINT
	point_add(spi_5V_e1);
#endif

}
void spi_3V(int on) {
#ifdef POINT
	point_add(spi_3V_s);
#endif

	if (on) {
		bcm2835_gpio_set(dev_pins.rfid.pin);
	}else{
		bcm2835_gpio_clr(dev_pins.rfid.pin);
	}
#ifdef POINT
	point_add(spi_3V_e1);
#endif

}

int spi_read(pin_t * dev, __uint8_t * buff, int len) {
	__uint8_t crc=0;
	int i;
	__uint8_t tmp[64];
	int err=20;

#ifdef POINT
	point_add(spi_read_s);
#endif

	if (len>=sizeof(tmp)) {
#ifdef POINT
		point_add(spi_read_e1);
#endif
		if (loglevel >= DEBUG) syslog(LOG_ERR|LOG_DAEMON,"spi_read error length");
		return 1;
	}

	while (err) {
		crc=0;
		for (i=0;i<len;i++) tmp[i]=buff[i];
		spi_xmit(dev,tmp,len);
		for (i=0;i<len;i++) {
			crc=_crc_ibutton_update(crc,tmp[i]);
			//			printf("%d %d %d\n",i,tmp[i],crc);
		}
		if (tmp[0]!=0) if (!crc) break;
		if (loglevel >= DEBUG) syslog(LOG_ERR|LOG_DAEMON,"spi_read error CRC. Dev %d",dev->pin);
		err--;
		usleep(9000);
	}
	if (!err) {
#ifdef POINT
		point_add(spi_read_e2);
#endif
		if (loglevel >= DEBUG) syslog(LOG_ERR|LOG_DAEMON,"spi_read error count");
		return 1;
	}
	for (i=0;i<len;i++) {
		buff[i]=tmp[i];
	}
#ifdef POINT
	point_add(spi_read_e3);
#endif

	return 0;
}

int spi_write(pin_t * dev, __uint8_t * const buff, int len) {
	__uint8_t crc=0;
	__uint8_t crc_save;
	int i;
	__uint8_t tmp[64];
	int err=20;

#ifdef POINT
	point_add(spi_write_s);
#endif

	if (len>sizeof(tmp)-3) {
#ifdef POINT
		point_add(spi_write_e1);
#endif

		return 1;
	}
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
		if (loglevel >= DEBUG1) syslog(LOG_ERR|LOG_DAEMON,"spi_write error CRC. Dev %d",dev->pin);
		err--;
		usleep(9000);

	}
	if (!err) {
#ifdef POINT
		point_add(spi_write_e2);
#endif

		return 1;
	}
#ifdef POINT
	point_add(spi_write_e3);
#endif

	return 0;
}

int get_irq(int num) {
#ifdef POINT
	point_add(get_irq_s);
#endif

	int r=0;
	if ((r=bcm2835_gpio_lev(num))==1) {
		if (loglevel >= DEBUG1) syslog(LOG_DEBUG|LOG_DAEMON,"DEBUG> IRQ_%s",((num==17)?"A":"B"));
	}
#ifdef POINT
	point_add(get_irq_e1);
#endif

	return r;
}
