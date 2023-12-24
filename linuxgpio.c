/*
 * avrdude - A Downloader/Uploader for AVR device programmers
 * Support for bitbanging GPIO pins using the /sys/class/gpio interface
 * 
 * Copyright (C) 2013 Radoslav Kolev <radoslav@kolev.info>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include "glob_vars.h"
#include "linuxgpio.h"
/*#include <bcm2835.h>*/

#define GPIO_DIR_IN		0
#define GPIO_DIR_OUT	1

#define MOSI_PIN 10
#define MISO_PIN 9
#define SCLK_PIN 11


static int linuxgpio_openfd(unsigned int gpio)
{
	char filepath[60];

	snprintf(filepath, sizeof(filepath), "/sys/class/gpio/gpio%d/value", gpio);
	return (open(filepath, O_RDWR));
}

static int linuxgpio_export(unsigned int gpio)
{
	int fd, len, r=0;
	char buf[11];

	if ((fd = linuxgpio_openfd(gpio)) < 0) {
		fd = open("/sys/class/gpio/export", O_WRONLY);
		if (fd < 0) {
			perror("Can't open /sys/class/gpio/export");
			return fd;
		}

		len = snprintf(buf, sizeof(buf), "%d", gpio);
		r = write(fd, buf, len);
	}
	close(fd);

	return r;
}

static int linuxgpio_unexport(unsigned int gpio)
{
	int fd, len, r;
	char buf[11];

	fd = open("/sys/class/gpio/unexport", O_WRONLY);
	if (fd < 0) {
		perror("Can't open /sys/class/gpio/unexport");
		return fd;
	}

	len = snprintf(buf, sizeof(buf), "%d", gpio);
	r = write(fd, buf, len);
	close(fd);

	return r;
}

static int linuxgpio_dir(unsigned int gpio, unsigned int dir)
{
	int fd, r;
	char buf[60];

	snprintf(buf, sizeof(buf), "/sys/class/gpio/gpio%d/direction", gpio);

	fd = open(buf, O_WRONLY);
	if (fd < 0) {
		perror("Can't open gpioX/direction");
		return fd;
	}

	if (dir == GPIO_DIR_OUT)
		r = write(fd, "out", 4);
	else
		r = write(fd, "in", 3);

	close(fd);

	return r;
}

static int linuxgpio_dir_out(unsigned int gpio)
{
	return linuxgpio_dir(gpio, GPIO_DIR_OUT);
}

static int linuxgpio_dir_in(unsigned int gpio)
{
	return linuxgpio_dir(gpio, GPIO_DIR_IN);
}

static int linuxgpio_setpin(pin_t * dev, int value)
{
	int r;

	if ( dev->fd < 0 )
		return -1;

	if (value)
		r = write(dev->fd, "1", 1);
	else
		r = write(dev->fd, "0", 1);

	if (r!=1) return -1;

	return 0;
}

static int linuxgpio_getpin(pin_t * dev)
{
	char c;

  if ( dev->fd < 0 )
    return -1;

  if (lseek(dev->fd, 0, SEEK_SET)<0)
    return -1;

  if (read(dev->fd, &c, 1)!=1)
    return -1;

  if (c=='0')
    return 0;
  else if (c=='1')
    return 1;
  else
    return -1;
}

int gpio_spi_init() {
	unsigned int i;
	int r=0;

	for (i = 0; i < NUM_PINS; i++) {
		if (dev_pins.pin_arr[i].pin==0) continue;
		if ((r = linuxgpio_export(dev_pins.pin_arr[i].pin)) < 0) return r+100;
		if (i > 0) linuxgpio_dir_out(dev_pins.pin_arr[i].pin); else linuxgpio_dir_in(dev_pins.pin_arr[i].pin);
		if ((dev_pins.pin_arr[i].fd=linuxgpio_openfd(dev_pins.pin_arr[i].pin)) < 0) return dev_pins.pin_arr[i].fd + 1000;
		if (i < 3) linuxgpio_setpin(&dev_pins.pin_arr[i],0); else linuxgpio_setpin(&dev_pins.pin_arr[i],1);
	}
	return 0;
}

void gpio_spi_release() {
	unsigned int i;

	for (i = 0; i < NUM_PINS; i++) {
		if (dev_pins.pin_arr[i].pin) linuxgpio_unexport(dev_pins.pin_arr[i].pin);
	}
}

