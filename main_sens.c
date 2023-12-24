/*
 * main_sens.c
 *
 *  Created on: 21.02.2016
 *      Author: alexs
 */

#include "glob_vars.h"
#include "bcm_spi_io.h"
#include "main.h"
#include "land_socket.h"
#include "main_sens.h"
#include <time.h>

void MS_get_main_on_off(_onoff_t * onoff){
	unsigned char buff[15];
#ifdef POINT
	point_add(MS_get_main_on_off_s);
#endif

	buff[0]=0x10;
	if (spi_read(&dev_pins.main,buff,15)) {
		syslog(LOG_ERR|LOG_DAEMON,"Error read main on/off state");
#ifdef POINT
		point_add(MS_get_main_on_off_e1);
#endif

		return;
	}
	memcpy(onoff,&buff[1],sizeof(_onoff_t));
#ifdef POINT
	point_add(MS_get_main_on_off_e2);
#endif

}

void MS_set_main_on_off(unsigned char addr, unsigned char val) {
	unsigned char buff[2];
#ifdef POINT
	point_add(MS_set_main_on_off_s);
#endif

	buff[0]=addr;
	buff[1]=val;
	if (spi_write(&dev_pins.main,buff,2)) {
		syslog(LOG_ERR|LOG_DAEMON,"Error write main on/off state");
#ifdef POINT
		point_add(MS_set_main_on_off_e1);
#endif

		return;
	}
#ifdef POINT
	point_add(MS_set_main_on_off_e2);
#endif

}

void MS_get_main_sens() {
	unsigned char ms_buff[6];
	//	FILE *f_tmp;

#ifdef POINT
	point_add(MS_get_main_sens_s);
#endif

	ms_buff[0]=0x00;
	if (spi_read(&dev_pins.main,ms_buff,6)) {
		syslog(LOG_ERR|LOG_DAEMON,"Error read main sensors");
#ifdef POINT
		point_add(MS_get_main_sens_e1);
#endif

		return;
	}

	if (ms_buff[1]&0x40) {
		syslog(LOG_DEBUG|LOG_DAEMON,"DEBUG> buff[0]: %02X",ms_buff[0]);
		syslog(LOG_DEBUG|LOG_DAEMON,"DEBUG> buff[1]: %02X",ms_buff[1]);
	}

	Battery.val=(ms_buff[1]>>7)&0x01;
	ACPower.val=(ms_buff[1]>>6)&0x01;
	WaterBcnt.val=(ms_buff[1]>>5)&0x01;
	WaterAcnt.val=(ms_buff[1]>>4)&0x01;
	Neptun.val=(ms_buff[1]>>3)&0x01;
	SmokeC.val=(ms_buff[1]>>2)&0x01;
	SmokeB.val=(ms_buff[1]>>1)&0x01;
	SmokeA.val=(ms_buff[1]>>0)&0x01;
	if (SmokeA.test) {SmokeA.val=SmokeA.def+1;}
	if (SmokeB.test) {SmokeB.val=SmokeB.def+1;}
//	if (SmokeC.test) {SmokeC.val=SmokeC.def+1;}

	/*3- включен и работает, 2- включен и в обрыве, 1- такого быть не должно, 0- выключен*/
	SmokeA.err=((ms_buff[3]>>3)&0x02) + ((ms_buff[3]>>0)&0x01);
	SmokeB.err=((ms_buff[3]>>4)&0x02) + ((ms_buff[3]>>1)&0x01);
	SmokeC.err=((ms_buff[3]>>5)&0x02) + ((ms_buff[3]>>2)&0x01);

	//	syslog(LOG_DEBUG|LOG_DAEMON,"DEBUG> smokeA %02X %02X",SmokeA.err,SmokeA.val);
	//	syslog(LOG_DEBUG|LOG_DAEMON,"DEBUG> smokeB %02X %02X",SmokeB.err,SmokeB.val);
	//	syslog(LOG_DEBUG|LOG_DAEMON,"DEBUG> smokeC %02X %02X",SmokeC.err,SmokeC.val);

	DoorB.val=(ms_buff[2]>>1)&0x01;
	DoorB.evn=(ms_buff[2]>>3)&0x01;
	DoorA.evn=(ms_buff[2]>>2)&0x01;
	if (door_mode) {
		DoorA.val=(ms_buff[2]>>4)&0x0f;
	}else{
		DoorA.val=(ms_buff[2]>>0)&0x01;
	}
#ifdef POINT
	point_add(MS_get_main_sens_e2);
#endif

}

