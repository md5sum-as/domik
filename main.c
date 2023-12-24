/*
 * main.c
 *
 *  Created on: 28.09.2015
 *      Author: alexs
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#define _VERSION "2.1"

#include "glob_vars.h"
#include "cfg_file.h"
#include "bcm_spi_io.h"
#include "main.h"
#include "land_socket.h"
#include "main_sens.h"
#include "power.h"
#include "rfid.h"
#include "token.h"
//#include "errors.htxt"

#ifdef POINT
point_t point[5];
void point_add(point_t a){
	point[4]=point[3];point[3]=point[2];point[2]=point[1];point[1]=point[0];point[0]=a;
}
#endif

static void check_child_proc();
static int parse_fifo_cmd();
static int write_mode();
static void create_landing_color(int first,int send);
//static void landing_flash(const int type);

int check_main=1;
int land_hello=1;

//struct timeval tm;

int second_flag=0;

#define _NO_WAIT -1
#define _WAITING 5
#define _WAITOK 0
int wait_ok=_NO_WAIT;

//Таймер для антидребезга трясущихся рук
#define RFID_TIMERN 2
_token_t mode_owner;

struct sigaction oldsasegv; /* старый обработчик SIGSEGV */

/* проверка дублирования процесса - если не можем заблокировать pid файл,
 * значит процесс уже работает. */
int check_duplicate(){
	int fd_pid;
	struct flock lock;
	//	pid_t mypid;
//	struct sched_param param;
//	cpu_set_t cpuset;

#ifdef POINT
	point_add(check_duplicate_s);
#endif

	fd_pid=open("/run/domik.pid",O_CREAT|O_WRONLY);
	if (lockf(fd_pid,F_TLOCK,0) < 0) {
		memset(&lock,0,sizeof(lock));
		fcntl(fd_pid,F_GETLK,&lock);
		printf("%s (Another process is running: %d)\n\n",strerror(errno),lock.l_pid);
		syslog(LOG_ERR|LOG_DAEMON,"%s (Another process is running: %d)",strerror(errno),lock.l_pid);
#ifdef POINT
		point_add(check_duplicate_e1);
#endif

		return 1;
/*
	}else {
		//	mypid=getpid();
		param.sched_priority = 98;
		if (sched_setscheduler(0,SCHED_FIFO,&param)!=0) {
			syslog(LOG_WARNING|LOG_DAEMON,"Can't set scheduler polisy");
		}

		CPU_ZERO(&cpuset);
		CPU_SET(3,&cpuset);
		if (sched_setaffinity(0,__NCPUBITS,&cpuset)!=0) {
			syslog(LOG_WARNING|LOG_DAEMON,"Can't set scheduler cpu sets");
		}
*/
	}

#ifdef POINT
	point_add(check_duplicate_e2);
#endif

	return 0;
}

void fifo_action(int sig_n, siginfo_t * sig_info, void * prev){
#ifdef POINT
	point_add(fifo_action_s);
#endif

	if (loglevel >= DEBUG) {
		if ((loglevel <DEBUG1)&&(sig_info->si_fd != land_fd)) syslog(LOG_DEBUG|LOG_DAEMON,"DEBUG> Signal num:%d code:%d band:%ld fd:%d",sig_info->si_signo,sig_info->si_code,sig_info->si_band,sig_info->si_fd);
	}

	if (sig_info->si_signo == SIGIO) {
		if (sig_info->si_fd == fifo_fd) {
			fifo_have_data=1;
		}
		else if ((sig_info->si_fd == land_fd)&&(land_fd>0)) {
			switch (sig_info->si_code){
			case POLL_PRI: //no break
			case POLL_MSG: //no break
			case POLL_IN:
				land_clear();
				break;
			case POLL_OUT:
				Landing.val=1;
				MS_most_sensor_check(&Landing,"Landing_POUT");
				if (loglevel >= DEBUG) syslog(LOG_DEBUG|LOG_DAEMON,"DEBUG> Landing ready");
				//land_cmd(land_last_command);
				break;
			case POLL_ERR: //no break
			case POLL_HUP:
				if (Landing.val==0) MS_most_sensor_check(&Landing,"Landing_HUP");
				land_close();
				if (loglevel >= DEBUG) syslog(LOG_DEBUG|LOG_DAEMON,"DEBUG> Landing disconnect");
				break;
			default:
				break;
			}
		}
	}
#ifdef POINT
	point_add(fifo_action_e1);
#endif

}

#ifdef POINT
void sigsegv_action(int sig_n, siginfo_t * sig_info, void * prev){
	syslog(LOG_DEBUG|LOG_DAEMON,"Segmentation Fault addr:%lx point4:%d point3:%d point2:%d point1:%d point0:%d",
			(unsigned long int)sig_info->si_addr,point[4],point[3],point[2],point[1],point[0]);
	(void)oldsasegv.sa_sigaction(sig_n,sig_info,prev);
	exit(1);
}
#endif

int set_sigact(){
	struct sigaction sa;
#ifdef POINT
	struct sigaction sasegv;
	point_add(set_sigact_s);
#endif

	sa.sa_sigaction=(void *)&fifo_action;
	sigemptyset (&sa.sa_mask);
	sa.sa_flags=SA_SIGINFO;
	if ((sigaction(SIGIO,&sa,NULL))<0) {
#ifdef POINT
		point_add(set_sigact_e1);
#endif

		return 1;
	}
	//	if ((sigaction(SIGPIPE,&sa,NULL))<0) return 1;

#ifdef POINT
	sasegv.sa_sigaction=(void *)&sigsegv_action;
	sigemptyset (&sasegv.sa_mask);
	sasegv.sa_flags=SA_SIGINFO;
	if ((sigaction(SIGSEGV,&sasegv,&oldsasegv))<0) {
		point_add(set_sigact_e2);
		return 1;
	}
#endif

	return 0;
}

