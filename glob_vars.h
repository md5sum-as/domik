/*
 * glob_vars.h
 *
 *  Created on: 13.12.2015
 *      Author: alexs
 */

#ifndef GLOB_VARS_H_
#define GLOB_VARS_H_
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <syslog.h>
#include <signal.h>
#include <bits/types.h>
#include <sys/wait.h>
//#include <sched.h>

//#define POINT

extern int loglevel; // Уровень информации в SYSLOG
#define ERROR 0
#define INFO 1
#define DEBUG 2
#define DEBUG1 3

#define _CHECK_MAIN_T 120
#define _LAND_HELLO_T 60
extern int land_hello;

/*Эта структура необходима для связки пина и дескриптора файла при эмуляции SPI программным методом.
Она будет использоваться и для аппаратного SPI храня только номера пинов.*/
typedef struct {
	int pin;
	int fd;
}pin_t;

/*Максимально 7 пинов - 3 для SPI (MISO, MOSI, SCK) и 4 для указания портов SS внешних плат.
 * И массив этих пинов*/
#define NUM_PINS 7
typedef union {
	struct {
		pin_t miso;
		pin_t mosi;
		pin_t sclk;
		pin_t main;
		pin_t powe;
		pin_t sens;
		pin_t rfid;
	};
	pin_t pin_arr[NUM_PINS];
} pin_arr_t;
extern pin_arr_t dev_pins;


/*_onoff_t структура возвращаемая платой расширения MAIN_BOARD при запросе включенных портов*/
typedef struct {
	unsigned char p3v;
	unsigned char p5v;
	unsigned char routerA;
	unsigned char routerB;
	unsigned char usbA;
	unsigned char usbB;
	unsigned char smokeA;
	unsigned char smokeB;
	unsigned char smokeC;
	unsigned char smokeAlarm;
	unsigned char neptunCntrl;
	unsigned char progRFID;
	unsigned char sec_mode;
} _onoff_t;

/*sensor_t В этой структуре храмим текущее, предудущее и дефолтное значение каждого сенсора,
а так-же внешние команды выполняемые при наступлении события*/
typedef struct {
	int def;
	int val;
	int old;
	char Alarm[80];
	char Clear[80];
} sensor_t;

typedef struct {
	int def;
	int val;
	char light[80];
	char power[80];
	//	char time
} landing_t;

typedef struct {
	//	int def;
	int val;
	int evn;
	//	char Open[80];
	//	char Close[80];
} sensor_door;


typedef struct {
	int def;
	int val;
	int old;
	int err;
	int eold;
	int power_time;
	int test;
	char Alarm[80];
	char Clear[80];
	char Error[80];
} smoke_sensor_t;

extern volatile int fifo_have_data; //флаг указывающий необходимость обработать команду полученную через FIFO
extern int fifo_fd;
extern int fd_state;

extern int ProccessTimeOut; //Время на выполнение внешнего процесса, после чего ему пошлется сигнал KILL
extern sensor_t Landing; // посадочная полоса готова к приему команд.
extern char landing_ip_chr[80];
extern int landing_port;
extern int land_fd;
extern char land_color_light[50];
extern char land_color_wc[50];
extern char land_color_power[50];
extern char land_color_heater[50];

extern _onoff_t on_off_cfg;
extern unsigned char door_mode;
extern int Smoke_off_time;
extern sensor_t Battery;
extern sensor_t ACPower;
extern sensor_t WaterAcnt;
extern sensor_t WaterBcnt;
extern sensor_t Neptun;
extern smoke_sensor_t SmokeA;
extern smoke_sensor_t SmokeB;
extern smoke_sensor_t SmokeC;
#define DOORclose 0;
#define DOORopen 1;
extern sensor_door DoorA;
extern sensor_door DoorB;

typedef struct child_t child_t;
typedef struct child_t
{
	pid_t pid;
	__time_t killtime;
	child_t * next;
} child_t;

