/*
 * init_memoria.c
 *
 *  Created on: 16 sep. 2021
 *      Author: utnso
 */
#include "init_memoria.h"

bool init_memoria(){
	memoriaPrincipal = malloc(cfg->tamanio);
	if (memoriaPrincipal == NULL) {
		log_error(logger, "Error en el malloc de la memoriaPrincipal");
		return false;
	}
	//memset(memoria_principal, 0, cfg->TAMANIO_MEMORIA); por ahora no nos piden esto
	//memoriaDisponible = cfg->tamanio;

	tiempo = 0;
	tiempoTLB = 0;
	listaTablasCarpinchos = list_create();
	listaTLB = list_create();
	listaHitMiss = list_create();
	cantidadFramesTotal = cfg->tamanio / cfg->tamanioPagina;
	printf("RAM FRAMES: %d \n", cantidadFramesTotal);
	char* data = asignar_bytes(cantidadFramesTotal);
	framesOcupados = bitarray_create_with_mode(data, cantidadFramesTotal/8, MSB_FIRST);
	pthread_mutex_init(&mutexMemoria, NULL);

	 MEM_SWAP_MESSAGE tipoAsignacion;

	if(cfg->esFija){
		tipoAsignacion = FIXED_ASIGN;

	}else{
		tipoAsignacion = GLOBAL_ASIGN;

	}

	send(socketSwamp, &tipoAsignacion, sizeof(MEM_SWAP_MESSAGE), 0);
	return true;
}

char* asignar_bytes(int cant_frames) {
    char* buf;
    int bytes;
    if(cant_frames < 8)
        bytes = 1;
    else
    {
        double c = (double) cant_frames;
        bytes = (int) ceil(c/8.0);
    }
    //log_info(logMemoria,"BYTES: %d\n", bytes);
    buf = malloc(bytes);
    memset(buf,0,bytes);
    return buf;
}

void finalizar_memoria(){
	free(memoriaPrincipal);
	free(cfg);
    list_destroy_and_destroy_elements(listaTablasCarpinchos,(void*) destructor_tpCarpinchos);
    list_destroy_and_destroy_elements(listaTLB,free);
    list_destroy_and_destroy_elements(listaHitMiss,free);
    bitarray_destroy(framesOcupados);
	pthread_mutex_destroy(&mutexMemoria);

}

void destructor_tpCarpinchos(tpCarpincho* tablaCarpincho){
	list_destroy_and_destroy_elements(tablaCarpincho->paginas,free);
}