int MS_get_door_mode(unsigned char * door_mode){
	unsigned char buff[3];
#ifdef POINT
	point_add(MS_get_door_mode_s);
#endif

	buff[0]=0x04;
	if (spi_read(&dev_pins.main,buff,3)) {
		syslog(LOG_ERR|LOG_DAEMON,"Error read door mode");
#ifdef POINT
		point_add(MS_get_door_mode_e1);
#endif

		return 1;
	}
	*door_mode=buff[1];
#ifdef POINT
	point_add(MS_get_door_mode_e2);
#endif

	return 0;
}

void MS_get_debug8(){
	unsigned char buff[3];
	static FILE * debug8_file=NULL;

#ifdef POINT
	point_add(MS_get_debug8_s);
#endif

	buff[0]=0x05;
	if (spi_read(&dev_pins.main,buff,3)) {
		syslog(LOG_ERR|LOG_DAEMON,"Error read debug8");
#ifdef POINT
		point_add(MS_get_debug8_e1);
#endif

		return;
	}
	//	*debug8=buff[1];
	if (debug8_file==NULL)
		if ((debug8_file=fopen("/tmp/domik.debug8","w"))==NULL) {
			syslog(LOG_ERR|LOG_DAEMON,"Can't create status file: /tmp/domik.debug8 (%s)",strerror(errno));
			return;
		}
	fprintf(debug8_file,"Val: %d\n",buff[1]);
	fflush(debug8_file);
	//	fgetpos(f_power,&pos_tmp);
	ftruncate(fileno(debug8_file),ftell(debug8_file));
	rewind(debug8_file);
#ifdef POINT
	point_add(MS_get_debug8_e2);
#endif

	return;
}

