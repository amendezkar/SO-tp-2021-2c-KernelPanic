/*
 * utilidades.c
 *
 *  Created on: 3 oct. 2021
 *      Author: utnso
 */

#include "../../include/utils/utilidades.h"

int cantidadDeElementosEnArray(char** array){
    int i = 0;
    while(array[i] != NULL){
        i++;
    }
    return i;
}

void freeDeArray(char** array){
    int cantidadElementosArray = cantidadDeElementosEnArray(array);

    int i;

    for (i = cantidadElementosArray; i>= 0; i--){
        free(array[i]);
    }

    free(array);
}

float diferencia_de_tiempo(float tiempoInicial, float tiempoFinal){

	return tiempoFinal - tiempoInicial;
}

void destruirListaYElementos(t_list* unaLista){
    list_clean_and_destroy_elements(unaLista, free);
    list_destroy(unaLista);
}

void destruirColaYElementos(t_queue* unaCola){
    queue_clean_and_destroy_elements(unaCola, free);
    queue_destroy(unaCola);
}