int create_fifo(){
	int flags;

#ifdef POINT
	point_add(create_fifo_s);
#endif

	umask(0111);
	unlink("/run/domik");
	if ((mkfifo("/run/domik",S_IRWXU|S_IRWXG|S_IRWXO))!=0) return 1;
	if ((fifo_fd=open("/run/domik",O_RDONLY|O_CREAT|O_NONBLOCK,S_IRWXU|S_IRWXG|S_IRWXO))<0) return 1;
	if ((fcntl(fifo_fd,F_SETOWN,getpid()))<0) {
#ifdef POINT
		point_add(create_fifo_e1);
#endif

		return 1;
	}
	if ((flags=fcntl(fifo_fd,F_GETFL))<0) {
#ifdef POINT
		point_add(create_fifo_e2);
#endif

		return 1;
	}
	if ((fcntl(fifo_fd,F_SETFL,flags|O_ASYNC))<0) {
#ifdef POINT
		point_add(create_fifo_e3);
#endif

		return 1;
	}
	if ((fcntl(fifo_fd,F_SETSIG,SIGIO))<0) {
#ifdef POINT
		point_add(create_fifo_e4);
#endif

		return 1;
	}
#ifdef POINT
	point_add(create_fifo_e5);
#endif

	return set_sigact();
}

static void kill_all_child();
void sig_term_h(int sig) {
#ifdef POINT
	point_add(sig_term_h_s);
#endif

	kill_all_child();
	while (wait3(NULL,0,NULL)>0);
	spi_release();
	land_close();
	syslog(LOG_INFO|LOG_DAEMON,"Service DOMIK was stopped.");
#ifdef POINT
	point_add(sig_term_h_e1);
#endif

	exit(0);
}

void sig_alarm_h(int sig) {
	static int tmp_msec=0;

#ifdef POINT
	point_add(sig_alarm_h_s);
#endif

	if (wait_ok>0) wait_ok--;

	tmp_msec++;
	if (tmp_msec>_NUM_MSEC) {
		tmp_msec=0;
		second_flag=1;
	}
#ifdef POINT
	point_add(sig_alarm_h_e1);
#endif

}

