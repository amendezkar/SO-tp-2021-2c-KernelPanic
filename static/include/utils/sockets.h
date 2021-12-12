/*
 * sockets.h
 *
 *  Created on: 6 sep. 2021
 *      Author: utnso
 */

#ifndef SHARED_INCLUDE_SOCKETS_H_
#define SHARED_INCLUDE_SOCKETS_H_

#include<stdlib.h>
#include<signal.h>
#include<unistd.h>
#include<sys/socket.h>
#include<netdb.h>
#include<string.h>
#include<commons/log.h>

int crear_conexion(t_log* logger, const char* server_name, char* ip, char* puerto);
void liberar_conexion(int socket_cliente);
int iniciar_servidor(t_log* logger, const char* name, char* ip, char* puerto);
int esperar_cliente(t_log* logger, const char* name, int socket_servidor);

#endif /* SHARED_INCLUDE_SOCKETS_H_ */
