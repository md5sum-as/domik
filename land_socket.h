/*
 * land_socket.h
 *
 *  Created on: 14.12.2015
 *      Author: alexs
 */

#ifndef LAND_SOCKET_H_
#define LAND_SOCKET_H_

void land_cmd (const char * cmd);
void land_clear();
void land_close();
extern char land_last_command[100];

#endif /* LAND_SOCKET_H_ */
