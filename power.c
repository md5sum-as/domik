/*
 * power.c
 *
 *  Created on: 21.02.2016
 *      Author: alexs
 */

#include "glob_vars.h"
#include "bcm_spi_io.h"
#include "main.h"
#include "land_socket.h"
#include "main_sens.h"
#include "power.h"

void PWR_get_power() {
	static FILE * f_power=NULL;
	static char old_onoff=0;
	static char old_power_s2c=0;
	unsigned char buff[9];
	char diff_onoff=0;
	char diff_s2c=0;
	//	fpos_t pos_tmp;

	int pwr1;

#ifdef POINT
	point_add(PWR_get_power_s);
#endif

	buff[0]=0x20;
	if (spi_read(&dev_pins.powe,buff,9)) {
		syslog(LOG_ERR|LOG_DAEMON,"Error read power sensors");
#ifdef POINT
		point_add(PWR_get_power_e1);
#endif

		return;
	}
	power_onoff=buff[1];
	if (old_onoff!=power_onoff) {
		diff_onoff=old_onoff ^ power_onoff;
		old_onoff=power_onoff;
	}
	power_s2c=buff[7];
	if (old_power_s2c!=power_s2c) {
		diff_s2c=old_power_s2c ^ power_s2c;
		old_power_s2c=power_s2c;
	}

	if (diff_s2c || diff_onoff) {
		PWR_write_power_onoff(diff_onoff, diff_s2c);
	}

	power_all=(buff[2]<<8)+buff[3];
	pwr1=((double)(power_all_zero-power_all)/(double)power_all_scale)*230;
	power_oven=(buff[4]<<8)+buff[5];
	power_temper=buff[6];
	//	syslog(LOG_ERR|LOG_DAEMON,"T  %d",power_temper);

	if (f_power==NULL)
		if ((f_power=fopen("/tmp/domik.power","w"))==NULL) {
			syslog(LOG_ERR|LOG_DAEMON,"Can't create status file: /tmp/domik.power (%s)",strerror(errno));
			return;
		}
	fprintf(f_power,"Power: %d\nPWR_temp: %d\nPower raw: %d\nPower raw oven: %d\n",pwr1,power_temper,power_all,power_oven);
	if (loglevel >= DEBUG) fprintf(f_power,"PA: %d\nPO: %d\nS2C: %02X\n",power_all,power_oven,power_s2c);
	fflush(f_power);
	//	fgetpos(f_power,&pos_tmp);
	ftruncate(fileno(f_power),ftell(f_power));
	rewind(f_power);
#ifdef POINT
	point_add(PWR_get_power_e2);
#endif

}

void PWR_write_power_onoff(const char diff_onoff,const char diff_s2c) {
	FILE * f_power;
	static char first=1;
	char diff_str[1024];

#ifdef POINT
	point_add(PWR_write_power_onoff_s);
#endif

	if ((f_power=fopen("/tmp/domik.pwronoff","w"))==NULL) {
		syslog(LOG_ERR|LOG_DAEMON,"Can't create status file: /tmp/domik.pwronoff (%s)",strerror(errno));
#ifdef POINT
		point_add(PWR_write_power_onoff_e1);
#endif

		return;
	}
	fprintf(f_power,"RCD: %s\n"
			"Water heater: %s\n"
			"Raspberry: %s\n"
			"Refrigerators: %s\n"
			"Washing machine: %s\n"
			"Oven_dishwasher: %s\n",
			(power_onoff&0x01)?"On":"Off",
					(power_onoff&0x02)?"On":"Off",
							(power_onoff&0x04)?"On":"Off",
									(power_onoff&0x08)?"On":"Off",
											(power_onoff&0x10)?"On":"Off",
													(power_onoff&0x20)?"On":"Off");

	fprintf(f_power,"S2C kitchener: %s\n"
			"S2C Water heater: %s\n"
			"S2C air conditioning: %s\n"
			"S2C washing machine: %s\n"
			"S2C Oven_dishwasher: %s\n",
			(power_s2c&0x01)?"On":"Off",
					(power_s2c&0x02)?"On":"Off",
							(power_s2c&0x04)?"On":"Off",
									(power_s2c&0x08)?"On":"Off",
											(power_s2c&0x10)?"On":"Off");

	fprintf(f_power,"Power WDog: %d\n",(power_s2c&0x80?1:0));

	if (!first) {
		sprintf(diff_str,"%s%s%s%s%s%s%s%s%s%s%s%s",
				((diff_onoff&0x01)?((power_onoff&0x01)?"RCD: On#":"RCD: Off#"):""),
				((diff_onoff&0x02)?((power_onoff&0x02)?"Water heater: On#":"Water heater: Off#"):""),
				((diff_onoff&0x04)?((power_onoff&0x04)?"Raspberry: On#":"Raspberry: Off#"):""),
				((diff_onoff&0x08)?((power_onoff&0x08)?"Refrigerators: On#":"Refrigerators: Off#"):""),
				((diff_onoff&0x10)?((power_onoff&0x10)?"Washing machine: On#":"Washing machine: Off#"):""),
				((diff_onoff&0x20)?((power_onoff&0x20)?"Oven_dishwasher: On#":"Oven_dishwasher: Off#"):""),

				((diff_s2c&0x01)?((power_s2c&0x01)?"S2C kitchener: On#":"S2C kitchener: Off#"):""),
				((diff_s2c&0x02)?((power_s2c&0x02)?"S2C Water heater: On#":"S2C Water heater: Off#"):""),
				((diff_s2c&0x04)?((power_s2c&0x04)?"S2C air conditioning: On#":"S2C air conditioning: Off#"):""),
				((diff_s2c&0x08)?((power_s2c&0x08)?"S2C washing machine: On#":"S2C washing machine: Off#"):""),
				((diff_s2c&0x10)?((power_s2c&0x10)?"S2C Oven_dishwasher: On#":"S2C Oven_dishwasher: Off#"):""),
				((diff_s2c&0x80)?"Power module watchdog#":"")
		);
		if (loglevel >= DEBUG) syslog(LOG_DEBUG|LOG_DAEMON,"Diff on/off: %s",diff_str);
		MS_run_event(pwr_onoff_change,diff_str);
	}
	fclose(f_power);
	first=0;
	MS_run_event(stat_ch,"/tmp/domik.pwronoff");
#ifdef POINT
	point_add(PWR_write_power_onoff_e2);
#endif

}