extern child_t * first_chld;

#define SEC_MODE_FREE 0
#define SEC_MODE_PRE_SEC 1
#define SEC_MODE_SECURITY 2
#define SEC_MODE_WAIT_PIN 3
#define SEC_MODE_WRITE_TOKEN 4
#define SEC_MODE_PRE_SEC_SMS 5

extern int pre_secur_timer;
extern int wait_pin_time;
extern int security_mode;
extern char security_on[1024];
extern char security_on_wh[1024];
extern char security_off[1024];
extern char security_alarm[1024];
extern char pwr_onoff_change[1024];
extern char comp_on[1024];
extern char stat_ch[1024];
extern char doorA_h[1024];
extern char doorB_h[1024];
extern char security_pre[1024];
extern char security_preclr[1024];

extern int power_all;
extern int power_all_zero;
extern int power_all_scale;
extern int power_all_warn;
extern int power_oven_zero;
extern int power_oven;
extern char power_onoff;
extern char power_temper;
extern char wc_lamp;
extern char power_s2c;

typedef struct _token_t _token_t;
typedef struct _token_t {
	union {
		struct {
			char SN[7];
			char val[16];
			char endval;
		};
		char SNVAL[24];
	};
	char type;
	char name[40];
	_token_t * next;
} _token_t;
extern _token_t * first_token;
extern char RFID_key[7];
extern char RFID_key_NEW[7];
extern int smoke_alarm;
extern int smoke_alarm_timer;
extern int smoke_alarm_timer_cfg;


