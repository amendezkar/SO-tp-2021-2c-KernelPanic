/*
 * comunicacion_kernel.c
 *
 *  Created on: 14 sep. 2021
 *      Author: utnso
 */

#include "comunicacion_kernel.h"

//---------------------DESERIALIZACIONES----------------------

void deserializar_sem_init(void* stream, mate_sem_name* nombre, unsigned int* valorAuxiliar){

	size_t sizeNombre;
	memcpy(&sizeNombre, stream, sizeof(size_t)); //Sacamos del stream el tamanio del nombre del semaforo

	mate_sem_name* nombreSemaforo = malloc(sizeNombre);
	memcpy(nombreSemaforo, stream+sizeof(size_t), sizeNombre);

	*nombre = malloc(sizeNombre);

	memcpy(*nombre, nombreSemaforo, sizeNombre); //Sacamos del stream el nombre del semaforo

	memcpy(valorAuxiliar, stream+sizeof(size_t)+sizeNombre, sizeof(unsigned int)); //Sacamos del stream el valor de inicializacion

	free(nombreSemaforo);
}

void deserializar_sem_wait(void* stream, mate_sem_name* nombre){

	size_t sizeNombre;
	memcpy(&sizeNombre, stream, sizeof(size_t)); //Sacamos del stream el tamanio del nombre del semaforo

	mate_sem_name nombreSemaforo = malloc(sizeNombre);
	memcpy(nombreSemaforo, stream+sizeof(size_t), sizeNombre);

	*nombre = malloc(sizeNombre);
	memcpy(*nombre, nombreSemaforo, sizeNombre); //Sacamos del stream el nombre del semaforo

	free(nombreSemaforo);
}

void deserializar_sem_post(void* stream, mate_sem_name* nombre){

	size_t sizeNombre;
	memcpy(&sizeNombre, stream, sizeof(size_t)); //Sacamos del stream el tamanio del nombre del semaforo

	mate_sem_name* nombreSemaforo = malloc(sizeNombre);
	memcpy(nombreSemaforo, stream+sizeof(size_t), sizeNombre);

	*nombre = malloc(sizeNombre);
	memcpy(*nombre, nombreSemaforo, sizeNombre); //Sacamos del stream el nombre del semaforo

	free(nombreSemaforo);
}

void deserializar_sem_destroy(void* stream, mate_sem_name* nombre){

	size_t sizeNombre;
	memcpy(&sizeNombre, stream, sizeof(size_t)); //Sacamos del stream el tamanio del nombre del semaforo

	mate_sem_name* nombreSemaforo = malloc(sizeNombre);
	memcpy(nombreSemaforo, stream+sizeof(size_t), sizeNombre);

	*nombre = malloc(sizeNombre);
	memcpy(*nombre, nombreSemaforo, sizeNombre); //Sacamos del stream el nombre del semaforo

	free(nombreSemaforo);
}

void deserializar_call_io(void* stream, mate_io_resource* io, char** msg){

	size_t sizeNombreIO;
	memcpy(&sizeNombreIO, stream, sizeof(size_t)); //Sacamos del stream el tamanio del nombre del io
	mate_io_resource nombreIO = malloc(sizeNombreIO);
	memcpy(nombreIO, stream+sizeof(size_t), sizeNombreIO);

	*io = malloc(sizeNombreIO);
	memcpy(*io, nombreIO, sizeNombreIO); //Sacamos del stream el nombre del io

	size_t sizeMsg;
	memcpy(&sizeMsg, stream+sizeof(size_t)+sizeNombreIO, sizeof(size_t)); //Sacamos del stream el tamanio del msg
	char* msgAux = malloc(sizeMsg);
	memcpy(msgAux, stream+sizeof(size_t)*2+sizeNombreIO, sizeMsg);

	*msg = malloc(sizeMsg);
	memcpy(*msg, msgAux, sizeMsg); //Sacamos del stream el msg

	free(nombreIO);
	free(msgAux);
}

