/*
 * utilidades.h
 *
 *  Created on: 3 oct. 2021
 *      Author: utnso
 */

#ifndef STATIC_INCLUDE_UTILS_UTILIDADES_H_
#define STATIC_INCLUDE_UTILS_UTILIDADES_H_

#include<stdlib.h>
#include<signal.h>
#include<unistd.h>
#include<sys/socket.h>
#include<netdb.h>
#include<string.h>
#include<semaphore.h>
#include<commons/log.h>
#include<commons/collections/queue.h>
#include<stdbool.h>

int cantidadDeElementosEnArray(char** array);
void freeDeArray(char** array);
float diferencia_de_tiempo(float tiempoInicial, float tiempoFinal);
void destruirListaYElementos(t_list* unaLista);
void destruirColaYElementos(t_queue* unaCola);
//int factorial(int n);
//int** permutaciones(int cantidadDeCarpinchos);

#endif /* STATIC_INCLUDE_UTILS_UTILIDADES_H_ */
