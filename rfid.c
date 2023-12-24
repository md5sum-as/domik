/*
 * rfid.c
 *
 *  Created on: 21.02.2016
 *      Author: alexs
 */

#include "glob_vars.h"
#include "bcm_spi_io.h"

static void RFID_load_key();

int RFID_get_token(_token_t * token) {
	unsigned char buff[20];
	char sn_length=0;

#ifdef POINT
	point_add(RFID_get_token_s);
#endif

	token->endval=0;
	token->type=0;
	buff[0]=0x00;
	if (spi_read(&dev_pins.rfid,buff,11)) {
		//		syslog(LOG_ERR|LOG_DAEMON,"Error read RFID");
#ifdef POINT
		point_add(RFID_get_token_e1);
#endif

		return 0;
	}
	if (buff[1]==0xff) {
		syslog(LOG_INFO|LOG_DAEMON,"RFID load key");
		RFID_load_key();
#ifdef POINT
		point_add(RFID_get_token_e2);
#endif

		return 0;
	}

	sn_length=buff[2];
	if (sn_length) memcpy(token->SN,&buff[3],7);
	if (sn_length==4) {
		memset(buff,0,sizeof(buff));
		buff[0]=0x10;
		if (spi_read(&dev_pins.rfid,buff,10)) {
			syslog(LOG_ERR|LOG_DAEMON,"Error read RFID_s");
#ifdef POINT
			point_add(RFID_get_token_e3);
#endif

			return 0;
		}
		memcpy(token->val,&buff[1],8);

		memset(buff,0,sizeof(buff));
		buff[0]=0x18;
		if (spi_read(&dev_pins.rfid,buff,10)) {
			syslog(LOG_ERR|LOG_DAEMON,"Error read RFID_s");
#ifdef POINT
			point_add(RFID_get_token_e4);
#endif

			return 0;
		}
		memcpy(&(token->val[8]),&buff[1],8);

	}else{
		memset(token->val,0,16);
	}
#ifdef POINT
	point_add(RFID_get_token_e5);
#endif

	return sn_length;
}

static void RFID_load_key() {
	unsigned char buff[7];

#ifdef POINT
	point_add(RFID_load_key_s);
#endif

	buff[0]=0xff;
	memcpy(&buff[1],RFID_key,6);
	//	syslog(LOG_ERR|LOG_DAEMON,"KKK %02x %02x %02x %02x %02x %02x",buff[1],buff[2],buff[3],buff[4],buff[5],buff[6]);
	if (spi_write(&dev_pins.rfid,buff,7)) {
		syslog(LOG_ERR|LOG_DAEMON,"Error write RFID key");
#ifdef POINT
		point_add(RFID_load_key_e1);
#endif

		return;
	}
#ifdef POINT
	point_add(RFID_load_key_e2);
#endif

}
void RFID_newkey() {
	unsigned char buff[7];

#ifdef POINT
	point_add(RFID_newkey_s);
#endif

	buff[0]=0xfe;
	memcpy(&buff[1],RFID_key_NEW,6);
	if (spi_write(&dev_pins.rfid,buff,7)) {
		syslog(LOG_ERR|LOG_DAEMON,"Error write RFID new key");
#ifdef POINT
		point_add(RFID_newkey_e1);
#endif

		return;
	}
#ifdef POINT
	point_add(RFID_newkey_e2);
#endif

}

void RFID_clear_last(char led_mode) {
	unsigned char buff[2];

#ifdef POINT
	point_add(RFID_clear_last_s);
#endif

	buff[0]=0xc0;
	buff[1]=led_mode;
	if (spi_write(&dev_pins.rfid,buff,2)) {
		syslog(LOG_ERR|LOG_DAEMON,"Error write RFID clear");
#ifdef POINT
		point_add(RFID_clear_last_e1);
#endif

		return;
	}
#ifdef POINT
	point_add(RFID_clear_last_e2);
#endif

}

void RFID_write_token(char * txt) {
	unsigned char buff[18];
	//	int strl;

	//	strl=strlen(txt);
	//	if (strl>16) {strl=16;}
#ifdef POINT
	point_add(RFID_write_token_s);
#endif

	memset(buff,' ',sizeof(buff));
	buff[0]=0xb0;
	memcpy((char *)&buff[1],txt,8);
	if (spi_write(&dev_pins.rfid,buff,9)) {
		syslog(LOG_ERR|LOG_DAEMON,"Error write RFID 0xb0");
#ifdef POINT
		point_add(RFID_write_token_e1);
#endif

		return;
	}
	pause();
	memset(buff,' ',sizeof(buff));
	buff[0]=0xb1;
	memcpy((char *)&buff[1],&txt[8],8);
	if (spi_write(&dev_pins.rfid,buff,9)) {
		syslog(LOG_ERR|LOG_DAEMON,"Error write RFID 0xb1");
#ifdef POINT
		point_add(RFID_write_token_e2);
#endif

		return;
	}
	pause();

	buff[0]=0xcf;
	buff[1]=1;

	if (spi_write(&dev_pins.rfid,buff,2)) {
		syslog(LOG_ERR|LOG_DAEMON,"Error write RFID 0xcf");
#ifdef POINT
		point_add(RFID_write_token_e3);
#endif

		return;
	}
	pause();
#ifdef POINT
	point_add(RFID_write_token_e4);
#endif

}

void RFID_write_token_cancel(){
	unsigned char buff[2];

#ifdef POINT
	point_add(RFID_write_token_cancel_s);
#endif

	buff[0]=0xcf;
	buff[1]=0;
	if (spi_write(&dev_pins.rfid,buff,2)) {
		syslog(LOG_ERR|LOG_DAEMON,"Error write RFID cancel");
#ifdef POINT
		point_add(RFID_write_token_cancel_e1);
#endif

		return;
	}
#ifdef POINT
	point_add(RFID_write_token_cancel_e2);
#endif

}
int RFID_write_stat(){
	unsigned char buff[3];

#ifdef POINT
	point_add(RFID_write_stat_s);
#endif

	buff[0]=0x7f;
	if (spi_read(&dev_pins.rfid,buff,3)) {
		syslog(LOG_ERR|LOG_DAEMON,"Error read RFID status");
#ifdef POINT
		point_add(RFID_write_stat_e1);
#endif

		return 1;
	}
	//	syslog(LOG_INFO|LOG_DAEMON,"%02d\n",buff[1]);
#ifdef POINT
	point_add(RFID_write_stat_e2);
#endif

	return buff[1];
}

void RFID_check_write() {
	unsigned char buff[20];
	int i;

#ifdef POINT
	point_add(RFID_check_write_s);
#endif

	buff[0]=0x30;
	if (spi_read(&dev_pins.rfid,buff,10)) {
		syslog(LOG_ERR|LOG_DAEMON,"Error read RFID write data");
#ifdef POINT
		point_add(RFID_check_write_e1);
#endif

		return;
	}
	buff[10]=0x38;
	if (spi_read(&dev_pins.rfid,&buff[10],10)) {
		syslog(LOG_ERR|LOG_DAEMON,"Error read RFID write data");
#ifdef POINT
		point_add(RFID_check_write_e2);
#endif

		return;
	}
	for (i=0;i<20;i++) {
		syslog(LOG_INFO|LOG_DAEMON,"%02d -> %02x -> %c\n",i,buff[i],buff[i]);
	}
#ifdef POINT
	point_add(RFID_check_write_e3);
#endif

}
