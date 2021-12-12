/*
 * planificacion.h
 *
 *  Created on: 21 sep. 2021
 *      Author: utnso
 */

#ifndef KERNEL_INCLUDE_PLANIFICACION_H_
#define KERNEL_INCLUDE_PLANIFICACION_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commons/log.h>
#include <commons/config.h>
#include <pthread.h>
#include "utils/log_config.h"
#include <commons/collections/queue.h>
#include <semaphore.h>
#include <time.h>
#include "utils/utilidades.h"
#include "comunicacion_kernel.h"

typedef struct{
	uint32_t carpinchoPID;
	float rafagaAnterior; //Era clock_t
	float estimacionActual; //Era int
	float estimacionAnterior; //Era int
	float horaDeIngresoAExe; //Era clock_t
	float horaDeIngresoAReady; //Era clock_t
	float tiempoEspera; //Era clock_t
	bool suspendido;
	t_list* semaforosRetenidos;
	t_semaforo* semaforoEsperado;
	//
	int socketCarpincho;
	int socketMemoria;
	t_log* logger;
	char* nombreServidor;
}pcb_carpincho;

typedef enum{
	SJF,
	HRRN
}t_algoritmo_planificacion;

typedef struct{
	pcb_carpincho* pcb;
	char* mensaje;
} informacionDeIO;

extern t_log* logger;
extern t_config* config;
extern float alfa;
extern float estimacionInicial;
extern t_algoritmo_planificacion algoritmoPlanificacion;
extern int gradoMultiprogramacion;
extern int intervaloDeadlock;

// LISTAS/COLAS
t_queue* colaNew;
t_list* colaReady;
t_list* listaExe;
t_list* listaBlock;
t_list* listaExit;
t_list* listaBlockSuspended;
t_queue* colaReadySuspended;
t_list* listaPotencialesRetensores;

// SEMAFOROS - Colas
pthread_mutex_t mutexNew;
pthread_mutex_t mutexReady;
pthread_mutex_t mutexBlock;
pthread_mutex_t mutexExe;
pthread_mutex_t mutexExit;
pthread_mutex_t mutexBlockSuspended;
pthread_mutex_t mutexReadySuspended;
pthread_mutex_t mutexPotencialesRetensores;

sem_t contadorNew;
sem_t contadorReady;
sem_t contadorExe;
sem_t contadorProcesosEnMemoria;
sem_t multiprogramacion;
sem_t multiprocesamiento;
sem_t contadorBlock;
sem_t analizarSuspension;
sem_t suspensionFinalizada;
sem_t largoPlazo;
sem_t contadorReadySuspended;
sem_t medianoPlazo;

extern t_list* listaSemaforos;

extern t_list* listaIO;

extern pthread_mutex_t mutexListaSemaforos;

void agregarAPotencialesRetensores(pcb_carpincho* pcb);
void sacarDePotencialesRetensores(pcb_carpincho* pcb);

void agregarANew(pcb_carpincho* carpincho);
pcb_carpincho* sacarDeNew();
void agregarAReady(pcb_carpincho* carpincho);
void agregarABlock(pcb_carpincho* carpincho);
void sacarDeBlock(pcb_carpincho* carpincho);
void agregarABlockSuspended(pcb_carpincho* carpincho);
void sacarDeBlockSuspended(pcb_carpincho* carpincho);
void agregarAReadySuspended(pcb_carpincho* carpincho);
pcb_carpincho* sacarDeReadySuspended();

void hiloNew_Ready();
void hiloReady_Exe();
void hiloBlockASuspension();
void hiloSuspensionAReady();
void deteccionYRecuperacion();
void analizarDeadlock(t_list* carpinchosEnDeadlock);
pcb_carpincho* casoRecursivo(t_list* carpinchosEnDeadlock, pcb_carpincho* potencialRetensor, pcb_carpincho* otroPCB);
//bool hayDeadlock();
//t_list* carpinchosEnColasDeSemaforos();

bool condiciones_de_suspension();

void ejecutar(void* carpincho);

bool recv_call_io(pcb_carpincho* pcb, mate_io_resource* io, char** msg);
void funcionDispositivo(void* dispositivo);

t_algoritmo_planificacion obtener_algoritmo();
pcb_carpincho* obtenerSiguienteDeReady();
pcb_carpincho* obtenerSiguienteSJF();
pcb_carpincho* obtenerSiguienteHRRN();
void actualizarTiemposDeEspera();

void realizar_operacion_sem(pcb_carpincho* pcb, sem_op tipoOP, t_semaforo* semaforo);
void operacion_semaforo_creado(pcb_carpincho* pcb, mate_sem_name nombreSemaforo, sem_op tipoOP);

bool recv_sem_init(pcb_carpincho* pcb);
bool recv_sem_wait(pcb_carpincho* pcb, mate_sem_name* nombreSemaforo);
bool recv_sem_post(pcb_carpincho* pcb, mate_sem_name* nombreSemaforo);
bool recv_sem_destroy(pcb_carpincho* pcb, mate_sem_name* nombreSemaforo);

void terminarEjecucion(pcb_carpincho* pcb);
void liberarSemaforosRetenidos(pcb_carpincho* pcb);

#endif /* KERNEL_INCLUDE_PLANIFICACION_H_ */
