/*
 * glob_vars.c
 *
 *  Created on: 13.12.2015
 *      Author: alexs
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "glob_vars.h"

int ProccessTimeOut=30;
pin_arr_t dev_pins;
volatile int fifo_have_data=0;
int fifo_fd=0;
int fd_state;

int loglevel=0;
sensor_t Landing={0,0,1,"",""};
char landing_ip_chr[80]="";
int landing_port=223;
int land_fd=0;
char land_color_light[50] ="99999999999999999999999999999999999999999999999999";
char land_color_wc[50]=    "33333333333333333333333333333333333333333333333333";
char land_color_power[50] ="44444xxxxxxxxxx44444xxxxxxxxxx44444xxxxxxxxxx44444";
char land_color_heater[50]="xxxxxxxxxxxxxxx55555555555555555555xxxxxxxxxxxxxxx";
//						   12345678901234567890123456789012345678901234567890
_onoff_t on_off_cfg;
unsigned char door_mode=0;
int Smoke_off_time=0;

/*Sensors*/
sensor_t Battery={1,1,1,"",""};
sensor_t ACPower={0,0,0,"",""};
sensor_t WaterAcnt={0,0,0,"",""};
sensor_t WaterBcnt={0,0,0,"",""};
sensor_t Neptun={0,0,0,"",""};
smoke_sensor_t SmokeA={1,1,1,0,0,0,0,"","",""};
smoke_sensor_t SmokeB={1,1,1,0,0,0,0,"","",""};
smoke_sensor_t SmokeC={1,1,1,0,0,0,0,"","",""};
sensor_door DoorA={0,0};
sensor_door DoorB={0,0};

child_t * first_chld=NULL;
int security_mode=SEC_MODE_FREE;
char security_on[1024]="";
char security_on_wh[1024]="";
char security_off[1024]="";
char security_alarm[1024]="";
char pwr_onoff_change[1024]="";
char comp_on[1024]="";
char stat_ch[1024]="";
char doorA_h[1024]="/bin/true";
char doorB_h[1024]="/bin/true";
char security_pre[1024]="/bin/true";
char security_preclr[1024]="/bin/true";

int pre_secur_timer=180;
int wait_pin_time=60;

int power_all=0;
int power_all_zero=0;
int power_all_scale=1;
int power_all_warn=1;
int power_oven_zero=0;
int power_oven=0;
char power_onoff=0;
char power_temper=0;
char power_s2c=0;
char wc_lamp=0;

_token_t * first_token=NULL;
char RFID_key[7]={0,0,0,0,0,0,0};
char RFID_key_NEW[7]={0,0,0,0,0,0,0};
int smoke_alarm=0;
int smoke_alarm_timer=0;
int smoke_alarm_timer_cfg=60;