int main(int argc, char *argv[]) {
	int r;
	_onoff_t on_off; /*массив включенных выходов на основной плате и режим охраны*/
	int need_write_onoff=0;
	int diff_on_off=0;
	int need_store_sens=0;
	int land_lamp=0;
	int security_mode_old=security_mode;
//	int security_mode_prev;
	struct itimerval delay_timer;
	int power_timer=1;
	int humidity_timer=1;
	int rfid_cnt=0;
	_token_t token;
	int rfid_timer=RFID_TIMERN;
	int secur_timer=-1;
	int secur_mode_land_flash=0;

#ifdef POINT
	point_add(main_s);
#endif

	openlog("DOMIK",LOG_PID,LOG_DAEMON);

	/* разбираем параметры запуска */
	r=parse_cmd_cfg(argc,argv);
	if (r==2){
		printf("domik ver: %s\n\n",_VERSION);
#ifdef POINT
		point_add(main_e1);
#endif

		return 0;
	}else if (r==1) {
		usage("domik");
#ifdef POINT
		point_add(main_e2);
#endif

		return 0;
	}
	if (check_duplicate()) {
#ifdef POINT
		point_add(main_e3);
#endif

		exit(1);
	}

	syslog(LOG_INFO|LOG_DAEMON,"Service DOMIK was started. Modules: MAIN%s%s%s%s. Log level is: %d",
			(dev_pins.powe.pin?", POWER":""),
			(dev_pins.sens.pin?", SENS_ADD":""),
			(dev_pins.rfid.pin?", RFID":""),
			(Landing.def?", Landing":""),
			loglevel
	);

	if (create_fifo()) {
		syslog(LOG_ERR|LOG_DAEMON,"Can't create or open FIFO file: /run/domik (%s)",strerror(errno));
#ifdef POINT
		point_add(main_e4);
#endif

		return 1;
	}
	/*Обработка ссигнала завершения - необходимо завершить все дочернии задачи*/
	signal(SIGTERM, &sig_term_h);
	signal(SIGHUP, &sig_term_h);
	signal(SIGINT, &sig_term_h);
	signal(SIGALRM, &sig_alarm_h);

	/*Инициализация SPI*/
	spi_init();

	/*Считываем последний режим охраны*/
	if ((fd_state=open("/home/domik/domik.mode",O_RDONLY))>0) {
		if ((read(fd_state,&security_mode,sizeof(security_mode)))==sizeof(security_mode)) {
			if (loglevel >= DEBUG) syslog(LOG_DEBUG|LOG_DAEMON,"DEBUG> Read /home/domik/domik.mode OK");
			close(fd_state);
			security_mode_old=security_mode;
		}else{
			close(fd_state);
			security_mode=SEC_MODE_FREE;
			security_mode_old=security_mode;
			write_mode();
		}
	}else{
		write_mode();
	}

	/*Считываем что включено*/
	/*
	need_write_onoff=1;
	if ((fd_state=open("/usr/local/run/domik.stat",O_RDONLY))>0) {
		if ((read(fd_state,&on_off,sizeof(on_off)))==sizeof(on_off)) {
			Если считали успешно, то подменим значения из конфигурации
			memcpy(&on_off_cfg,&on_off,sizeof(on_off_cfg));
			need_write_onoff=0;
			if (loglevel >= DEBUG) syslog(LOG_DEBUG|LOG_DAEMON,"DEBUG> Read /usr/local/run/domik.stat OK");
		}
		close(fd_state);
	}
	if (need_write_onoff) {
		need_write_onoff=MS_write_on_off();
	}
	 */

	do {
		/*проверим все ли питание включено или выключено в соответствии с установками.
		Если еть разница - поправим
		будем поправлять пока не будет равенства*/
		diff_on_off=0;
		memset(&on_off,0,sizeof(on_off));
		MS_get_main_on_off(&on_off);
		if (on_off.p3v != on_off_cfg.p3v) {MS_set_main_on_off(_write_3,on_off_cfg.p3v);need_write_onoff=1;diff_on_off=1;}
		if (on_off.p5v != on_off_cfg.p5v) {MS_set_main_on_off(_write_5,on_off_cfg.p5v);need_write_onoff=1;diff_on_off=1;}
		if (on_off.routerB != on_off_cfg.routerB) {MS_set_main_on_off(_write_RouterB,on_off_cfg.routerB);need_write_onoff=1;diff_on_off=1;}
		if (on_off.smokeA != on_off_cfg.smokeA) {MS_set_main_on_off(_write_SmA,on_off_cfg.smokeA);need_write_onoff=1;diff_on_off=1;}
		if (on_off.smokeB != on_off_cfg.smokeB) {MS_set_main_on_off(_write_SmB,on_off_cfg.smokeB);need_write_onoff=1;diff_on_off=1;}
		if (on_off.smokeC != on_off_cfg.smokeC) {MS_set_main_on_off(_write_SmC,on_off_cfg.smokeC);need_write_onoff=1;diff_on_off=1;}
		if (on_off.neptunCntrl != on_off_cfg.neptunCntrl) {MS_set_main_on_off(_write_Neptun,on_off_cfg.neptunCntrl);need_write_onoff=1;diff_on_off=1;}
		if (on_off.smokeAlarm != on_off_cfg.smokeAlarm) {MS_set_main_on_off(_write_SmAlarm,on_off_cfg.smokeAlarm);need_write_onoff=1;diff_on_off=1;}
		if (on_off.progRFID != 1) {MS_set_main_on_off(_write_ProgRFID,1);on_off.progRFID=1;need_write_onoff=1;diff_on_off=1;}
	} while (diff_on_off);

	/*если были изменения сохраним их на диск*/
	if (need_write_onoff) {
		memcpy(&on_off_cfg,&on_off,sizeof(on_off_cfg));
	}
	need_write_onoff=MS_write_on_off();

	if (MS_get_door_mode(&door_mode)) {
		return 1;
	}
	if (loglevel >= DEBUG) syslog(LOG_DEBUG|LOG_DAEMON,"DEBUG> Door mode is %s",(door_mode?"PFM":"normal"));

	need_store_sens=1; /*При запуске сохраним значения сенсоров*/

	memset(&delay_timer,0,sizeof(delay_timer));
	delay_timer.it_value.tv_usec=_PAUSE;
	delay_timer.it_interval.tv_usec=_PAUSE;
	setitimer(ITIMER_REAL,&delay_timer,NULL);

	TOKEN_load();
	while (1) {

		if (second_flag) {
			second_flag=0;
			//			syslog(LOG_DEBUG|LOG_DAEMON,"DEBUG> second");
			if (secur_timer) secur_timer--;
			if (secur_timer==0) {
				secur_timer=-1;
				if (security_mode==SEC_MODE_PRE_SEC) {
					security_mode=SEC_MODE_SECURITY;
					if (mode_owner.type==2) {
						if (power_onoff&0x02) MS_run_event(security_on_wh,mode_owner.name);
						else MS_run_event(security_on,mode_owner.name);
					}
				}else if (security_mode==SEC_MODE_WAIT_PIN) {
					security_mode=SEC_MODE_SECURITY;
					MS_run_event(security_alarm,"Door open, Not token.");
				}else if (security_mode==SEC_MODE_WRITE_TOKEN) {
					security_mode=SEC_MODE_FREE;
					RFID_write_token_cancel();
				}
			}

			check_main--;
			land_hello--;
			if (check_main<0) check_main=_CHECK_MAIN_T;
			if (land_hello<0) land_hello=_LAND_HELLO_T;

			if (dev_pins.powe.pin>0)
				if (!(--power_timer)) {
					PWR_get_power();
					if (land_lamp) {power_timer=1;} else {power_timer=2;}
					if (dev_pins.sens.pin) PWR_get_wc_lamp();
					if (dev_pins.sens.pin) PWR_get_sens_curr();
				}

			if (security_mode==SEC_MODE_WAIT_PIN) {
				if (land_lamp) {
					secur_mode_land_flash++;
					if (secur_mode_land_flash>1) secur_mode_land_flash=0;
					if (!secur_mode_land_flash) create_landing_color(0,1);
					if (secur_mode_land_flash==1) {
						land_cmd("\r:Chcol,32,11111111111111111111111111111111111111111111111111\r");
						//						land_hello=_LAND_HELLO_T;
					}
					//					landing_flash(2);
				}
			}else if (security_mode==SEC_MODE_FREE) {
				if (land_lamp) {
					create_landing_color(0,0);
				}
			}else if (security_mode==SEC_MODE_PRE_SEC) {
				if (land_lamp) {
					secur_mode_land_flash++;
					if (secur_mode_land_flash>1) secur_mode_land_flash=0;
					if (!secur_mode_land_flash) create_landing_color(0,1);
					if (secur_mode_land_flash==1) {
						land_cmd("\r:Chcol,32,22222222222222222222222222222222222222222222222222\r");
					}
				}
			}

			if (dev_pins.sens.pin>0)
				if (!(--humidity_timer)) {
					MS_humidity();
					humidity_timer=10;
				}
			MS_get_debug8();

			//Restore smoke power
			if (SmokeA.power_time) {
				SmokeA.power_time--;
				if (!SmokeA.power_time) {
					on_off_cfg.smokeA=1;
					SmokeA.err=3; SmokeA.eold=3;
					MS_set_main_on_off(_write_SmA,on_off_cfg.smokeA);
					syslog(LOG_INFO|LOG_DAEMON,"SmokeA power restore");
				}
			}
			if (SmokeB.power_time) {
				SmokeB.power_time--;
				if (!SmokeB.power_time) {
					on_off_cfg.smokeB=1;
					SmokeB.err=3; SmokeB.eold=3;
					MS_set_main_on_off(_write_SmB,on_off_cfg.smokeB);
					syslog(LOG_INFO|LOG_DAEMON,"SmokeB power restore");
				}
			}
			if (SmokeC.power_time) {
				SmokeC.power_time--;
				if (!SmokeC.power_time) {
					on_off_cfg.smokeC=1;
					SmokeC.err=3; SmokeC.eold=3;
					MS_set_main_on_off(_write_SmC,on_off_cfg.smokeC);
					syslog(LOG_INFO|LOG_DAEMON,"SmokeC power restore");
				}
			}


		}
		// RFID
		if (dev_pins.rfid.pin) {
			if (!(--rfid_timer)) {
				rfid_timer=RFID_TIMERN;
				if (security_mode==SEC_MODE_WRITE_TOKEN) {
					if (RFID_write_stat()==0) security_mode=SEC_MODE_FREE;
				}else{
					rfid_cnt=RFID_get_token(&token);
					if ((rfid_cnt!=0xff)&&(rfid_cnt)) {
						if (loglevel>INFO) {
							syslog(LOG_DEBUG|LOG_DAEMON,"DEBUG> RFID \"%02X%02X%02X%02X%02X%02X%02X:%s\"",
									token.SN[0],
									token.SN[1],
									token.SN[2],
									token.SN[3],
									token.SN[4],
									token.SN[5],
									token.SN[6],
									token.val);
						}
						//						if (Landing.val) {
						if (TOKEN_find(&token)>=0) {
							//landing_flash(1);
						}else{
							//landing_flash(0);
							syslog(LOG_WARNING|LOG_DAEMON,"Unknown RFID:\"%02X%02X%02X%02X%02X%02X%02X:%s\"",
									token.SN[0],
									token.SN[1],
									token.SN[2],
									token.SN[3],
									token.SN[4],
									token.SN[5],
									token.SN[6],
									token.val);
						}
						//						}

						if (token.type==0x7f) {

							if (security_mode!=SEC_MODE_FREE) {
								MS_run_event(security_alarm,"Smoke token");
								RFID_clear_last(RFID_led_fast);
							}else{
								RFID_clear_last(RFID_led_off);
								if (on_off_cfg.smokeA) {
									if (SmokeA.val!=SmokeA.def) {
										on_off_cfg.smokeA=0;
										SmokeA.err=0; SmokeA.eold=0;
										SmokeA.power_time=Smoke_off_time;
										MS_set_main_on_off(_write_SmA,on_off_cfg.smokeA);
										syslog(LOG_INFO|LOG_DAEMON,"SmokeA power off by token");
									}
								}
								if (on_off_cfg.smokeB) {
									if (SmokeB.val!=SmokeB.def) {
										on_off_cfg.smokeB=0;
										SmokeB.err=0; SmokeB.eold=0;
										SmokeB.power_time=Smoke_off_time;
										MS_set_main_on_off(_write_SmB,on_off_cfg.smokeB);
										syslog(LOG_INFO|LOG_DAEMON,"SmokeB power off by token");
									}
								}
								if (on_off_cfg.smokeC) {
									if (SmokeC.val!=SmokeC.def) {
										on_off_cfg.smokeC=0;
										SmokeC.err=0; SmokeC.eold=0;
										SmokeC.power_time=Smoke_off_time;
										MS_set_main_on_off(_write_SmC,on_off_cfg.smokeC);
										syslog(LOG_INFO|LOG_DAEMON,"SmokeC power off by token");
									}
								}
							}
						}else if (token.type==0x7e) {
							RFID_clear_last(RFID_led_off);
							if (security_mode!=SEC_MODE_FREE) {MS_run_event(security_alarm,token.name);}
						}else if (token.type<0x7e) {
							//Проверим и изменим режим.
							if (security_mode==SEC_MODE_FREE) {
								secur_timer=pre_secur_timer;
								security_mode=SEC_MODE_PRE_SEC;
								memcpy(&mode_owner,&token,sizeof(token));
							}else if (security_mode==SEC_MODE_PRE_SEC) {
								if (memcmp(token.SNVAL,mode_owner.SNVAL,23)==0) {
									if ((token.type==3)&&(secur_timer>175)) {
										syslog(LOG_DEBUG|LOG_DAEMON,"Comp on");
										MS_run_event(comp_on,token.name);
									}
									secur_timer=-1;
									security_mode=SEC_MODE_FREE;
								}else{
									RFID_clear_last(RFID_led_slow);
								}
							}else if (security_mode==SEC_MODE_WAIT_PIN) {
								//							RFID_clear_last(RFID_led_off);
								secur_timer=-1;
								security_mode=SEC_MODE_FREE;
								if (token.type>=2) MS_run_event(security_off,token.name);
								if (land_lamp) {
									create_landing_color(0,1);
								}

							}
						}else{
							if ((security_mode==SEC_MODE_FREE)||(security_mode==SEC_MODE_SECURITY)||(security_mode==SEC_MODE_WRITE_TOKEN)) {
								RFID_clear_last(RFID_led_off);
							}else if (security_mode==SEC_MODE_PRE_SEC) {
								RFID_clear_last(RFID_led_slow);
							}else if (security_mode==SEC_MODE_WAIT_PIN) {
								RFID_clear_last(RFID_led_fast);
							}
						}
					}
				}
			}
		}

		//		gettimeofday(&tm,NULL);
		if (wait_ok==_NO_WAIT)
			if ((get_irqA)||(check_main==0)) {
				wait_ok=5;
			}

		if (wait_ok==_WAITOK)
		/*if ((get_irqA)||(check_main==0))*/ {
			wait_ok=_NO_WAIT;
			MS_get_main_sens();
			MS_set_main_on_off(_clear_irqA,1);
			if (loglevel >= DEBUG1) syslog(LOG_DEBUG|LOG_DAEMON,"DEBUG> Check MAIN");

			/*Проверим что изменилось*/
			/*Батарейка*/
			need_store_sens|=MS_most_sensor_check(&Battery,"Battery");
			/*Питание 220*/
			need_store_sens|=MS_most_sensor_check(&ACPower,"AC Power");
			/*Датчик протечек*/
			need_store_sens|=MS_most_sensor_check(&Neptun,"Neptun");
			/*Датчики дыма*/
			need_store_sens|=MS_smoke_sensor_check(&SmokeA,"Smoke A");
			need_store_sens|=MS_smoke_sensor_check(&SmokeB,"Smoke B");
			need_store_sens|=MS_smoke_sensor_check(&SmokeC,"Smoke C");

			/*Двери*/

			if ((DoorA.evn)/*||(security_mode==SEC_MODE_PRE_SEC)*/) {
				need_store_sens=1;
				if (door_mode) { /*Дверь частотная*/
					if (loglevel>INFO) {
						syslog(LOG_DEBUG|LOG_DAEMON,"DEBUG> DoorA 0x%02x %d",DoorA.val,DoorA.evn);

					}

					if (DoorA.val==0x0e) { /*Оба замка открыты*/
						syslog(LOG_INFO|LOG_DAEMON,"Door open");
						MS_run_event(doorA_h,"14");
						if (Landing.val) {
							create_landing_color(1,0);
							land_lamp=1;
						}

						if (security_mode==SEC_MODE_SECURITY) {
							if (dev_pins.rfid.pin) {
								security_mode=SEC_MODE_WAIT_PIN;
								secur_timer=wait_pin_time;
							}else{
								security_mode=SEC_MODE_FREE;
								MS_run_event(security_off,token.name);
								if (land_lamp) {
									create_landing_color(0,1);
								}
							}
						}
					}

					if (DoorA.val<0x0e) { /*Один из замков закрылся - выключим свет*/
						syslog(LOG_INFO|LOG_DAEMON,"Door close");
						if (land_lamp) {
							land_cmd("\r:Cont\r");
							//							land_hello=_LAND_HELLO_T;
							land_lamp=0;
						}
					}

					if (DoorA.val==0x08) { /*Оба замка закрылись*/
						if (security_mode==SEC_MODE_FREE){
    	    						MS_run_event(doorA_h,"08");
							security_mode=SEC_MODE_SECURITY;
							strncpy(mode_owner.name,"Door close",40);
							mode_owner.type=2;
							if (power_onoff&0x02) MS_run_event(security_on_wh,mode_owner.name);
							else MS_run_event(security_on,mode_owner.name);
						}else if (security_mode==SEC_MODE_PRE_SEC) {
							security_mode=SEC_MODE_SECURITY;
							if (mode_owner.type==2) {
								if (power_onoff&0x02) MS_run_event(security_on_wh,mode_owner.name);
								else MS_run_event(security_on,mode_owner.name);
							}
						}else if (security_mode==SEC_MODE_SECURITY) {
							MS_run_event(security_alarm,"DoorA.");
						}
					}

					if (DoorA.val==0x0a) { /*Нижний закрыт*/
						MS_run_event(doorA_h,"10");
						if (security_mode==SEC_MODE_PRE_SEC) {
							security_mode=SEC_MODE_SECURITY;
							if (mode_owner.type==2) {
								if (power_onoff&0x02) MS_run_event(security_on_wh,mode_owner.name);
								else MS_run_event(security_on,mode_owner.name);
							}
						}else if (security_mode==SEC_MODE_SECURITY) {
							MS_run_event(security_alarm,"DoorA");
						}
					}

					if (DoorA.val==0x0c) { /*Верхний закрыт*/
						MS_run_event(doorA_h,"12");
						/*
						if (security_mode==SEC_MODE_SECURITY) {
							MS_run_event(security_alarm,"Door alarm");
						}
						 */
					}

					/*Дверь обычный сенсор*/
				}else{

					if (security_mode==SEC_MODE_SECURITY) {
						if (dev_pins.rfid.pin) {
							security_mode=SEC_MODE_WAIT_PIN;
							secur_timer=wait_pin_time;
						}else{
							security_mode=SEC_MODE_FREE;
							MS_run_event(security_off,token.name);
						}
					}
					if (security_mode==SEC_MODE_PRE_SEC) {
						if (DoorA.val) {
							security_mode=SEC_MODE_SECURITY;
							if (mode_owner.type==2) {
								MS_run_event(security_on,mode_owner.name);
							}
						}
					}

					if (loglevel>INFO) {
						syslog(LOG_DEBUG|LOG_DAEMON,"DEBUG> DoorA 0x%02x %d",DoorA.val,DoorA.evn);
					}
				}

			}

			if (DoorB.evn) {
				need_store_sens=1;
				if (DoorB.val==1) {
					MS_run_event(doorB_h,"1");
					if (security_mode==SEC_MODE_SECURITY) {
						MS_run_event(security_alarm,"DoorB");
					}
				}else{
					MS_run_event(doorB_h,"0");
				}
				if (loglevel>INFO) {
					syslog(LOG_DEBUG|LOG_DAEMON,"DEBUG> DoorB 0x%02x %d",DoorB.val,DoorB.evn);
				}
			}
			/*Если необходимо сохранить сенсоры*/
			if (need_store_sens) {
				need_store_sens=0;
				MS_store_sens_to_file();
			}


			/*Счетчики воды*/
			if ((WaterAcnt.val==1) && (WaterAcnt.old==0)) {
				if (WaterAcnt.def) MS_run_event(WaterAcnt.Alarm,"");
				if (loglevel>INFO) {
					syslog(LOG_DEBUG|LOG_DAEMON,"DEBUG> WaterAcnt %d",WaterAcnt.val);
				}
			}
			WaterAcnt.old=WaterAcnt.val;

			if ((WaterBcnt.val==1) && (WaterBcnt.old==0)) {
				if (WaterBcnt.def) MS_run_event(WaterBcnt.Alarm,"");
				if (loglevel>INFO) {
					syslog(LOG_DEBUG|LOG_DAEMON,"DEBUG> WaterBcnt %d",WaterBcnt.val);
				}
			}
			WaterBcnt.old=WaterBcnt.val;

			check_main=_CHECK_MAIN_T;

		} //get_irqA

		if (security_mode!=security_mode_old) {
			write_mode();
			MS_store_sens_to_file();
//			security_mode_prev=security_mode_old;
			security_mode_old=security_mode;
			if (security_mode==SEC_MODE_SECURITY) {
				if (dev_pins.rfid.pin) RFID_clear_last(RFID_led_off);
				syslog(LOG_INFO|LOG_DAEMON,"Security mode is SECURITY by %s",mode_owner.name);
			}else if (security_mode==SEC_MODE_FREE) {
				if (dev_pins.rfid.pin) RFID_clear_last(RFID_led_off);
				syslog(LOG_INFO|LOG_DAEMON,"Security mode is OFF. Token: %s",token.name);
				//if (security_mode_prev==SEC_MODE_PRE_SEC) 
				MS_run_event(security_preclr,mode_owner.name);
			}else if (security_mode==SEC_MODE_PRE_SEC_SMS) {
				secur_timer=1;
				security_mode=SEC_MODE_PRE_SEC;
				syslog(LOG_INFO|LOG_DAEMON,"Security mode is pre_sec. Token: %s",token.name);
			}else if (security_mode==SEC_MODE_PRE_SEC) {
				if (secur_timer==-1) secur_timer=pre_secur_timer;
				if (dev_pins.rfid.pin) RFID_clear_last(RFID_led_slow);
				syslog(LOG_INFO|LOG_DAEMON,"Security mode is pre_sec. Token: %s",token.name);
				MS_run_event(security_pre,mode_owner.name);
			}else if (security_mode==SEC_MODE_WAIT_PIN) {
				if (dev_pins.rfid.pin) RFID_clear_last(RFID_led_fast);
				syslog(LOG_INFO|LOG_DAEMON,"Security mode is wait for PIN");
			}else if (security_mode==SEC_MODE_WRITE_TOKEN) {
				if (secur_timer==-1) secur_timer=pre_secur_timer;
				//if (dev_pins.rfid.pin) RFID_clear_last(RFID_led_slow);
				syslog(LOG_INFO|LOG_DAEMON,"Security mode is WriteToken");
			}
		}

		/*Проверим есть ли завершенные или просроченные задачи и прибьем их*/
		if (first_chld!=NULL) check_child_proc();
		if (fifo_have_data) {
			fifo_have_data=0;
			need_write_onoff=parse_fifo_cmd();
		}
		if (need_write_onoff) need_write_onoff=MS_write_on_off();
		if (Landing.def) {
			if (land_hello==0) {
				//				land_hello=_LAND_HELLO_T;
				land_cmd("\r:Hello\r");
			}
		}
		//		sleep(20);
		pause();

	}/*While (1)*/
	return 0;
}

