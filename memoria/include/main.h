/*
 * main.h
 *
 *  Created on: 8 sep. 2021
 *      Author: utnso
 */

#ifndef MEMORIA_INCLUDE_MAIN_H_
#define MEMORIA_INCLUDE_MAIN_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commons/log.h>
#include <commons/config.h>
#include "utils/log_config.h"
#include "utils/comunicacion.h"
#include "init_memoria.h"
#include "utils/sockets.h"


int socketSwamp;
int socketMemoria;
t_log* logger;
t_config* config;
t_cfg* cfg;

#endif /* MEMORIA_INCLUDE_MAIN_H_ */