void MS_store_sens_to_file() {
	FILE * f_sens=NULL;
	FILE * f_door_log=NULL;

	int store_door_logA=0;
	int store_door_logB=0;

#ifdef POINT
	point_add(MS_store_sens_to_file_s);
#endif

	//	struct timeval tm;
	//	struct tm tma;

	/*
	if ((DoorA.evn)||(DoorB.evn)) {
		gettimeofday(&tm,NULL);
		store_door_logA=DoorA.evn?1:0;
		store_door_logB=DoorB.evn?1:0;
		if ((f_door_log=fopen("/tmp/domik.door.log","a"))==NULL) {
			syslog(LOG_ERR|LOG_DAEMON,"Can't create status file: /tmp/domik.door.log (%s)",strerror(errno));
			//			return;
			store_door_logA=0;
			store_door_logB=0;
		}else{
			gettimeofday(&tm,NULL);
			localtime_r(&tm.tv_sec,&tma);
			fprintf(f_door_log,"%02d/%02d/%04d %02d:%02d:%02d : ",tma.tm_mday,tma.tm_mon+1,tma.tm_year+1900,tma.tm_hour,tma.tm_min,tma.tm_sec);
		}
	}
	 */
	if ((f_sens=fopen("/tmp/domik.sens.tmp","w"))==NULL) {
		syslog(LOG_ERR|LOG_DAEMON,"Can't create status file: /tmp/domik.sens.tmp (%s)",strerror(errno));
#ifdef POINT
		point_add(MS_store_sens_to_file_e1);
#endif

		return;
	}
	fprintf(f_sens,"Bat:\t%s\n",(Battery.val==Battery.def)?"Ok":"Alarm");
	fprintf(f_sens,"AC:\t%s\n",(ACPower.val==ACPower.def)?"Ok":"Alarm");
	fprintf(f_sens,"Neptun:\t%s\n",(Neptun.val==Neptun.def)?"Ok":"Alarm");

	if (SmokeA.err==2) {
		fprintf(f_sens,"SmokeA:\t%s\n","Error");
	}else if (SmokeA.err ==3) {
		fprintf(f_sens,"SmokeA:\t%s\n",(SmokeA.val==SmokeA.def)?"Ok":"Alarm");
	}else if (SmokeA.err<2) {
		fprintf(f_sens,"SmokeA:\t%s\n","Off");
	}

	if (SmokeB.err==2) {
		fprintf(f_sens,"SmokeB:\t%s\n","Error");
	}else if (SmokeB.err ==3) {
		fprintf(f_sens,"SmokeB:\t%s\n",(SmokeB.val==SmokeB.def)?"Ok":"Alarm");
	}else if (SmokeB.err<2) {
		fprintf(f_sens,"SmokeB:\t%s\n","Off");
	}

	if (SmokeC.err==2) {
		fprintf(f_sens,"SmokeC:\t%s\n","Error");
	}else if (SmokeC.err ==3) {
		fprintf(f_sens,"SmokeC:\t%s\n",(SmokeC.val==SmokeC.def)?"Ok":"Alarm");
	}else if (SmokeC.err<2) {
		fprintf(f_sens,"SmokeC:\t%s\n","Off");
	}

	if (door_mode) {
		switch (DoorA.val) {
		case 0x0e: //оба открыты
			fprintf(f_sens,"DoorA:\topen\n");
			if (store_door_logA) fprintf(f_door_log,"DoorA: open\t");
			break;
		case 0x08: //оба закрыты
			fprintf(f_sens,"DoorA:\tboth close\n");
			if (store_door_logA) fprintf(f_door_log,"DoorA: both close\t");
			break;
		case 0x0a: // Нижний
			fprintf(f_sens,"DoorA:\tlower\n");
			if (store_door_logA) fprintf(f_door_log,"DoorA: lower\t");
			break;
		case 0x0c: // Верхний
			fprintf(f_sens,"DoorA:\tupper\n");
			if (store_door_logA) fprintf(f_door_log,"DoorA: upper\t");
			break;
		default: //Error
			fprintf(f_sens,"DoorA:\tError (%x)\n",DoorA.val);
			if (store_door_logA) fprintf(f_door_log,"DoorA: Error (%x)\t",DoorA.val);
			break;
		}
	}else{
		fprintf(f_sens,"DoorA:\t%s\n",DoorA.val?"close":"open");
		if (store_door_logA) fprintf(f_door_log,"DoorA: %s\t",DoorA.val?"close":"open");
	}
	fprintf(f_sens,"DoorB:\t%s\n",DoorB.val?"close":"open");
	if (store_door_logB) fprintf(f_door_log,"DoorB: %s\t",DoorB.val?"close":"open");

	switch (security_mode) {
	case SEC_MODE_FREE:
		fprintf(f_sens,"Secur:\tOFF\n");
		if ((store_door_logA)||(store_door_logB)) fprintf(f_door_log,"Secur: OFF\n");
		break;
	case SEC_MODE_PRE_SEC:
		fprintf(f_sens,"Secur:\twait close\n");
		if ((store_door_logA)||(store_door_logB)) fprintf(f_door_log,"Secur: wait close\n");
		break;
	case SEC_MODE_PRE_SEC_SMS:
		fprintf(f_sens,"Secur:\tSMS\n");
		if ((store_door_logA)||(store_door_logB)) fprintf(f_door_log,"Secur: SMS\n");
		break;
	case SEC_MODE_SECURITY:
		fprintf(f_sens,"Secur:\tON\n");
		if ((store_door_logA)||(store_door_logB)) fprintf(f_door_log,"Secur: ON\n");
		break;
	case SEC_MODE_WAIT_PIN:
		fprintf(f_sens,"Secur:\tPIN\n");
		if ((store_door_logA)||(store_door_logB)) fprintf(f_door_log,"Secur: PIN\n");
		break;
	case SEC_MODE_WRITE_TOKEN:
		fprintf(f_sens,"Secur:\tservice\n");
		if ((store_door_logA)||(store_door_logB)) fprintf(f_door_log,"Secur: service\n");
		break;
	default:
		fprintf(f_sens,"Secur:\terror (%d)\n",security_mode);
		if ((store_door_logA)||(store_door_logB)) fprintf(f_door_log,"Secur: error (%d)\n",security_mode);
		break;
	}
	fclose(f_sens);
	if ((store_door_logA)||(store_door_logB)) fclose(f_door_log);
	rename("/tmp/domik.sens.tmp","/tmp/domik.sens");
	MS_run_event(stat_ch,"/tmp/domik.sens");

#ifdef POINT
	point_add(MS_store_sens_to_file_e2);
#endif

}

