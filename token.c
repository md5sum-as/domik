/*
 * token_file.c
 *
 *  Created on: 21.02.2016
 *      Author: alexs
 */
#include "glob_vars.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>


char token_file_name[80]={"/usr/local/etc/token.cfg"};

static void clear_token(_token_t ** token);
static void clear_token(_token_t ** token) {
	_token_t * p=*token;

#ifdef POINT
	point_add(clear_token_s);
#endif

	//	if (loglevel>INFO) syslog(LOG_DEBUG|LOG_DAEMON,"DEBUG> clear %04X %04X",(unsigned int)p,(unsigned int)p->next);
	if (p->next!=NULL) clear_token(&(p->next));
#ifdef POINT
	point_add(clear_token_free);
#endif

	free(p);
	p=NULL;
	*token=NULL;
#ifdef POINT
	point_add(clear_token_e1);
#endif

}

static void prn_token() {
	_token_t *p;
#ifdef POINT
	point_add(prn_token_s);
#endif

	p=first_token;
	while (p!=NULL) {
		syslog(LOG_DEBUG|LOG_DAEMON,"DEBUG> Token (%04X): %02X%02X%02X%02X%02X%02X%02X:%s:%02x",(unsigned int)p,
				p->SN[0],
				p->SN[1],
				p->SN[2],
				p->SN[3],
				p->SN[4],
				p->SN[5],
				p->SN[6],
				p->val,
				p->type);
		p=p->next;
	}
#ifdef POINT
	point_add(prn_token_e1);
#endif

}


static char upcase(char c){

#ifdef POINT
	//	point_add(upcase_s);
#endif

	switch (c) {
	case 'a': //no break
	case 'A':
		return 'A';
		break;
	case 'b': //no break
	case 'B':
		return 'B';
		break;
	case 'c': //no break
	case 'C':
		return 'C';
		break;
	case 'd': //no break
	case 'D':
		return 'D';
		break;
	case 'e': //no break
	case 'E':
		return 'E';
		break;
	case 'f': //no break
	case 'F':
		return 'F';
		break;
	default:
		break;
	}
#ifdef POINT
	//	point_add(upcase_e1);
#endif

	return c;
}

static char str_to_hex(char * str){
	char tmp=0;

#ifdef POINT
	//	point_add(str_to_hex_s);
#endif

	if ((str[0]>='0')&&(str[0]<='9')) tmp=(str[0]-'0')<<4;
	if ((upcase(str[0])>='A')&&(upcase(str[0])<='F')) tmp=((upcase(str[0])-'A')+10)<<4;
	if ((str[1]>='0')&&(str[1]<='9')) tmp+=(str[1]-'0');
	if ((upcase(str[1])>='A')&&(upcase(str[1])<='F')) tmp+=(upcase(str[1])-'A')+10;
#ifdef POINT
	//	point_add(str_to_hex_e1);
#endif

	return tmp;
}

void TOKEN_load() {
	FILE * fd;
	_token_t * p=NULL;
	char str[1024];
	int i;
	char * comma1=NULL;
	char * comma2=NULL;
	//	char * comma3=NULL;

#ifdef POINT
	point_add(TOKEN_load_s);
#endif

	if (first_token!=NULL) clear_token(&first_token);

	fd=fopen(token_file_name,"r");
	if (fd == NULL) {
		syslog(LOG_ERR|LOG_DAEMON,"Cant open token file: %s\n\n",token_file_name);
#ifdef POINT
		point_add(TOKEN_load_e1);
#endif

		return;
	}

	do {
		if (fgets(str,sizeof(str),fd)==NULL) continue;
		/*
		if (str[0]=='#') continue;
		if (str[0]=='\n') continue;
		 */
		while ((comma1=strchr(str,'\n'))!=NULL) *comma1=0;
		while ((comma1=strchr(str,'\r'))!=NULL) *comma1=0;
		if (strpbrk(str,"0123456789abcdefABCDEF")!=&str[0]) continue;
		comma1=strchr(str,':');
		comma2=strchr(comma1+1,':');
		if (comma1!=&str[14]) continue;
		if (comma2==NULL) continue;

		p=malloc(sizeof(_token_t));
		p->next=first_token;
		first_token=p;
		first_token->endval=0;
		for (i=0;i<7;i++) {
			first_token->SN[i]=str_to_hex(&str[i<<1]);
		}
		comma1++;
		for (i=0;i<16;i++) {
			first_token->val[i]=comma1!=comma2?*(comma1++):0;
		}
		switch (comma2[1]) {
		case 's'://no break
		case 'S': /*smoke off token*/
			first_token->type=0x7f;
			break;
		case 'b'://no break
		case 'B': /*token is block*/
			first_token->type=0x7e;
			break;
		case 'i'://no break
		case 'I': /*No info token*/
			first_token->type=2;
			break;
		case 'c'://no break
		case 'C': /*Comp on if dual token*/
			first_token->type=3;
			break;
		default:
			first_token->type=1;
			break;
		}
		if (strlen(comma2)>4) {
			strncpy(first_token->name,&comma2[3],40);
		} else {
			sprintf(first_token->name,"No name");
		}

	} while (!feof(fd));
	fclose(fd);
	if (loglevel>INFO) prn_token();
#ifdef POINT
	point_add(TOKEN_load_e2);
#endif

}

int TOKEN_find(_token_t * token) {
	_token_t * p;

#ifdef POINT
	point_add(TOKEN_find_s);
#endif

	p=first_token;
	token->type=0xff;
	sprintf(token->name,"Noname");
	while (p!=NULL) {
		//		syslog(LOG_DEBUG|LOG_DAEMON,"DEBUG> find %04X %04X",(unsigned int)p,(unsigned int)p->next);
		if (memcmp(token->SNVAL,p->SNVAL,23)==0) {
			/*
			token->type=p->type;
			memcpy(token->name,p->name,40);
			 */
			memcpy(token,p,sizeof(_token_t));
			return p->type;
		}
		p=p->next;
	}
#ifdef POINT
	point_add(TOKEN_find_e1);
#endif

	return -1;
}
