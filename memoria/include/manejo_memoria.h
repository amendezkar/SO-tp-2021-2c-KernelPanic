/*
 * manejo_memoria.h
 *
 *  Created on: 17 sep. 2021
 *      Author: utnso
 */

#ifndef MEMORIA_INCLUDE_MANEJO_MEMORIA_H_
#define MEMORIA_INCLUDE_MANEJO_MEMORIA_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <commons/log.h>
#include <commons/collections/list.h>
#include <commons/config.h>
#include <commons/bitarray.h>
#include <stdbool.h>
#include <math.h>
#include "utils/log_config.h"
#include "utils/comunicacion.h"
#include "utils/sockets.h"
#include "utils/mensajes_mem_swamp.h"

void* memoriaPrincipal;
//uint32_t memoriaDisponible;

//ESTRUCTURAS DE LA MEMORIA

typedef struct {
    uint32_t pid;
    t_list* paginas;
    t_list* heapsPartidos; // tipo: <t_info_pagina>
} tpCarpincho;

typedef struct {
    int indice;
    int frame;
    int bitPresencia;
  //  int bytesDisponibles;
    int tiempo_uso;
    int bitDeUso;
    int bitDeModificado;
} t_info_pagina;


typedef struct {
	uint32_t prevAlloc;
	uint32_t nextAlloc;
	uint8_t isFree;
}__attribute__((packed))
HeapMetadata;

typedef struct{
	uint32_t direcLogicaHeap; //principio
	HeapMetadata* heapPartido;
}HeapMetadataPartido;

typedef struct {
    uint32_t pid;
    int numPagina;
	int frame;
	int tiempo_uso;
} TLB;

typedef struct{
	uint32_t pid;
	int cantHits;
	int cantMiss;
} HitMiss;

//void* memoriaPrincipal;
t_bitarray* framesOcupados;
int cantidadFramesTotal;
t_list* listaTablasCarpinchos;
t_list* listaHeapMetadata_t;
t_list* listaTLB;
t_list* listaHitMiss;
uint32_t staticPid;
uint32_t staticNumFrame;
size_t sizeStatic;
uint32_t TURGlobal;
bool nuevoHeapStatic;
int punteroClock;
int tiempo;
int tiempoTLB;
int cantTLBhits;
int cantTLBmiss;
pthread_mutex_t mutexMemoria;

typedef struct {
    	char* ip;
    	char* ipSwamp;
    	char* puerto;
    	char* puertoSwamp;
    	uint32_t tamanio;
    	int tamanioPagina;
    	uint32_t cantPaginas;
    	bool esFija;
    	uint32_t marcosMaximos;
    	bool esClock;
    	uint32_t cantEntradasTLB;
    	bool esFifo;
    	uint32_t retardoAciertoTLB;
    	uint32_t retardoFalloTLB;
    	char* pathDump;
    }t_cfg;



extern t_config* config;
extern t_cfg* cfg;
extern t_log* logger;
extern int socketSwamp;
extern int socketMemoria;

pthread_mutex_t mutexTiempo;
pthread_mutex_t mutexTiempoTLB;
pthread_mutex_t mutexEscribirMemoria;