/*
 * 0xA0 - kitchener
 * 0xA1 - Water heater
 * 0xA2 - air conditioning
 * 0xA3 - washing machine
 * 0xA4 - Oven
 * 0xAF - all of the above
 */
void PWR_power_off_abb(char num){
	unsigned char buff[2];
#ifdef POINT
	point_add(PWR_power_off_abb_s);
#endif

	buff[0]=num;
	buff[1]=1;
	if (spi_write(&dev_pins.powe,buff,2)) {
		syslog(LOG_ERR|LOG_DAEMON,"Error write power off state");
#ifdef POINT
		point_add(PWR_power_off_abb_e1);
#endif

		return;
	}
#ifdef POINT
	point_add(PWR_power_off_abb_e2);
#endif

}

/* 0xc2 0A
 * 0xbd 0.28A
 * 0xb0 2A
 */
void PWR_get_sens_curr(){
	unsigned char buff[3];
	static FILE * f_scurr=NULL;
	double curr;

#ifdef POINT
	point_add(PWR_get_sens_curr_s);
#endif

	buff[0]=0x01;
	if (spi_read(&dev_pins.sens,buff,3)) {
		syslog(LOG_ERR|LOG_DAEMON,"Error read WC sensors");
#ifdef POINT
		point_add(PWR_get_sens_curr_e1);
#endif

		return;
	}
#ifdef POINT
	point_add(PWR_get_sens_curr_s1);
#endif

	curr=(191-buff[1])*((double)1L/7.5);
#ifdef POINT
	point_add(PWR_get_sens_curr_s2);
#endif

	if (f_scurr==NULL) {
#ifdef POINT
		point_add(PWR_get_sens_curr_s3);
#endif

		if ((f_scurr=fopen("/tmp/domik.senscurr","w"))==NULL) {
			syslog(LOG_ERR|LOG_DAEMON,"Can't create status file: /tmp/domik.senscurr (%s)",strerror(errno));
			return;
		}
	}
#ifdef POINT
	point_add(PWR_get_sens_curr_s4);
#endif

	fprintf(f_scurr,"PWR Supply current: %.3f\n",curr);
#ifdef POINT
	point_add(PWR_get_sens_curr_s5);
#endif

	fflush(f_scurr);
	//	fgetpos(f_power,&pos_tmp);
#ifdef POINT
	point_add(PWR_get_sens_curr_s6);
#endif

	ftruncate(fileno(f_scurr),ftell(f_scurr));
#ifdef POINT
	point_add(PWR_get_sens_curr_s7);
#endif

	rewind(f_scurr);
#ifdef POINT
	point_add(PWR_get_sens_curr_e2);
#endif

}

char PWR_get_wc_lamp(){
	unsigned char buff[3];
	FILE * f_wc;
	static unsigned char old_wc=0;

#ifdef POINT
	point_add(PWR_get_wc_lamp_s);
#endif

	buff[0]=0x00;
	if (spi_read(&dev_pins.sens,buff,3)) {
		syslog(LOG_ERR|LOG_DAEMON,"Error read WC sensors");
#ifdef POINT
		point_add(PWR_get_wc_lamp_e1);
#endif

		return 0;
	}

	if (old_wc!=buff[1]) {
		old_wc=buff[1];
		if ((f_wc=fopen("/tmp/domik.wc","w"))==NULL) {
			syslog(LOG_ERR|LOG_DAEMON,"Can't create status file: /tmp/domik.wc (%s)",strerror(errno));
#ifdef POINT
			point_add(PWR_get_wc_lamp_e2);
#endif

			return 0;
		}
		fprintf(f_wc,"Restroom: %s\n"
				"Bathroom: %s\n"
				"Fan: %s\n"
				"Humidity_level: %s\n"
				,(buff[1]&0x01)?"On":"Off"
						,(buff[1]&0x02)?"On":"Off"
								,(buff[1]&0x04)?"On":"Off"
										,(buff[1]&0x08)?"Hi":"lo");
		fclose(f_wc);
		MS_run_event(stat_ch,"/tmp/domik.wc");
	}
#ifdef POINT
	point_add(PWR_get_wc_lamp_e3);
#endif

	return buff[1]&0x07;
}

void PWR_set_fan(char num) {
    unsigned char buff[2];

    buff[0]=0xbb;
    buff[1]=num;
	if (spi_write(&dev_pins.sens,buff,2)) {
		syslog(LOG_ERR|LOG_DAEMON,"Error write fan state");
	}
}