static unsigned char xmit_byte_gpio(unsigned char send){
	int i;
	unsigned char tmp=0;
	unsigned char tmpi=0;
	unsigned char tmpv=0;


	for (i=0; i< 8 ;i++) {
		if (send & 0x80) linuxgpio_setpin(&dev_pins.mosi,1);
		else linuxgpio_setpin(&dev_pins.mosi,0);
		usleep(100);
		linuxgpio_setpin(&dev_pins.sclk,1);
		tmp <<= 1;
		usleep(100);
		for (tmpi=0; tmpi<10; tmpi++) {
			if ((tmpv=linuxgpio_getpin(&dev_pins.miso))==(linuxgpio_getpin(&dev_pins.miso))) break;
		}
		tmp |= tmpv;

		linuxgpio_setpin(&dev_pins.sclk,0);
		send <<= 1;
	}
/*	linuxgpio_setpin(&dev_pins.mosi,1);*/
	return tmp;
}

void gpio_spi_xmit (pin_t * dev, __uint8_t * buff, int len) {
	int i;
	__uint8_t * p;

	p=buff;
	linuxgpio_setpin(dev,0);
	usleep(300);
	for (i=0; i<len; i++) {
/*
		if (debug_level>3) {
			printf("send: %X\n",*p);
		}
*/
		*p=xmit_byte_gpio(*p);
		p++;
		usleep(300);
	}
/*	usleep(10);*/
	linuxgpio_setpin(dev,1);
}

void gpio_spi_5V(int on) {
	if (on) {
		linuxgpio_setpin(&dev_pins.powe,1);
		linuxgpio_setpin(&dev_pins.sens,1);
	}else{
		linuxgpio_setpin(&dev_pins.powe,0);
		linuxgpio_setpin(&dev_pins.sens,0);
	}
}

void gpio_spi_3V(int on) {
	if (on) {
		linuxgpio_setpin(&dev_pins.rfid,1);
	}else{
		linuxgpio_setpin(&dev_pins.rfid,0);
	}
}

/*
 * End of GPIO user space helpers
 */

/*#define N_GPIO (PIN_MAX + 1)*/

/*
 * an array which holds open FDs to /sys/class/gpio/gpioXX/value for all needed pins
 */

/*static int linuxgpio_fds[N_GPIO] ;*/


/*
static int linuxgpio_setpin(PROGRAMMER * pgm, int pin, int value)
{
  int r;

  if (pin & PIN_INVERSE)
  {
    value  = !value;
    pin   &= PIN_MASK;
  }

  if ( linuxgpio_fds[pin] < 0 )
    return -1;

  if (value)
    r = write(linuxgpio_fds[pin], "1", 1);
  else
    r = write(linuxgpio_fds[pin], "0", 1);

  if (r!=1) return -1;

  if (pgm->ispdelay > 1)
    bitbang_delay(pgm->ispdelay);

  return 0;
}

static int linuxgpio_getpin(PROGRAMMER * pgm, int pin)
{
  unsigned char invert=0;
  char c;

  if (pin & PIN_INVERSE)
  {
    invert = 1;
    pin   &= PIN_MASK;
  }

  if ( linuxgpio_fds[pin] < 0 )
    return -1;

  if (lseek(linuxgpio_fds[pin], 0, SEEK_SET)<0)
    return -1;

  if (read(linuxgpio_fds[pin], &c, 1)!=1)
    return -1;

  if (c=='0')
    return 0+invert;
  else if (c=='1')
    return 1-invert;
  else
    return -1;

}

static int linuxgpio_highpulsepin(PROGRAMMER * pgm, int pin)
{

  if ( linuxgpio_fds[pin & PIN_MASK] < 0 )
    return -1;

  linuxgpio_setpin(pgm, pin, 1);
  linuxgpio_setpin(pgm, pin, 0);

  return 0;
}

 */



