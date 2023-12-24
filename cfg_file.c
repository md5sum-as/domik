/*
 * cfg_file.c
 *
 *  Created on: 30.10.2015
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

//#include "linuxgpio.h"
#include "bcm_spi_io.h"


char cfg_file_name[80]={"/usr/local/etc/domik.cfg"};

static int cfg_copy_str(char * to, char * from, int to_len) {
	char * chr;

#ifdef POINT
	point_add(cfg_copy_str_s);
#endif

	if ((chr=strchr(from,'\n'))!=NULL) *chr=0;
	if ((chr=strchr(from,'\r'))!=NULL) *chr=0;
	if ((chr=strchr(from,'\t'))!=NULL) *chr=' ';

	if (strlen(from)>(to_len-1)) {
		syslog(LOG_ERR|LOG_DAEMON,"Command is too long: %s...",from);
#ifdef POINT
		point_add(cfg_copy_str_e1);
#endif

		return 1;
	}
	strncpy(to,from,(to_len-1));
	to[to_len-1]=0;
	if ((chr=strchr(to,'\n'))!=NULL) *chr=0;
	if ((chr=strchr(to,'\r'))!=NULL) *chr=0;
	if ((chr=strchr(to,'\t'))!=NULL) *chr=' ';
#ifdef POINT
	point_add(cfg_copy_str_e2);
#endif

	return 0;
}

static int parse_config_file () {
	FILE *fd;
	char str[1024];
	int i;
	int r=0;

#ifdef POINT
	point_add(parse_config_file_s);
#endif

	fd=fopen(cfg_file_name,"r");
	if (fd == NULL) {
		fprintf(stderr,"Cant open config file: %s\n\n",cfg_file_name);
#ifdef POINT
		point_add(parse_config_file_e1);
#endif

		return 1;
	}

	for (i=0; i< NUM_PINS; i++) {dev_pins.pin_arr[i].pin=0;}
	/*стандартные пины*/
	dev_pins.miso.pin=9;
	dev_pins.mosi.pin=10;
	dev_pins.sclk.pin=11;

	/*дефолтные значения некоторых выходов*/
	memset(&on_off_cfg,0,sizeof(on_off_cfg));
	on_off_cfg.smokeAlarm=0;
	on_off_cfg.progRFID=1;
	on_off_cfg.routerA=1;
	on_off_cfg.usbA=1;
	on_off_cfg.usbB=1;
	on_off_cfg.sec_mode=0;

	/*заполним дефолтные значения сенсоров*/
	/*	СМ glob_vars.c
	 *
	Battery.def=1; Battery.old=1; Battery.Alarm=""; Battery.Clear="";
	ACPower.def=0; ACPower.old=0; ACPower.Alarm=""; ACPower.Clear="";
	WaterAcnt.def=1; WaterAcnt.old=1; WaterAcnt.Alarm=""; WaterAcnt.Clear="";
	WaterBcnt.def=1; WaterBcnt.old=1; WaterBcnt.Alarm=""; WaterBcnt.Clear="";
	Neptun.def=0; Neptun.old=0; Neptun.Alarm=""; Neptun.Clear="";
	SmokeA.def=1; SmokeA.old=1; SmokeA.Alarm=""; SmokeA.Clear=""; SmokeA.Error="";
	SmokeB.def=1; SmokeB.old=1;
	SmokeC.def=1; SmokeC.old=1;
	SmokeA.err=0; SmokeA.eold=0;
	SmokeB.err=0; SmokeB.eold=0;
	SmokeC.err=0; SmokeC.eold=0;
	DoorA.def=0; DoorA.old=0; Значение закрытого состояния
	DoorB.def=0; DoorB.old=0;
	 */

	do {
		fgets(str,sizeof(str),fd);
		if (str[0]=='#') continue;
		if (str[0]=='\n') continue;

		if (strncasecmp("MOSI=",str,5)==0) dev_pins.mosi.pin=atoi(str+5);
		if (strncasecmp("MISO=",str,5)==0) dev_pins.miso.pin=atoi(str+5);
		if (strncasecmp("SCLK=",str,5)==0) dev_pins.sclk.pin=atoi(str+5);
		if (strncasecmp("MAIN_BOARD=",str,11)==0) dev_pins.main.pin=atoi(str+11);
		if (strncasecmp("POWER=",str,6)==0) dev_pins.powe.pin=atoi(str+6);
		if (strncasecmp("SENS_ADD=",str,9)==0) dev_pins.sens.pin=atoi(str+9);
		if (strncasecmp("RFID=",str,5)==0) dev_pins.rfid.pin=atoi(str+5);
		/*
		if (strncasecmp("USE_BCM2835=Y",str,13)==0) {
			spi_use_bcm2835(1);
		}
		 */
		if (strncasecmp("LOG_LEVEL=",str,10)==0) loglevel=(atoi(str+10));
		if (strncasecmp("BCM2835_CLK_DIV=",str,16)==0) spi_set_clock(atoi(str+16));
		if (strncasecmp("LANDING_IP=",str,11)==0){
			r=cfg_copy_str(landing_ip_chr,&str[11],sizeof(landing_ip_chr));
			Landing.def=1;
		}
		if (strncasecmp("LANDING_PORT=",str,13)==0) landing_port=atoi(str+13);
		if (strncasecmp("LANDING_COLOR_LIGHT=",str,20)==0) r=cfg_copy_str(land_color_light,&str[20],sizeof(land_color_light)+1);
		if (strncasecmp("LANDING_COLOR_WC=",str,17)==0) r=cfg_copy_str(land_color_wc,&str[17],sizeof(land_color_wc)+1);
		if (strncasecmp("LANDING_COLOR_POWER=",str,20)==0) r=cfg_copy_str(land_color_power,&str[20],sizeof(land_color_power)+1);
		if (strncasecmp("LANDING_COLOR_HEATER=",str,21)==0) r=cfg_copy_str(land_color_heater,&str[21],sizeof(land_color_heater)+1);

		if (strncasecmp("MAIN_3=",str,7)==0) on_off_cfg.p3v=atoi(str+7)?1:0;
		if (strncasecmp("MAIN_5=",str,7)==0) on_off_cfg.p5v=atoi(str+7)?1:0;
		if (strncasecmp("ROUTER_B=",str,9)==0) on_off_cfg.routerB=atoi(str+9)?1:0;
		if (strncasecmp("SMOKE_A=",str,8)==0) {
			on_off_cfg.smokeA=atoi(str+8)?1:0;
			/*если питание включено, то дефолтное значение 3*/
			if (on_off_cfg.smokeA) {SmokeA.err=3;SmokeA.eold=3;}
		}
		if (strncasecmp("SMOKE_B=",str,8)==0) {
			on_off_cfg.smokeB=atoi(str+8)?1:0;
			/*если питание включено, то дефолтное значение 3*/
			if (on_off_cfg.smokeB) {SmokeB.err=3; SmokeB.eold=3;}
		}
		if (strncasecmp("SMOKE_C=",str,8)==0) {
			on_off_cfg.smokeC=atoi(str+8)?1:0;
			/*если питание включено, то дефолтное значение 3*/
			if (on_off_cfg.smokeC) {SmokeC.err=3; SmokeC.eold=3;}
		}
		if (strncasecmp("Smoke_OFF_time_sec=",str,19)==0) Smoke_off_time=(atoi(str+19));

		if (strncasecmp("NEPTUN=",str,7)==0) on_off_cfg.neptunCntrl=atoi(str+7)?1:0;

		if (strncasecmp("Battery_alarm=",str,14)==0) r=cfg_copy_str(Battery.Alarm,&str[14],sizeof(Battery.Alarm));
		if (strncasecmp("Battery_clear=",str,14)==0) r=cfg_copy_str(Battery.Clear,&str[14],sizeof(Battery.Clear));

		if (strncasecmp("ACpower_alarm=",str,14)==0) r=cfg_copy_str(ACPower.Alarm,&str[14],sizeof(ACPower.Alarm));
		if (strncasecmp("ACpower_clear=",str,14)==0) r=cfg_copy_str(ACPower.Clear,&str[14],sizeof(ACPower.Clear));

		if (strncasecmp("Neptun_alarm=",str,13)==0) r=cfg_copy_str(Neptun.Alarm,&str[13],sizeof(Neptun.Alarm));
		if (strncasecmp("Neptun_clear=",str,13)==0) r=cfg_copy_str(Neptun.Clear,&str[13],sizeof(Neptun.Clear));

		if (strncasecmp("Smoke_A_alarm=",str,14)==0) r=cfg_copy_str(SmokeA.Alarm,&str[14],sizeof(SmokeA.Alarm));
		if (strncasecmp("Smoke_A_clear=",str,14)==0) r=cfg_copy_str(SmokeA.Clear,&str[14],sizeof(SmokeA.Clear));
		if (strncasecmp("Smoke_A_error=",str,14)==0) r=cfg_copy_str(SmokeA.Error,&str[14],sizeof(SmokeA.Error));

		if (strncasecmp("Smoke_B_alarm=",str,14)==0) r=cfg_copy_str(SmokeB.Alarm,&str[14],sizeof(SmokeB.Alarm));
		if (strncasecmp("Smoke_B_clear=",str,14)==0) r=cfg_copy_str(SmokeB.Clear,&str[14],sizeof(SmokeB.Clear));
		if (strncasecmp("Smoke_B_error=",str,14)==0) r=cfg_copy_str(SmokeB.Error,&str[14],sizeof(SmokeB.Error));

		if (strncasecmp("Smoke_C_alarm=",str,14)==0) r=cfg_copy_str(SmokeC.Alarm,&str[14],sizeof(SmokeC.Alarm));
		if (strncasecmp("Smoke_C_clear=",str,14)==0) r=cfg_copy_str(SmokeC.Clear,&str[14],sizeof(SmokeC.Clear));
		if (strncasecmp("Smoke_C_error=",str,14)==0) r=cfg_copy_str(SmokeC.Error,&str[14],sizeof(SmokeC.Error));

		if (strncasecmp("WaterA=",str,7)==0) {
			r=cfg_copy_str(WaterAcnt.Alarm,&str[7],sizeof(WaterAcnt.Alarm));
			WaterAcnt.def=1;
		}
		if (strncasecmp("WaterB=",str,7)==0) {
			r=cfg_copy_str(WaterBcnt.Alarm,&str[7],sizeof(WaterBcnt.Alarm));
			WaterBcnt.def=1;
		}

		if (strncasecmp("Security_ON=",str,12)==0) r=cfg_copy_str(security_on,&str[12],sizeof(security_on));
		if (strncasecmp("Security_ON_WH=",str,15)==0) r=cfg_copy_str(security_on_wh,&str[15],sizeof(security_on_wh));
		if (strncasecmp("Security_OFF=",str,13)==0) r=cfg_copy_str(security_off,&str[13],sizeof(security_off));
		if (strncasecmp("Security_ALARM=",str,15)==0) r=cfg_copy_str(security_alarm,&str[15],sizeof(security_alarm));

		if (strncasecmp("Security_PreSec=",str,16)==0) r=cfg_copy_str(security_pre,&str[16],sizeof(security_pre));
		if (strncasecmp("Security_PS_clear=",str,18)==0) r=cfg_copy_str(security_preclr,&str[18],sizeof(security_preclr));

		if (strncasecmp("Mifare_K_key=",str,13)==0) {
			strncpy(RFID_key,&str[13],sizeof(RFID_key));
			memcpy(RFID_key_NEW,RFID_key,sizeof(RFID_key));
			if (loglevel >= DEBUG1) syslog(LOG_ERR|LOG_DAEMON,"RFID_key: %s",RFID_key);
		}
		if (strncasecmp("SMOKE_ALARM_TIMER=",str,18)==0) smoke_alarm_timer_cfg=(atoi(str+18));

		if (strncasecmp("PWR_ZERO=",str,9)==0) power_all_zero=atoi(str+9);
		if (strncasecmp("PWR_SCALE=",str,10)==0) power_all_scale=atoi(str+10);
		if (strncasecmp("PWR_WARN=",str,9)==0) power_all_warn=atoi(str+9);
		if (strncasecmp("PWR2_ZERO=",str,10)==0) power_oven_zero=atoi(str+10);
		if (strncasecmp("PWR_INFO=",str,9)==0) r=cfg_copy_str(pwr_onoff_change,&str[9],sizeof(pwr_onoff_change));

		if (strncasecmp("Computer_ON=",str,12)==0) r=cfg_copy_str(comp_on,&str[12],sizeof(comp_on));
		if (strncasecmp("Statistic_change=",str,17)==0) r=cfg_copy_str(stat_ch,&str[17],sizeof(stat_ch));

		if (strncasecmp("DoorA=",str,6)==0) r=cfg_copy_str(doorA_h,&str[6],sizeof(doorA_h));
		if (strncasecmp("DoorB=",str,6)==0) r=cfg_copy_str(doorB_h,&str[6],sizeof(doorB_h));

	} while (!feof(fd));
	fclose(fd);

