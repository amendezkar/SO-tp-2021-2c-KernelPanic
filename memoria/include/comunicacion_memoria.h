/*
 * comunicacion_memoria.h
 *
 *  Created on: 13 sep. 2021
 *      Author: utnso
 */

#ifndef MEMORIA_INCLUDE_COMUNICACION_MEMORIA_H_
#define MEMORIA_INCLUDE_COMUNICACION_MEMORIA_H_


#include "manejo_memoria.h"

typedef enum{
	FIJA,
	DINAMICA
}t_asignacion;

typedef enum{
	CLOCK
}alg_reemplazo_mmu;

typedef enum{
	FIFO
}alg_reemplazo_tlb;

typedef struct{

    t_log* logger;
    int socket;
    char* nombreServidor;
} t_procesar_conexion_args;

uint32_t PIDGlobal;
int server_escuchar(char* server_name, int server_socket);
void procesar_conexion(void* void_args);
bool init_config();
bool generar_conexiones();

#endif /* MEMORIA_INCLUDE_COMUNICACION_MEMORIA_H_ */