//static int linuxgpio_open(PROGRAMMER *pgm, char *port)
//{
//  int r, i, pin;
//
//  bitbang_check_prerequisites(pgm);
//
//
//  for (i=0; i<N_GPIO; i++)
//    linuxgpio_fds[i] = -1;
//  //Avrdude assumes that if a pin number is 0 it means not used/available
//  //this causes a problem because 0 is a valid GPIO number in Linux sysfs.
//  //To avoid annoying off by one pin numbering we assume SCK, MOSI, MISO
//  //and RESET pins are always defined in avrdude.conf, even as 0. If they're
//  //not programming will not work anyway. The drawbacks of this approach are
//  //that unwanted toggling of GPIO0 can occur and that other optional pins
//  //mostry LED status, can't be set to GPIO0. It can be fixed when a better
//  //solution exists.
//  for (i=2; i<N_PINS; i++) {
//    if ( pgm->pinno[i] != 0 ||
//         i == PIN_AVR_RESET ||
//         i == PIN_AVR_SCK   ||
//         i == PIN_AVR_MOSI  ||
//         i == PIN_AVR_MISO ) {
//        pin = pgm->pinno[i] & PIN_MASK;
//        if ((r=linuxgpio_export(pin)) < 0) {
//            fprintf(stderr, "Can't export GPIO %d, already exported/busy?: %s",
//                    pin, strerror(errno));
//            return r;
//        }
//        if (i == PIN_AVR_MISO)
//            r=linuxgpio_dir_in(pin);
//        else
//            r=linuxgpio_dir_out(pin);
//
//        if (r < 0)
//            return r;
//
//        if ((linuxgpio_fds[pin]=linuxgpio_openfd(pin)) < 0)
//            return linuxgpio_fds[pin];
//    }
//  }
//
// return(0);
//}
//
//static void linuxgpio_close(PROGRAMMER *pgm)
//{
//  int i, reset_pin;
//
//  reset_pin = pgm->pinno[PIN_AVR_RESET] & PIN_MASK;
//
//  //first configure all pins as input, except RESET
//  //this should avoid possible conflicts when AVR firmware starts
//  for (i=0; i<N_GPIO; i++) {
//    if (linuxgpio_fds[i] >= 0 && i != reset_pin) {
//       close(linuxgpio_fds[i]);
//       linuxgpio_dir_in(i);
//       linuxgpio_unexport(i);
//    }
//  }
//  //configure RESET as input, if there's external pull up it will go high
//  if (linuxgpio_fds[reset_pin] >= 0) {
//    close(linuxgpio_fds[reset_pin]);
//    linuxgpio_dir_in(reset_pin);
//    linuxgpio_unexport(reset_pin);
//  }
//}
//
//void linuxgpio_initpgm(PROGRAMMER *pgm)
//{
//  strcpy(pgm->type, "linuxgpio");
//
//  pgm_fill_old_pins(pgm); // to be removed if old pin data no longer needed
//
//  pgm->rdy_led        = bitbang_rdy_led;
//  pgm->err_led        = bitbang_err_led;
//  pgm->pgm_led        = bitbang_pgm_led;
//  pgm->vfy_led        = bitbang_vfy_led;
//  pgm->initialize     = bitbang_initialize;
//  pgm->display        = linuxgpio_display;
//  pgm->enable         = linuxgpio_enable;
//  pgm->disable        = linuxgpio_disable;
//  pgm->powerup        = linuxgpio_powerup;
//  pgm->powerdown      = linuxgpio_powerdown;
//  pgm->program_enable = bitbang_program_enable;
//  pgm->chip_erase     = bitbang_chip_erase;
//  pgm->cmd            = bitbang_cmd;
//  pgm->open           = linuxgpio_open;
//  pgm->close          = linuxgpio_close;
//  pgm->setpin         = linuxgpio_setpin;
//  pgm->getpin         = linuxgpio_getpin;
//  pgm->highpulsepin   = linuxgpio_highpulsepin;
//  pgm->read_byte      = avr_read_byte_default;
//  pgm->write_byte     = avr_write_byte_default;
//}
//
//const char linuxgpio_desc[] = "GPIO bitbanging using the Linux sysfs interface";
//
//#else  /* !HAVE_LINUXGPIO */
//
//void linuxgpio_initpgm(PROGRAMMER * pgm)
//{
//  fprintf(stderr,
//	  "%s: Linux sysfs GPIO support not available in this configuration\n",
//	  progname);
//}
//
//const char linuxgpio_desc[] = "GPIO bitbanging using the Linux sysfs interface (not available)";
//
//#endif /* HAVE_LINUXGPIO */