typedef enum {
	start,
	check_duplicate_s,check_duplicate_e1,check_duplicate_e2,
	fifo_action_s,fifo_action_e1,
	set_sigact_s,set_sigact_e1,set_sigact_e2,
	create_fifo_s,create_fifo_e1,create_fifo_e2,create_fifo_e3,create_fifo_e4,create_fifo_e5,
	sig_term_h_s,sig_term_h_e1,
	sig_alarm_h_s,sig_alarm_h_e1,
	main_s,main_e1,main_e2,main_e3,main_e4,
	parse_cmd_cfg_s,parse_cmd_cfg_e1,parse_cmd_cfg_e2,parse_cmd_cfg_e3,
	spi_init_s,spi_init_e1,
	MS_get_main_on_off_s,MS_get_main_on_off_e1,MS_get_main_on_off_e2,
	MS_set_main_on_off_s,MS_set_main_on_off_e1,MS_set_main_on_off_e2,
	MS_write_on_off_s,MS_write_on_off_e1,
	create_landing_color_s,create_landing_color_e1,
	kill_all_child_s,kill_all_child_e1,kill_all_child_e2,
	check_child_proc_s,check_child_proc_e1,
	write_mode_s,write_mode_e1,write_mode_e2,write_mode_e3,
	parse_fifo_cmd_s,parse_fifo_cmd_e1,parse_fifo_cmd_e2,parse_fifo_cmd_e3,parse_fifo_cmd_e4,parse_fifo_cmd_e5,
	parse_fifo_cmd_e6,parse_fifo_cmd_e7,parse_fifo_cmd_e8,parse_fifo_cmd_e9,parse_fifo_cmd_e10,
	parse_fifo_cmd_e11,parse_fifo_cmd_e12,parse_fifo_cmd_e13,parse_fifo_cmd_e14,parse_fifo_cmd_e15,
	parse_fifo_cmd_e16,parse_fifo_cmd_e17,parse_fifo_cmd_e18,parse_fifo_cmd_e19,parse_fifo_cmd_e20,
	parse_fifo_cmd_e21,parse_fifo_cmd_e22,parse_fifo_cmd_e23,parse_fifo_cmd_e24,parse_fifo_cmd_e25,
	spi_release_s,spi_release_e1,
	spi_xmit_s,spi_xmit_e1,
	spi_5V_s,spi_5V_e1,
	spi_3V_s,spi_3V_e1,
	spi_read_s,spi_read_e1,spi_read_e2,spi_read_e3,
	spi_write_s,spi_write_e1,spi_write_e2,spi_write_e3,
	get_irq_s,get_irq_e1,
	cfg_copy_str_s,cfg_copy_str_e1,cfg_copy_str_e2,
	parse_config_file_s,parse_config_file_e1,parse_config_file_e2,
	usage_s,usage_e1,
	_crc_ibutton_update_s,_crc_ibutton_update_e1,
	land_connect_s,land_connect_s1,land_connect_e1,land_connect_e2,land_connect_e3,
	land_connect_e4,land_connect_e5,land_connect_e6,land_connect_e7,land_connect_e8,
	land_cmd_s,land_cmd_e1,land_cmd_e2,land_cmd_e3,
	land_clear_s,land_clear_e1,
	land_close_s,land_close_e1,
	MS_get_main_sens_s,MS_get_main_sens_e1,MS_get_main_sens_e2,
	MS_get_door_mode_s,MS_get_door_mode_e1,MS_get_door_mode_e2,
	MS_get_debug8_s,MS_get_debug8_e1,MS_get_debug8_e2,
	MS_store_sens_to_file_s,MS_store_sens_to_file_e1,MS_store_sens_to_file_e2,
	MS_most_sensor_check_s,MS_most_sensor_check_e1,
	MS_smoke_sensor_check_s,MS_smoke_sensor_check_e1,
	str2arr_s,str2arr_e1,
	MS_run_event_s,MS_run_event_fork,MS_run_event_parent,MS_run_event_free,MS_run_event_e1,
	MS_humidity_s,MS_humidity_e1,MS_humidity_e2,
	PWR_get_power_s,PWR_get_power_e1,PWR_get_power_e2,
	PWR_write_power_onoff_s,PWR_write_power_onoff_e1,PWR_write_power_onoff_e2,
	PWR_power_off_abb_s,PWR_power_off_abb_e1,PWR_power_off_abb_e2,
	PWR_get_sens_curr_s,PWR_get_sens_curr_e1,PWR_get_sens_curr_e2,
	PWR_get_sens_curr_s1,PWR_get_sens_curr_s2,PWR_get_sens_curr_s3,PWR_get_sens_curr_s4,
	PWR_get_sens_curr_s5,PWR_get_sens_curr_s6,PWR_get_sens_curr_s7,
	PWR_get_wc_lamp_s,PWR_get_wc_lamp_e1,PWR_get_wc_lamp_e2,PWR_get_wc_lamp_e3,
	RFID_get_token_s,RFID_get_token_e1,RFID_get_token_e2,RFID_get_token_e3,RFID_get_token_e4,RFID_get_token_e5,
	RFID_load_key_s,RFID_load_key_e1,RFID_load_key_e2,
	RFID_newkey_s,RFID_newkey_e1,RFID_newkey_e2,
	RFID_clear_last_s,RFID_clear_last_e1,RFID_clear_last_e2,
	RFID_write_token_s,RFID_write_token_e1,RFID_write_token_e2,RFID_write_token_e3,RFID_write_token_e4,
	RFID_write_token_cancel_s,RFID_write_token_cancel_e1,RFID_write_token_cancel_e2,
	RFID_write_stat_s,RFID_write_stat_e1,RFID_write_stat_e2,
	RFID_check_write_s,RFID_check_write_e1,RFID_check_write_e2,RFID_check_write_e3,
	clear_token_s,clear_token_free,clear_token_e1,
	prn_token_s,prn_token_e1,
	upcase_s,upcase_e1,
	str_to_hex_s,str_to_hex_e1,
	TOKEN_load_s,TOKEN_load_e1,TOKEN_load_e2,
	TOKEN_find_s,TOKEN_find_e1,

} point_t;

#ifdef POINT
extern point_t point[5];
void point_add(point_t);
#endif


#endif /* GLOB_VARS_H_ */
