/*
 * comunicacion.h
 *
 *  Created on: 7 sep. 2021
 *      Author: utnso
 */

#ifndef SHARED_INCLUDE_COMUNICACION_H_
#define SHARED_INCLUDE_COMUNICACION_H_

#include<stdlib.h>
#include<signal.h>
#include<unistd.h>
#include<sys/socket.h>
#include<netdb.h>
#include<string.h>
#include<semaphore.h>
#include <pthread.h>
#include<commons/log.h>
#include<commons/collections/queue.h>
#include<stdbool.h>

typedef enum
{
	MENSAJE,
	PAQUETE,

	CREAR_PCB,
	ASIGNAR_PID,

	TERMINAR_EJECUCION,

	SEM_INIT,
	SEM_WAIT,
	SEM_POST,
	SEM_DESTROY,
	CALL_IO,

	MEMALLOC,
	MEMFREE,
	MEMREAD,
	MEMWRITE
}op_code;

typedef enum
{

	OFFLINE,
	KERNEL,
	MEMORIA,
	ERROR_HANDSHAKE = -1
}handshake_code;

typedef enum
{
	WAIT,
	POST,
	DESTROY,
	CLEAR,
	ELIMINAR_RETENCION
}sem_op;

typedef enum{
	SWAP_OUT,
	GET_PAG,
	RESERVAR_PAG
}op_code_mem_swamp;


typedef struct
{
	int size;
	void* stream;
} t_buffer;

typedef struct
{
	op_code codigo_operacion;
	t_buffer* buffer;
} t_paquete;

typedef char *mate_io_resource;

typedef char *mate_sem_name;

typedef int32_t mate_pointer;

typedef struct{
	mate_sem_name nombre;
	int valor;
	t_queue* listaDeEspera;
	pthread_mutex_t mutexSemaforo;
}t_semaforo;

typedef enum{
	CONTINUE,
	FOUND,
	NOT_FOUND,
	SUSPEND = 20,
	FINALIZE,
	READY_TO_FINISH
} sem_code;

typedef struct{
	mate_io_resource io;
	unsigned int duracion;
	t_queue* listaDeEspera;
	sem_t contadorEspera;
	pthread_mutex_t mutexDispositivo;
}t_io;

void enviar_mensaje(char* mensaje, int socket_cliente);
handshake_code handshakeCliente(int socketServidor, uint32_t* pid);
void handshakeServidor(int socketCliente, handshake_code codigo, t_log* logger);
void realizar_operacion_segun(int socketServidor, handshake_code resultadoHandshake, uint32_t* pid);
void* serializar_paquete(t_paquete* paquete, int bytes);
int recibir_operacion(int socket_cliente);
void recibir_mensaje(int socket_cliente,t_log* logger);
void* recibir_buffer(int* size, int socket_cliente);
void eliminar_paquete(t_paquete* paquete);

//SERIALIZACIONES

void* serializar_sem_init(size_t* size, mate_sem_name nombre, unsigned int value);
void* serializar_sem_wait(size_t* size, mate_sem_name nombre);
void* serializar_sem_post(size_t* size, mate_sem_name nombre);
void* serializar_sem_destroy(size_t* size, mate_sem_name nombre);
void* serializar_call_io(size_t* size, mate_io_resource io, char* msg);

void* serializar_memalloc(size_t* sizeStream, int size,uint32_t pid);
void* serializar_memfree(size_t* sizeStream, uint32_t direccionLogica, uint32_t pid);
void* serializar_memread(size_t* sizeStream, uint32_t direccionLogica, int size, uint32_t pid);
void* serializar_memwrite(size_t* sizeStream, uint32_t direccionLogica, void* contenido, int size, uint32_t pid);

void* serializar_contenido_memread(size_t* sizeStream, void* contenido, int size);




//DESERIALIZACIONES
void deserializar_memalloc(void* stream, int* size,uint32_t* pid);
void deserializar_memfree(void* stream, uint32_t* direccionLogica, uint32_t* pid);
void deserializar_memread(void* stream, uint32_t* direccionLogica, int* size, uint32_t* pid);
void deserializar_memwrite(void* stream, uint32_t* direccionLogica, void** contenido,int* size, uint32_t* pid);

void deserializar_contenido_memread(void* stream, void** contenido,int* size);


//RECV
bool recv_memalloc(int socketCliente,int* size,uint32_t* pid);
bool recv_memfree(int socketCliente,uint32_t* direcLogica, uint32_t* pid);
bool recv_memread(int socketCliente,uint32_t* direcLogica,int* size, uint32_t* pid);
bool recv_memwrite(int socketCliente,uint32_t* direcLogica,void** contenido,int* size, uint32_t* pid);

bool recv_direcLogica(int socketCliente, uint32_t* direcMemoria);
bool recv_respuesta_bool(int socketCliente, bool* respuesta);
bool recv_contenido_memread(int socketCliente, void** contenido,int* size);

void* recibir_buffer2(int socketCliente, int* size);
//SEND
bool send_memalloc(int socketCliente,int size,uint32_t pid);
bool send_memfree(int socketCliente,uint32_t direcLogica, uint32_t pid);
bool send_memread(int socketCliente,uint32_t direcLogica,int size, uint32_t pid);
bool send_memwrite(int socketCliente,uint32_t direcLogica,void* contenido,int size, uint32_t pid);
bool send_direcLogica(int socketCliente, uint32_t direcMemoria);
bool send_respuesta_bool(int socketCliente, bool respuesta);
bool send_contenido_memread(int socketCliente, void* contenido,int size);

#endif /* SHARED_INCLUDE_COMUNICACION_H_ */
