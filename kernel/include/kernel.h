/*
 * main.h
 *
 *  Created on: 8 sep. 2021
 *      Author: utnso
 */

#ifndef KERNEL_INCLUDE_KERNEL_H_
#define KERNEL_INCLUDE_KERNEL_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commons/log.h>
#include <commons/config.h>
#include <commons/collections/list.h>
#include <semaphore.h>
#include "planificacion.h"
#include "utils/utilidades.h"

uint32_t pidKernel;
t_log* logger;
t_config* config;
int socketServidor;
char* ip;
char* puerto;
int socketMemoria;
char* ipMemoria;
char* puertoMemoria;

t_algoritmo_planificacion algoritmoPlanificacion;
float alfa;
float estimacionInicial;
int gradoMultiprogramacion;
int gradoMultiprocesamiento;
int intervaloDeadlock;

pthread_t hiloNewReady;
t_list* listaHilosDeDispositivos;
pthread_t hiloReady_Exec;
pthread_t hiloMedianoPlazo;
pthread_t hiloQueDesuspende;
pthread_t hiloDeadlock;

// Estructuras para administrar los semaforos que utilizan los carpinchos

t_list* listaSemaforos;

// Estructuras para administrar los dispositivos de Entrada/Salida que son del kernel

t_list* listaIO;

// Estructuras para administrar la informacion de memoria que mandan los carpinchos

// Semaforos para la ejecucion del kernel

pthread_mutex_t mutexListaSemaforos;

void inicializar_configuracion();
void inicializar_listas();
void inicializar_semaforos();
void inicializar_planificacion();

bool generar_conexiones();
int server_escuchar(t_log* logger, char* nombreServidor, int socketServidor);

void abortar_planificacion();
void destruir_semaforos();
void destruir_listas();

void liberarListaDeSemaforos();
void liberarListaIO();

#endif /* KERNEL_INCLUDE_KERNEL_H_ */
