/*
 * land_socket.c
 *
 *  Created on: 14.12.2015
 *      Author: alexs
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "glob_vars.h"
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/poll.h>
#include <netinet/tcp.h>

char land_last_command[100];

static int land_connect() {
	struct hostent * host;
	struct sockaddr_in sa;
	int flags;

#ifdef POINT
	point_add(land_connect_s);
#endif

	if (land_fd==0) {
		if ((land_fd=socket(PF_INET,SOCK_STREAM,0))<0) {
			syslog(LOG_ERR|LOG_DAEMON,"Can't create socket: %d, %s",errno,strerror(errno));
#ifdef POINT
			point_add(land_connect_e1);
#endif

			return -1;
		}
	}
	if ((fcntl(land_fd,F_SETOWN,getpid()))<0) {
		syslog(LOG_ERR|LOG_DAEMON,"Can't set owner to socket: %d, %s",errno,strerror(errno));
#ifdef POINT
		point_add(land_connect_e2);
#endif

		return -1;
	}
	if ((flags=fcntl(land_fd,F_GETFL))<0) {
		syslog(LOG_ERR|LOG_DAEMON,"Can't retrieve socket flags: %d, %s",errno,strerror(errno));
#ifdef POINT
		point_add(land_connect_e3);
#endif

		return -1;
	}
	if ((fcntl(land_fd,F_SETFL,flags|O_ASYNC|O_NONBLOCK))<0) {
		syslog(LOG_ERR|LOG_DAEMON,"Can't set flags to socket: %d, %s",errno,strerror(errno));
#ifdef POINT
		point_add(land_connect_e4);
#endif

		return -1;
	}
	if ((fcntl(land_fd,F_SETSIG,SIGIO))<0) {
		syslog(LOG_ERR|LOG_DAEMON,"Can't set SIGIO to socket: %d, %s",errno,strerror(errno));
#ifdef POINT
		point_add(land_connect_e5);
#endif

		return -1;
	}

	if ((host=gethostbyname(landing_ip_chr))==NULL) {
		syslog(LOG_ERR|LOG_DAEMON,"DNS error. Try later");
#ifdef POINT
		point_add(land_connect_e6);
#endif

		return -1;
	}
#ifdef POINT
	point_add(land_connect_s1);
#endif

	sa.sin_addr=*( struct in_addr*)( host->h_addr_list[0]);
	sa.sin_family=AF_INET;
	sa.sin_port=htons(landing_port);
	if (connect(land_fd,(struct sockaddr *)&sa,sizeof(sa)) < 0) {
		if ((errno!=EISCONN)&&(errno!=EINPROGRESS)&&(errno!=EALREADY)) {
			syslog(LOG_ERR|LOG_DAEMON,"Can't connect to landing: %d, %s",errno,strerror(errno));
#ifdef POINT
			point_add(land_connect_e7);
#endif

			return -1;
		}
	}
#ifdef POINT
	point_add(land_connect_e8);
#endif

	return 0;
}

void land_cmd (const char * cmd){
	static char buff[80];
	static char len;
	char * charp;
	struct tcp_info tinfo;
	socklen_t tinfolen=sizeof(tinfo);

#ifdef POINT
	point_add(land_cmd_s);
#endif

	len=strlen(cmd);
	strncpy(land_last_command,cmd,sizeof(land_last_command));

	if (len>sizeof(buff)) len=sizeof(buff);
	if (!Landing.val) {
		land_connect();
#ifdef POINT
		point_add(land_cmd_e1);
#endif

		return;
	}
	if (Landing.val) {
		getsockopt(land_fd,SOL_TCP,TCP_INFO,&tinfo,&tinfolen);
		if (loglevel >= DEBUG1) {
			syslog(LOG_DEBUG|LOG_DAEMON,"DEBUG> TCP unacked: %u",tinfo.tcpi_unacked);
		}
		if (tinfo.tcpi_unacked>0) {
			if (loglevel >= DEBUG1) syslog(LOG_DEBUG|LOG_DAEMON,"DEBUG> TCP shutdown");
			shutdown(land_fd,SHUT_RDWR);
			if (close(land_fd)==0) land_fd=0;
			Landing.val=0;
			land_connect();
#ifdef POINT
			point_add(land_cmd_e2);
#endif

			return;
		}
	}
	strncpy(buff,cmd,sizeof(buff));
	if (loglevel >= DEBUG1) syslog(LOG_DEBUG|LOG_DAEMON,"DEBUG> sending");
	send(land_fd,buff,len,0);
	land_hello=_LAND_HELLO_T;
	if (loglevel >= DEBUG1) {
		buff[sizeof(buff)-1]=0;
		while ((charp=strchr(buff,'\r'))!=NULL) *charp='.';
		while ((charp=strchr(buff,'\n'))!=NULL) *charp='^';
		syslog(LOG_DEBUG|LOG_DAEMON,"DEBUG> Landing send: %s",buff);
	}
	pause();
#ifdef POINT
	point_add(land_cmd_e3);
#endif

}

void land_clear() {
	char buff[80];
	char * charp;
	int len;

#ifdef POINT
	point_add(land_clear_s);
#endif

	len=recv(land_fd,buff,sizeof(buff)-1,MSG_NOSIGNAL);
	buff[len]=0;
	if (loglevel >= DEBUG1) {
		while ((charp=strchr(buff,'\r'))!=NULL) *charp='.';
		while ((charp=strchr(buff,'\n'))!=NULL) *charp='^';
		syslog(LOG_DEBUG|LOG_DAEMON,"DEBUG> Landing recv: %s",buff);
	}
#ifdef POINT
	point_add(land_clear_e1);
#endif

}

void land_close() {
#ifdef POINT
	point_add(land_close_s);
#endif

	shutdown(land_fd,SHUT_RDWR);
	if (close(land_fd)==0) land_fd=0;
#ifdef POINT
	point_add(land_close_e1);
#endif

}