static void create_landing_color(int first,int send) {
	char buff[64];
	int i;
	static char lamp_old=0;
	char lamptmp=0;
	static char pwr=0;
	char pwr1=0;
	char pwr2=0;
	//	char send=0;
	static char heater=0;

#ifdef POINT
	point_add(create_landing_color_s);
#endif

	if (first) {
		lamp_old=0;
		pwr=0;
		heater=0;
		send=1;
		sprintf(buff,"\r:Color,32,%s\r",land_color_light);
	}else{
		sprintf(buff,"\r:Chcol,32,%s\r",land_color_light);

	}

	if (dev_pins.sens.pin) lamptmp=PWR_get_wc_lamp();
	if (dev_pins.powe.pin) {
		if ((power_all_zero-power_all)<power_all_warn) pwr1=0;
		if ((power_all_zero-power_all)>power_all_warn+2) pwr1=1;
		if (power_oven<power_oven_zero) {pwr2=1;} else {pwr2=0;}
	}

	if (dev_pins.sens.pin) {
		if (lamptmp!=lamp_old) {
			lamp_old=lamptmp;
			send=1;
		}
		if (lamptmp)
			for (i=0;i<50;i++) {
				if ((land_color_wc[i]!='x')&&(land_color_wc[i]!='X')) buff[i+11]=land_color_wc[i];
			}
	}

	if (dev_pins.powe.pin) {
		if (pwr!=(pwr1|pwr2)) {
			pwr=(pwr1|pwr2);
			send=1;
		}
		if (pwr)
			for (i=0;i<50;i++) {
				if ((land_color_power[i]!='x')&&(land_color_power[i]!='X')) buff[i+11]=land_color_power[i];
			}
		if (heater!=(power_onoff&0x02)) {
			heater=power_onoff&0x02;
			send=1;
		}
		if (heater)
			for (i=0;i<50;i++) {
				if ((land_color_heater[i]!='x')&&(land_color_heater[i]!='X')) buff[i+11]=land_color_heater[i];
			}

	}

	if (send) {
		land_cmd(buff);
		//		land_hello=_LAND_HELLO_T;
	}
#ifdef POINT
	point_add(create_landing_color_e1);
#endif

}

