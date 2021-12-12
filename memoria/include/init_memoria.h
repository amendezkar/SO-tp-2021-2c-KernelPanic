/*
 * init_memoria.h
 *
 *  Created on: 16 sep. 2021
 *      Author: utnso
 */

#ifndef MEMORIA_INCLUDE_INIT_MEMORIA_H_
#define MEMORIA_INCLUDE_INIT_MEMORIA_H_


#include "comunicacion_memoria.h"


bool init_memoria();
char* asignar_bytes(int cant_frames);
void finalizar_memoria();
void destructor_tpCarpinchos(tpCarpincho* tablaCarpincho);

#endif /* MEMORIA_INCLUDE_INIT_MEMORIA_H_ */
