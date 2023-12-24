/*
 * sock.c
 *
 *  Created on: 20.12.2015
 *      Author: alexs
 */

#include "glob_vars.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

static struct sockaddr_un addr;
static int socket_fd;

int sock_create() {

	umask(0111);
	socket_fd=socket(AF_UNIX, SOCK_STREAM, 0);
	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, "/run/home_mon.sock", sizeof(addr.sun_path)-1);
	bind(socket_fd, (struct sockaddr*)&addr, sizeof(addr));
	listen(socket_fd,5);

	return 0;
}