/*
static void landing_flash(const int type){
	switch (type) {
	case 0:
		land_cmd("\r:Flash2,1,100\r");
		break;
	case 1:
		land_cmd("\r:Flash2,2,40\r");
		break;
	case 2:
		land_cmd("\r:Flash2,3,1\r");
		break;

	default:
		break;
	}
}
 */

static void kill_all_child(){
	child_t * ch;

#ifdef POINT
	point_add(kill_all_child_s);
#endif

	ch=first_chld;
	if (ch==NULL) {
#ifdef POINT
		point_add(kill_all_child_e1);
#endif

		return;
	}
	while (ch!=NULL) {
		killpg(ch->pid,SIGTERM);
		ch=ch->next;
	}
#ifdef POINT
	point_add(kill_all_child_e2);
#endif

}

static void check_child_proc(){
	child_t *chp;
	child_t *ch;
	struct timeval tm;
	pid_t pid;

#ifdef POINT
	point_add(check_child_proc_s);
#endif

	chp=NULL;
	ch=first_chld;
	gettimeofday(&tm,NULL);

	while (ch!=NULL) {
		if ((pid=wait4(ch->pid,NULL,WNOHANG,NULL))>0) {
			if (chp==NULL) {first_chld=ch->next;}else{chp->next=ch->next;}
			free(ch);
			if (chp==NULL) {ch=first_chld;}else{ch=chp->next;}
			continue;
		}else{
			if (ch->killtime<tm.tv_sec) {
				killpg(ch->pid,SIGTERM);
				if (loglevel >= DEBUG) syslog(LOG_DEBUG|LOG_DAEMON,"DEBUG> Kill process %d (timeout)",ch->pid);
			}
		}
		chp=ch;
		ch=ch->next;
	}
#ifdef POINT
	point_add(check_child_proc_e1);
#endif

}

