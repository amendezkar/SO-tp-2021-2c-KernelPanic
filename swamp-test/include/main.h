/*
 * main.h
 *
 *  Created on: 8 sep. 2021
 *      Author: utnso
 */

#ifndef SWAMP_INCLUDE_MAIN_H_
#define SWAMP_INCLUDE_MAIN_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commons/log.h>
#include <commons/config.h>
#include <commons/collections/list.h>
#include "utils/log_config.h"
#include "utils/sockets.h"
#include "utils/comunicacion.h"

#endif /* SWAMP_INCLUDE_MAIN_H_ */

void recieve_and_set_mode(void);
void send_add_pages(int pid, int amountOfPages, void* data, int size, int socket_cliente);
void send_add_page(int pid, int pageNumber, void* data, int size, int socket_cliente);
void send_get_page(int pid, int pageNumber, int socket_cliente);
void send_release_process(int socketClient, int pid);