int MS_most_sensor_check(sensor_t * sens, const char * desc) {
	int r=0;
#ifdef POINT
	point_add(MS_most_sensor_check_s);
#endif

	if (sens->val != sens->old) {
		sens->old=sens->val;
		r=1;
		if (sens->val == sens->def) {
			MS_run_event(sens->Clear, desc);
			syslog(LOG_WARNING|LOG_DAEMON,"Clear sensor: %s",desc);
		}else{
			MS_run_event(sens->Alarm, desc);
			syslog(LOG_WARNING|LOG_DAEMON,"Alarm sensor: %s",desc);
		}
	}
#ifdef POINT
	point_add(MS_most_sensor_check_e1);
#endif

	return r;
}

int MS_smoke_sensor_check(smoke_sensor_t * sens, const char * desc) {
	int r=0;
#ifdef POINT
	point_add(MS_smoke_sensor_check_s);
#endif

	if (sens->err != sens->eold) {
		r=1;
		sens->eold=sens->err;
		if (sens->err == 3) {
			MS_run_event(sens->Clear, desc);
			syslog(LOG_WARNING|LOG_DAEMON,"Clear sensor: %s",desc);
		}else{
			MS_run_event(sens->Error, desc);
			syslog(LOG_WARNING|LOG_DAEMON,"Error sensor: %s",desc);
		}
	}else{
		if (sens->val != sens->old) {
			sens->old=sens->val;
			r=1;
			if (sens->val == sens->def) {
				MS_run_event(sens->Clear, desc);
				syslog(LOG_WARNING|LOG_DAEMON,"Clear sensor: %s",desc);
			}else{
				MS_run_event(sens->Alarm, desc);
				syslog(LOG_WARNING|LOG_DAEMON,"Alarm sensor: %s",desc);
			}
		}
	}
#ifdef POINT
	point_add(MS_smoke_sensor_check_e1);
#endif

	return r;
}

static char ** str2arr(const char * tmp) {
	int i=0;
	char * str;
	char * p;
	char * fc=NULL;
	char search=' ';
	char **arr=NULL;

#ifdef POINT
	point_add(str2arr_s);
#endif

	str=strdup(tmp);
	if ((p=strchr(str,'\n'))!=NULL) *p=0;
	if ((p=strchr(str,'\r'))!=NULL) *p=0;
	while ((p=strchr(str,'\t'))!=NULL) *p=' ';
	p=str;
	while (*p!=0) {
		if (*p==search) {
			if (fc==NULL) {
				p++;
				continue;
			}else{
				*p=0;
				arr=(char **)realloc(arr,(i+1)*sizeof(char *));
				arr[i]=fc;
				fc=NULL;
				i++;
				if (i==511) *(p+1)=0;
				if (search!=' ') search=' ';
			}
		}else{
			if (((*p=='"')||(*p=='\''))&&(search==' ')) {
				search=*p;
				p++;
				continue;
			}
			if (fc==NULL) fc=p;
		}
		p++;
	}
	if (fc!=NULL) {
		arr=(char **)realloc(arr,(i+1)*sizeof(char *));
		arr[i]=strdup(fc);
		i++;
	}
	arr=(char **)realloc(arr,(i+1)*sizeof(char *));
	arr[i]=NULL;
#ifdef POINT
	point_add(str2arr_e1);
#endif

	return arr;
}

int MS_run_event(const char * const_cmdi, const char * const_desc) {
	struct timeval tm;
	pid_t pid;
	char cmd[1024]="";
	char * dog;
	char * copy_position;
	char * cmdi;
	char * desc;
	child_t * ch;
	char ** params;

#ifdef POINT
	point_add(MS_run_event_s);
#endif

	cmdi=strdup(const_cmdi);
	desc=strdup(const_desc);
	copy_position=cmdi;

	do {
		dog=strchr(copy_position,'@');
		if (dog==NULL) {
			strncat(cmd,copy_position,(sizeof(cmd)-strlen(cmd)));
		}else{
			*dog=0;
			strncat(cmd,copy_position,(sizeof(cmd)-strlen(cmd)));
			strncat(cmd,desc,(sizeof(cmd)-strlen(cmd)));
			copy_position=dog+1;
		}
	}while (dog!=NULL);

	cmd[1023]=0;
	if (loglevel >= INFO) syslog(LOG_DEBUG|LOG_DAEMON,"DEBUG> cmd: %s",cmd);

#ifdef POINT
	point_add(MS_run_event_fork);
#endif

	pid=fork();
	if (!pid) {
		setpgrp();
		close(0);
		close(1);
		close(2);

		params=str2arr(cmd);
		execv(params[0],params);
		exit (0);
	}else if (pid>0) {
#ifdef POINT
		point_add(MS_run_event_parent);
#endif

		ch=malloc(sizeof(child_t));
		ch->next=first_chld;
		first_chld=ch;
		first_chld->pid=pid;
		gettimeofday(&tm,NULL);
		first_chld->killtime=tm.tv_sec+ProccessTimeOut;
		if (loglevel >= DEBUG) syslog(LOG_DEBUG|LOG_DAEMON,"DEBUG> Start process init by %s, pid: %d (%s)",desc,pid,cmd);
	}

#ifdef POINT
	point_add(MS_run_event_free);
#endif

	free(cmdi);
	free(desc);
#ifdef POINT
	point_add(MS_run_event_e1);
#endif

	return 0;
}