static int write_mode() {
	int fd_tmp;
#ifdef POINT
	point_add(write_mode_s);
#endif

	if ((fd_tmp=open("/home/domik/domik.mode",O_WRONLY|O_CREAT))<0) {
		syslog(LOG_ERR|LOG_DAEMON,"Can't create status file: /home/domik/domik.mode (%s)",strerror(errno));
#ifdef POINT
		point_add(write_mode_e1);
#endif

		return 1;
	}
	if ((write(fd_tmp,&security_mode,sizeof(security_mode)))==sizeof(security_mode)) {
		close(fd_tmp);
		//		lseek(fd_state,0,SEEK_SET);
		if (loglevel >= DEBUG) syslog(LOG_DEBUG|LOG_DAEMON,"DEBUG> Write /home/domik/domik.mode OK");
	}else{
#ifdef POINT
		point_add(write_mode_e2);
#endif

		return 1;
	}
#ifdef POINT
	point_add(write_mode_e3);
#endif

	return 0;
}


static int parse_fifo_cmd() {
	char buff[100];
	char tmpbuff[10];
	int i;
	char * charp;

#ifdef POINT
	point_add(parse_fifo_cmd_s);
#endif

	i=read(fifo_fd,buff,sizeof(buff)-1);
	buff[i]=0;
	buff[sizeof(buff)-1]=0;
	if (i<=0) {
#ifdef POINT
		point_add(parse_fifo_cmd_e1);
#endif

		return 0;
	}

	while (read(fifo_fd,tmpbuff,sizeof(tmpbuff))) {}

	if (strncasecmp("help",buff,4)==0) {
		syslog(LOG_INFO|LOG_DAEMON,"   *** Domik help:");
		syslog(LOG_INFO|LOG_DAEMON,"       3V on/off");
		syslog(LOG_INFO|LOG_DAEMON,"       5V on/off");
		syslog(LOG_INFO|LOG_DAEMON,"       Router_A off");
		syslog(LOG_INFO|LOG_DAEMON,"       Router_B on/off");
		syslog(LOG_INFO|LOG_DAEMON,"       USB_A on/off");
		syslog(LOG_INFO|LOG_DAEMON,"       USB_B on/off");
		syslog(LOG_INFO|LOG_DAEMON,"       Smoke_A on/off");
		syslog(LOG_INFO|LOG_DAEMON,"       Smoke_B on/off");
		syslog(LOG_INFO|LOG_DAEMON,"       Smoke_C on/off");
		syslog(LOG_INFO|LOG_DAEMON,"       Smoke_test_A on/off");
		syslog(LOG_INFO|LOG_DAEMON,"       Smoke_test_B on/off");
		syslog(LOG_INFO|LOG_DAEMON,"       Smoke_test_C on/off");
		//		syslog(LOG_INFO|LOG_DAEMON,"       Smoke_alarm on/off");
		syslog(LOG_INFO|LOG_DAEMON,"       Neptun on/off");
		syslog(LOG_INFO|LOG_DAEMON,"       RFID_prog on/off");
		syslog(LOG_INFO|LOG_DAEMON,"       To secure");
		syslog(LOG_INFO|LOG_DAEMON,"       Landing_cmd:");
		syslog(LOG_INFO|LOG_DAEMON,"       Kitchen off");
		syslog(LOG_INFO|LOG_DAEMON,"       WaterHeat off");
		syslog(LOG_INFO|LOG_DAEMON,"       Air cond off");
		syslog(LOG_INFO|LOG_DAEMON,"       Washing off");
		syslog(LOG_INFO|LOG_DAEMON,"       Oven off");
		syslog(LOG_INFO|LOG_DAEMON,"       All ABB off");
		syslog(LOG_INFO|LOG_DAEMON,"       Reload RFID");
#ifdef POINT
		point_add(parse_fifo_cmd_e2);
#endif

		return 0;
	}

	if (strncasecmp("3V ",buff,3)==0) {
		if (strncasecmp("on",buff+3,2)==0) {on_off_cfg.p3v=1;}
		if (strncasecmp("off",buff+3,3)==0) {on_off_cfg.p3v=0;}
		MS_set_main_on_off(_write_3,on_off_cfg.p3v);
		//		spi_on_off_3V(on_off_cfg.p3v);
#ifdef POINT
		point_add(parse_fifo_cmd_e3);
#endif

		return 1;
	}

	if (strncasecmp("5V ",buff,3)==0) {
		if (strncasecmp("on",buff+3,2)==0) {on_off_cfg.p5v=1;}
		if (strncasecmp("off",buff+3,3)==0) {on_off_cfg.p5v=0;}
		MS_set_main_on_off(_write_5,on_off_cfg.p5v);
		//		spi_on_off_5V(on_off_cfg.p5v);
#ifdef POINT
		point_add(parse_fifo_cmd_e4);
#endif

		return 1;
	}

	if (strncasecmp("Router_A off",buff,12)==0) {
//		on_off_cfg.routerA=0;
		MS_set_main_on_off(_write_RouterA,0);
		//		spi_on_off_5V(on_off_cfg.p5v);
#ifdef POINT
		point_add(parse_fifo_cmd_e5);
#endif

		return 0;
	}

	if (strncasecmp("Router_B ",buff,9)==0) {
		if (strncasecmp("on",buff+9,2)==0) {on_off_cfg.routerB=1;}
		if (strncasecmp("off",buff+9,3)==0) {on_off_cfg.routerB=0;}
		MS_set_main_on_off(_write_RouterB,on_off_cfg.routerB);
#ifdef POINT
		point_add(parse_fifo_cmd_e6);
#endif

		return 1;
	}

	if (strncasecmp("USB_A ",buff,6)==0) {
		if (strncasecmp("on",buff+6,2)==0) {on_off_cfg.usbA=1;}
		if (strncasecmp("off",buff+6,3)==0) {on_off_cfg.usbA=0;}
		MS_set_main_on_off(_write_USBA,on_off_cfg.usbA);
#ifdef POINT
		point_add(parse_fifo_cmd_e7);
#endif

		return 1;
	}
	if (strncasecmp("USB_B ",buff,6)==0) {
		if (strncasecmp("on",buff+6,2)==0) {on_off_cfg.usbB=1;}
		if (strncasecmp("off",buff+6,3)==0) {on_off_cfg.usbB=0;}
		MS_set_main_on_off(_write_USBB,on_off_cfg.usbB);
#ifdef POINT
		point_add(parse_fifo_cmd_e8);
#endif

		return 1;
	}

	if (strncasecmp("Smoke_A ",buff,8)==0) {
		if (strncasecmp("on",buff+8,2)==0) {on_off_cfg.smokeA=1;}
		if (strncasecmp("off",buff+8,3)==0) {on_off_cfg.smokeA=0;}
		MS_set_main_on_off(_write_SmA,on_off_cfg.smokeA);
#ifdef POINT
		point_add(parse_fifo_cmd_e9);
#endif

		return 1;
	}
	if (strncasecmp("Smoke_B ",buff,8)==0) {
		if (strncasecmp("on",buff+8,2)==0) {on_off_cfg.smokeB=1;}
		if (strncasecmp("off",buff+8,3)==0) {on_off_cfg.smokeB=0;}
		MS_set_main_on_off(_write_SmB,on_off_cfg.smokeB);
#ifdef POINT
		point_add(parse_fifo_cmd_e10);
#endif

		return 1;
	}
	if (strncasecmp("Smoke_C ",buff,8)==0) {
		if (strncasecmp("on",buff+8,2)==0) {on_off_cfg.smokeC=1;}
		if (strncasecmp("off",buff+8,3)==0) {on_off_cfg.smokeC=0;}
		MS_set_main_on_off(_write_SmC,on_off_cfg.smokeC);
#ifdef POINT
		point_add(parse_fifo_cmd_e11);
#endif

		return 1;
	}
	if (strncasecmp("Smoke_Alarm ",buff,12)==0) {
		if (strncasecmp("on",buff+12,2)==0) {on_off_cfg.smokeAlarm=1;}
		if (strncasecmp("off",buff+12,3)==0) {on_off_cfg.smokeAlarm=0;}
		MS_set_main_on_off(_write_SmAlarm,on_off_cfg.smokeAlarm);
#ifdef POINT
		point_add(parse_fifo_cmd_e12);
#endif

		return 1;
	}
	if (strncasecmp("Smoke_test_A ",buff,13)==0) {
		if (strncasecmp("on",buff+13,2)==0) {SmokeA.test=1;}
		if (strncasecmp("off",buff+13,3)==0) {SmokeA.test=0;}
		check_main=1;
		syslog(LOG_INFO|LOG_DAEMON,"SmokeA test mode %s",buff+13);
#ifdef POINT
		point_add(parse_fifo_cmd_e9);
#endif
		return 1;
	}
	if (strncasecmp("Smoke_test_B ",buff,13)==0) {
		if (strncasecmp("on",buff+13,2)==0) {SmokeB.test=1;}
		if (strncasecmp("off",buff+13,3)==0) {SmokeB.test=0;}
		check_main=1;
		syslog(LOG_INFO|LOG_DAEMON,"SmokeB test mode %s",buff+13);
#ifdef POINT
		point_add(parse_fifo_cmd_e9);
#endif
		return 1;
	}
	if (strncasecmp("Smoke_test_C ",buff,13)==0) {
		if (strncasecmp("on",buff+13,2)==0) {SmokeC.test=1;}
		if (strncasecmp("off",buff+13,3)==0) {SmokeC.test=0;}
		check_main=1;
		syslog(LOG_INFO|LOG_DAEMON,"SmokeC test mode %s",buff+13);
#ifdef POINT
		point_add(parse_fifo_cmd_e9);
#endif
		return 1;
	}

	if (strncasecmp("Neptun ",buff,7)==0) {
		if (strncasecmp("on",buff+7,2)==0) {on_off_cfg.neptunCntrl=1;}
		if (strncasecmp("off",buff+7,3)==0) {on_off_cfg.neptunCntrl=0;}
		MS_set_main_on_off(_write_Neptun,on_off_cfg.neptunCntrl);
#ifdef POINT
		point_add(parse_fifo_cmd_e13);
#endif

		return 1;
	}

	if (strncasecmp("RFID_prog ",buff,10)==0) {
		if (strncasecmp("on",buff+10,2)==0) {on_off_cfg.progRFID=0;}
		if (strncasecmp("off",buff+10,3)==0) {on_off_cfg.progRFID=1;}
		MS_set_main_on_off(_write_ProgRFID,on_off_cfg.progRFID);
#ifdef POINT
		point_add(parse_fifo_cmd_e14);
#endif

		return 1;
	}

	if (strncasecmp("To secure",buff,9)==0) {
		security_mode=SEC_MODE_PRE_SEC_SMS;
		check_main=_CHECK_MAIN_T;
		strncpy(mode_owner.name,"SMS",40);
		if (strlen(buff)>10) {
		    strncpy(mode_owner.name,buff+10,40);
		}
		mode_owner.type=2;
#ifdef POINT
		point_add(parse_fifo_cmd_e15);
#endif

		return 0;
	}


	if (strncasecmp("Landing_cmd:",buff,12)==0) {
		buff[0]='\r';
		strncpy(&buff[1],&buff[11],sizeof(buff)-12);
		while ((charp=strchr(buff,'\n'))!=NULL) *charp='\r';
		land_cmd(buff);
		land_hello=_LAND_HELLO_T;
		if (loglevel >= DEBUG) {
			while ((charp=strchr(buff,'\r'))!=NULL) *charp='.';
			syslog(LOG_DEBUG|LOG_DAEMON,"DEBUG> Land cmd: %s",buff);
		}
#ifdef POINT
		point_add(parse_fifo_cmd_e16);
#endif

		return 0;
	}

	if (strncasecmp("Kitchen off",buff,11)==0) {
		PWR_power_off_abb(0xa0);
#ifdef POINT
		point_add(parse_fifo_cmd_e17);
#endif

		return 1;
	}
	if (strncasecmp("WaterHeat off",buff,11)==0) {
		PWR_power_off_abb(0xa1);
#ifdef POINT
		point_add(parse_fifo_cmd_e18);
#endif

		return 1;
	}
	if (strncasecmp("Air cond off",buff,12)==0) {
		PWR_power_off_abb(0xa2);
#ifdef POINT
		point_add(parse_fifo_cmd_e19);
#endif

		return 1;
	}
	if (strncasecmp("Washing off",buff,11)==0) {
		PWR_power_off_abb(0xa3);
#ifdef POINT
		point_add(parse_fifo_cmd_e20);
#endif

		return 1;
	}
	if (strncasecmp("Oven off",buff,8)==0) {
		PWR_power_off_abb(0xa4);
#ifdef POINT
		point_add(parse_fifo_cmd_e21);
#endif

		return 1;
	}
	if (strncasecmp("All ABB off",buff,11)==0) {
		PWR_power_off_abb(0xaf);
#ifdef POINT
		point_add(parse_fifo_cmd_e22);
#endif

		return 1;
	}
	if (strncasecmp("Reload RFID",buff,11)==0) {
		TOKEN_load();
#ifdef POINT
		point_add(parse_fifo_cmd_e23);
#endif

		return 1;
	}
	if (strncasecmp("Write token:",buff,12)==0) {
		if (security_mode==SEC_MODE_FREE) {
			//			syslog(LOG_DEBUG|LOG_DAEMON,"DEBUG> Write token str");
			RFID_write_token(&buff[12]);
			security_mode=SEC_MODE_WRITE_TOKEN;
			//			syslog(LOG_DEBUG|LOG_DAEMON,"DEBUG> Read token str");
			//			RFID_check_write();
		}
#ifdef POINT
		point_add(parse_fifo_cmd_e24);
#endif

		return 1;
	}
	if (strncasecmp("Fan_mode ",buff,9)==0) {
		if (strncasecmp("on",buff+9,2)==0) {PWR_set_fan(1);}
		if (strncasecmp("off",buff+9,3)==0) {PWR_set_fan(0);}
		if (strncasecmp("auto",buff+9,4)==0) {PWR_set_fan(2);}
		return 1;
	}

#ifdef POINT
	point_add(parse_fifo_cmd_e25);
#endif

	return 0;
}
