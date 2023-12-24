/*
 * bcm_spi.h
 *
 *  Created on: 22.11.2015
 *      Author: alexs
 */

#ifndef BCM_SPI_H_
#define BCM_SPI_H_
//#include "linuxgpio.h"
#include "glob_vars.h"

void spi_set_clock(int clk);
int spi_init();
void spi_release();
void spi_xmit (pin_t * dev, unsigned char * buff, int len);
void spi_5V(int on);
void spi_3V(int on);

int spi_read(pin_t * dev, __uint8_t * buff, int len);
int spi_write(pin_t * dev, __uint8_t * const buff, int len);

int get_irq();
#define get_irqA get_irq(17)
#define get_irqB get_irq(18)



#endif /* BCM_SPI_H_ */