uint32_t reservar_memoria(uint32_t pid, int size);
tpCarpincho* obtener_tpCarpincho_con_pid(uint32_t pid);
void crear_nueva_tpCarpincho(uint32_t pid, tpCarpincho* tpCarpincho);
uint32_t direccion_reservada(uint32_t pid, int size);
bool pagina_tiene_numFrame(void* x);
bool pagina_tiene_bytes_disponibles(void* x);
bool heap_tiene_next_null(void* x);
void* algoritmo_first_fit_v2(uint32_t pid, int size, int* offset, bool* entraHeapFree);
bool size_entra_en_heap_libre(void* heap);
bool suspender_proceso(uint32_t pid);
void* mayor_inicio_heapMetadata(void* heapMetadata_t1, void* heapMetadata_t2);
uint32_t calcular_dir_logica(void* dirFisica);
int num_pagina_con_num_frame(int numFrame);
tpCarpincho*  tpCarpincho_con_num_frame(int numFrame);
bool tpCarpincho_tiene_num_frame(void* x);
void* leer_memoria_pag(int frame);
bool get_pag_swamp(int socket, int numPagina, uint32_t pid);
bool send_reserva_pagina_swamp(int socketCliente, uint32_t pid, int numPagina);
bool send_remover_pag(int socketCliente, uint32_t pid,int indice);
bool send_swap_out(int socketCliente, int numero, uint32_t pid);
bool send_cantidad_reserva_swamp(int socketCliente,uint32_t pid, int cantidadFramesNecesarios);
bool send_finalizar_proceso_swamp(int socketCliente,uint32_t pid);
void* serializar_pagina(uint32_t pid, int indice,void* contenidoPagina,size_t* sizeStream);
void* serializar_get_pag(int indice,uint32_t pid,size_t* sizeStream);
bool recv_pag_swamp(int socket,t_info_pagina** t_info_pagina,uint32_t* pid,void** contenidoPag);
void* recibir_pagina(int socketCliente, bool* recibido);
t_info_pagina* pagina_a_reemplazar(uint32_t pid);
t_list* buscarInfosPaginasEnRam();
t_list* buscarInfosPaginasCarpinchoEnRam(uint32_t pid);
int cantidad_frames_usados_carpincho(tpCarpincho* tpCarpincho);
t_info_pagina* t_info_pagina_con_num_frame(int numFrame);
bool ejecutar_reemplazo(void* pagina, t_info_pagina* info_pagina, int pid);
int obtener_tiempo();
void sobreescribir_con_ceros(int frame, int desplInicialDentroPagina, int bytesAEscribir);
void sobreescribir_pagina(int frame, void* buffer);
uint32_t buscar_frame_disponible();
int cantidad_paginas(int size);
bool valor_frame(int frame);
void ocupar_frame(int frame);
void vaciar_frame(int frame);
int cantidad_frames_libres();
int nuevo_indice_pagina(uint32_t pid);
void* calcular_direccion_fisica_siguiente_heap(uint32_t pid,uint32_t direcLogica);
bool hay_heap_partido(int indice, uint32_t direcLogica);
HeapMetadata* ultimo_heapMetadata_carpincho(uint32_t pid,bool* ultimoHeapPartido,int* restante, uint32_t* direcLogicaUltimoHeap);
void* calcular_direccion_fisica(uint32_t pid,uint32_t direcLogica);
t_info_pagina* t_info_pagina_con_numero_pagina(int numeroPagina,tpCarpincho* tpCarpincho);
int bytes_diponibles_ultima_pag(tpCarpincho* tpCarpincho);
void* leer_memoria(uint32_t direcLogica ,int size, uint32_t pid);
bool escribir_memoria(uint32_t direcLogica,uint32_t pid, int size, void* contenido);
bool liberar_memoria(uint32_t direcLogica,uint32_t pid);
void remover_pagina_con_indice(t_list* paginas, int indice);
void* heap_correspondiente(uint32_t direcLogica,uint32_t pid, bool* partido, int* resto);
bool traer_pag_correspondiente(int numPag,uint32_t pid);
HeapMetadata* primer_heapMetadata_carpincho(uint32_t pid);
HeapMetadata* heap_correspondiente_a_DL(uint32_t pid, uint32_t direcLogica,bool* heapPartido,int* restante);
HeapMetadata* heap_apto_free(uint32_t direcLogica, uint32_t pid,bool* heapPartido,int* restante, uint32_t* direcLogicaHeap);
void crear_heap_partido(tpCarpincho* tpCarpincho, uint32_t direcLogicaUltimoHeap, t_info_pagina* pagina, int sizeHastaFinalPagina, void* posicionPrincipioHeapPartido1 ,void* posicionPrincipioHeapPartido2, int numFrameParaHeap);
void crear_heap_partido2(tpCarpincho* tpCarpincho, uint32_t direcLogicaUltimoHeap, t_info_pagina* pagina, uint32_t direcLogicaHeapPartido, int numFrameParaHeap);
bool send_pag_swamp(int socket,t_info_pagina* t_info_pagina, uint32_t pid);
//------------------------------------------------------------
int obtener_tiempo_TLB();
int buscar_frame_tlb(int numeroPagina, uint32_t pid);
void actualizarTLB(uint32_t pid,int numeroPagina, int frame);
TLB* entrada_a_remover();
void sumar_TLBhit_a_pid(uint32_t pid);
void sumar_TLBmiss_a_pid(uint32_t pid);
void crear_nueva_HitMiss(uint32_t pid);
HitMiss* buscar_pid_listaHitMiss(uint32_t pid);
void limpiar_TLB();
void imprimir_metricas();
bool finalizar_proceso(uint32_t pid, bool* existeCarpincho);
bool dump_tlb(char* carpeta, t_list* listaTLB);
int liberar_frame_para_carpincho(uint32_t pid);
void eliminar_entradas_tlb_pid(uint32_t pid, int numeroPagina);
void imprimir_estado_frames();
uint32_t min(uint32_t a, uint32_t b);

#endif /* MEMORIA_INCLUDE_MANEJO_MEMORIA_H_ */