#ifdef POINT
	point_add(parse_config_file_e2);
#endif

	return r;
}

void usage (char *base) {
#ifdef POINT
	point_add(usage_s);
#endif

	printf("Usage: %s [options]\n"
			"\n"
			"OPTIONS:\n"
			" -c <config_file>\tBy default is /usr/local/etc/domik.cfg\n"
			" -v\t\t\tShow version and exit\n"
			" -h\t\t\tThis help\n"
			"\n\n"
			,base);
	printf("Commands that you can sent to process /run/domik:\n"
			"3V on/off\n5V on/off\nRouter_A off\nRouter_B on/off\n"
			"USB_A on/off\nUSB_B on/off\n"
			"Smoke_A on/off\nSmoke_B on/off\nSmoke_C on/off\n"
			"Smoke_test_A on/off\nSmoke_test_B on/off\nSmoke_test_C on/off\n"
			"Smoke_Alarm on/off\n"
			"Neptun on/off\n"
			"RFID_prog on/off\n"
			"To secure\n"
			"Landing_cmd:\n"
			"Kitchen off\nWaterHeat off\nAir cond off\nWashing off\nOven off\nAll ABB off\n"
			"Reload RFID\nWrite token:\nFan_mode on/off/auto\n\n");
#ifdef POINT
	point_add(usage_e1);
#endif

}

int parse_cmd_cfg(int argc, char *argv[]) {
	int r=0;
	int i=1;

#ifdef POINT
	point_add(parse_cmd_cfg_s);
#endif

	while (i<argc) {

		if (strncmp("-c",argv[i],2)==0) {
			strncpy(cfg_file_name,argv[i+1],sizeof(cfg_file_name)-1);
			cfg_file_name[sizeof(cfg_file_name)-1]=0;
			i+=2;
		}
		else if (strncmp("-h",argv[i],2)==0) {
			r=1;
			break;
		}
		else if (strncmp("-v",argv[i],2)==0) {
			r=2;
			break;
		}
		else {
			fprintf(stderr, "Invalid option: %s\n\n",argv[i]);
			r=1;
			break;
		}
	}
	if (r>0) {
#ifdef POINT
		point_add(parse_cmd_cfg_e1);
#endif

		return r;
	}
	else if (r<0) {
		parse_config_file();
#ifdef POINT
		point_add(parse_cmd_cfg_e2);
#endif

		return r;
	}
	else
	{
#ifdef POINT
		point_add(parse_cmd_cfg_e3);
#endif

		return parse_config_file();
	}
}