int MS_write_on_off() {
	FILE * f_tmp;

#ifdef POINT
	point_add(MS_write_on_off_s);
#endif

	if ((f_tmp=fopen("/tmp/domik.onoff.tmp","w"))==NULL) {
		syslog(LOG_ERR|LOG_DAEMON,"Can't create status file: /tmp/domik.sens.tmp (%s)",strerror(errno));
	}else{
		fprintf(f_tmp,"3V: %s\n",(on_off_cfg.p3v)?"On":"Off");
		fprintf(f_tmp,"5V: %s\n",(on_off_cfg.p5v)?"On":"Off");
		fprintf(f_tmp,"Router_A: %s\n",(on_off_cfg.routerA)?"On":"Off");
		fprintf(f_tmp,"Router_B: %s\n",(on_off_cfg.routerB)?"On":"Off");
		fprintf(f_tmp,"USB_A: %s\n",(on_off_cfg.usbA)?"On":"Off");
		fprintf(f_tmp,"USB_B: %s\n",(on_off_cfg.usbB)?"On":"Off");
		fprintf(f_tmp,"Smoke_A: %s\n",(on_off_cfg.smokeA)?"On":"Off");
		fprintf(f_tmp,"Smoke_B: %s\n",(on_off_cfg.smokeB)?"On":"Off");
		fprintf(f_tmp,"Smoke_C: %s\n",(on_off_cfg.smokeC)?"On":"Off");
		fprintf(f_tmp,"Smoke_alarm: %s\n",(on_off_cfg.smokeAlarm)?"On":"Off");
		fprintf(f_tmp,"Neptun_pwr: %s\n",(on_off_cfg.neptunCntrl)?"On":"Off");
		fprintf(f_tmp,"RFID_prog: %s\n",(on_off_cfg.progRFID)?"Off":"On");

		fclose(f_tmp);
		rename("/tmp/domik.onoff.tmp","/tmp/domik.onoff");
		MS_run_event(stat_ch,"/tmp/domik.onoff");
	}
#ifdef POINT
	point_add(MS_write_on_off_e1);
#endif

	return 0;
}

void MS_humidity() {
	static FILE * f_humm=NULL;
	unsigned char buff[8];
	static int hum=0,h_temp=0;
	int old_hum,old_h_temp;

#ifdef POINT
	point_add(MS_humidity_s);
#endif

	buff[0]=0x10;
	if (spi_read(&dev_pins.sens,buff,6)) {
		syslog(LOG_ERR|LOG_DAEMON,"Error read add sensors");
#ifdef POINT
		point_add(MS_humidity_e1);
#endif

		return;
	}

	//	syslog(LOG_ERR|LOG_DAEMON,"T  %d",power_temper);

	old_hum=hum;
	old_h_temp=h_temp;
	hum=(int)(((((buff[1]<<8)|buff[2])/(double)0x10000)*125.0)-6.0);
	h_temp=(int)(((((buff[3]<<8)|buff[4])/(double)0x10000)*175.72)-46.85);
	if ((old_hum!=hum)||(old_h_temp!=h_temp)) {
	    if (f_humm==NULL)
		if ((f_humm=fopen("/tmp/domik.humidity","w"))==NULL) {
			syslog(LOG_ERR|LOG_DAEMON,"Can't create status file: /tmp/domik.humidity (%s)",strerror(errno));
			return;
		}
	    fprintf(f_humm,"Humidity: %d\nH_Temp: %d\n",hum,h_temp);
	    fflush(f_humm);
	    ftruncate(fileno(f_humm),ftell(f_humm));
	    rewind(f_humm);
    	    MS_run_event(stat_ch,"/tmp/domik.humidity");

	}
#ifdef POINT
	point_add(MS_humidity_e2);
#endif

}
