/*
 * manejo_memoria.c
 *
 *  Created on: 17 sep. 2021
 *      Author: utnso
 */

#include "manejo_memoria.h"

uint32_t reservar_memoria(uint32_t pid, int size){



	tpCarpincho* tpCarpincho = obtener_tpCarpincho_con_pid(pid);


	if(tpCarpincho == NULL){
		//es un carpincho nuevo

		tpCarpincho = malloc(sizeof(*tpCarpincho));
		crear_nueva_tpCarpincho(pid,tpCarpincho);

		//semaforos
		list_add(listaTablasCarpinchos,(void*) tpCarpincho);
	}

	uint32_t direccionReservada = direccion_reservada(pid, size);

	return direccionReservada;

}



tpCarpincho* obtener_tpCarpincho_con_pid(uint32_t pid) {


	bool tp_carpincho_tiene_pid(void* x) {

		if(x == NULL){

			return false;
		}else{
		tpCarpincho* elem  = x;
		    return elem->pid == pid;
		}
	}

	//semaforo
	tpCarpincho* elem = list_find(listaTablasCarpinchos, &tp_carpincho_tiene_pid);
    return elem;
}

void crear_nueva_tpCarpincho(uint32_t pid, tpCarpincho* tpCarpincho) {

	tpCarpincho->pid = pid;
	tpCarpincho->paginas = list_create();
	tpCarpincho->heapsPartidos = list_create();

}

uint32_t direccion_reservada(uint32_t pid, int size) {

	uint32_t offset;
	int offsetHeap;
	int restoHeapFree;
	int numFrameDisponible;
	bool entraEnHeapFree = false;
	t_info_pagina* pagina = algoritmo_first_fit_v2(pid, size, &offsetHeap, &entraEnHeapFree);
	int resto;
	bool ultimoHeapPartido = false;
	uint32_t direcLogicaUltimoHeap = 0;
	HeapMetadata* ultimoHeap = ultimo_heapMetadata_carpincho(pid,&ultimoHeapPartido,&resto,&direcLogicaUltimoHeap);
	tpCarpincho* tpCarpincho = obtener_tpCarpincho_con_pid(pid);
	HeapMetadata* heapFree;

	bool ultimoEstabaFree = false;

	bool heapFreePartido = false;

	int cantPaginas = cantidad_paginas(size);

	if(ultimoHeap != NULL){

		ultimoEstabaFree = ultimoHeap->isFree;

	}


	//-----------------------------------------ENTRA EN HEAPFREE-------------------------------------
	if(entraEnHeapFree){
		uint32_t direcLogicaHeapHueco = 0;

		heapFree = heap_correspondiente(pagina->indice*cfg->tamanioPagina + offsetHeap +sizeof(HeapMetadata), pid, &heapFreePartido, &restoHeapFree);
		if(heapFree == NULL){
			return (uint32_t) NULL;
		}
		heapFree->isFree = false;

		bool heapSiguientePartido = false;
		int restoHeapSiguientePartido = 0;

		HeapMetadata* heapSiguiente = heap_correspondiente(heapFree->nextAlloc + sizeof(HeapMetadata), pid,&heapSiguientePartido, &restoHeapSiguientePartido);
		if(heapSiguiente == NULL){
			return (uint32_t) NULL;
		}

		direcLogicaHeapHueco = heapSiguiente->prevAlloc + sizeof(HeapMetadata) + size;

		HeapMetadata* heapMetadataHueco = malloc(sizeof(HeapMetadata));
		heapMetadataHueco->nextAlloc = heapFree->nextAlloc;
		heapMetadataHueco->isFree = true;

		heapMetadataHueco->prevAlloc = heapSiguiente->prevAlloc;
		heapFree->nextAlloc = direcLogicaHeapHueco;
		heapSiguiente->prevAlloc = direcLogicaHeapHueco;

		if(heapFreePartido){
			void* direcFisicaActual = calcular_direccion_fisica(pid,heapMetadataHueco->prevAlloc);
			memcpy(direcFisicaActual, heapFree, sizeof(HeapMetadata)-restoHeapFree);
			memcpy(calcular_direccion_fisica(pid, heapMetadataHueco->prevAlloc +(sizeof(HeapMetadata)-restoHeapFree)),heapFree + (sizeof(HeapMetadata)-restoHeapFree),restoHeapFree);
			free(heapFree);
		}

		uint32_t direcARetornar = heapMetadataHueco->prevAlloc + sizeof(HeapMetadata);
		if(heapSiguientePartido){
			void* direcFisicaActual = calcular_direccion_fisica(pid,heapMetadataHueco->nextAlloc);
			memcpy(direcFisicaActual, heapSiguiente, sizeof(HeapMetadata)-restoHeapSiguientePartido);
			memcpy(calcular_direccion_fisica(pid, heapMetadataHueco->nextAlloc +(sizeof(HeapMetadata)-restoHeapSiguientePartido)),heapSiguiente + (sizeof(HeapMetadata)-restoHeapSiguientePartido),restoHeapSiguientePartido);
			free(heapSiguiente);

		}

		uint32_t direcLogicaFinalHeapHueco = direcLogicaHeapHueco + sizeof(HeapMetadata);

		int numPagHeapHueco = direcLogicaHeapHueco/cfg->tamanioPagina;
		int numPagFinalHeapHueco = direcLogicaFinalHeapHueco/cfg->tamanioPagina;

		t_info_pagina* paginaHeapHueco = t_info_pagina_con_numero_pagina(numPagHeapHueco, tpCarpincho);

		//vemos que el la paginaHeapHueco este en memoria
		if(!paginaHeapHueco->bitPresencia){
			get_pag_swamp(socketSwamp,numPagHeapHueco, pid);
			MEM_SWAP_MESSAGE respuestaSwamp;
			recv(socketSwamp,&respuestaSwamp, sizeof(MEM_SWAP_MESSAGE), 0);
			if(respuestaSwamp == PAGE_READ){
				bool recibido;
				void* contenidoPagina = recibir_pagina(socketSwamp, &recibido);
				if(!recibido){
					return (uint32_t) NULL;
				}
				ejecutar_reemplazo(contenidoPagina, paginaHeapHueco, pid);
			}else{

				return (uint32_t) NULL; //swamp no nos puede dar la pagina donde se crearia el heapHueco
			}

		}

		void* direcFisicaHeapHueco = calcular_direccion_fisica(pid, direcLogicaHeapHueco);

		t_info_pagina* paginaHeapHuecoSegundaMitad;

		if((direcLogicaFinalHeapHueco)%cfg->tamanioPagina < sizeof(HeapMetadata)){
			//aca entra cuando el heapMetadataHueco esta partido
			//traer las paginas donde estarian direcFisicaHeapHueco y direcFisicaSegundaMitad
			paginaHeapHuecoSegundaMitad = t_info_pagina_con_numero_pagina(numPagFinalHeapHueco, tpCarpincho);

			//vemos que el la paginaHeapHueco este en memoria
			if(!paginaHeapHuecoSegundaMitad->bitPresencia){
				get_pag_swamp(socketSwamp,paginaHeapHuecoSegundaMitad->indice, pid);
				MEM_SWAP_MESSAGE respuestaSwamp;
				recv(socketSwamp,&respuestaSwamp, sizeof(MEM_SWAP_MESSAGE), 0);
				if(respuestaSwamp == PAGE_READ){
					bool recibido;
					void* contenidoPagina = recibir_pagina(socketSwamp, &recibido);
					if(!recibido){
						return (uint32_t) NULL;
					}
					ejecutar_reemplazo(contenidoPagina, paginaHeapHueco, pid);
				}else{

					return (uint32_t) NULL; //swamp no nos puede dar la pagina donde se crearia el heapHueco
				}
			}

			uint32_t resto = (int) (direcLogicaFinalHeapHueco)%cfg->tamanioPagina;
			memcpy(direcFisicaHeapHueco, heapMetadataHueco, sizeof(HeapMetadata)- resto);
			void* direcFisicaSegundaMitad = calcular_direccion_fisica(pid,direcLogicaFinalHeapHueco - resto);
			memcpy(direcFisicaSegundaMitad, heapMetadataHueco+(sizeof(HeapMetadata)- resto),resto);

		}else{

			memcpy(direcFisicaHeapHueco,heapMetadataHueco,sizeof(HeapMetadata));

		}

		free(heapMetadataHueco);
		return direcARetornar;
	}


	uint32_t direcLogicaFinalUltimoHeap = direcLogicaUltimoHeap + sizeof(HeapMetadata);
	int sizeHastaFinalPagina = cfg->tamanioPagina - (direcLogicaFinalUltimoHeap%cfg->tamanioPagina);

	if(ultimoHeap != NULL){
		if(!ultimoHeap->isFree || sizeHastaFinalPagina == cfg->tamanioPagina){
			sizeHastaFinalPagina = 0;
		}
	}


	int cantidadFramesUsadosCarpincho = cantidad_frames_usados_carpincho(tpCarpincho);

	int memoriaDisponibleParaProceso;
	if(cfg->esFija){
		if(direcLogicaUltimoHeap == 0){
			memoriaDisponibleParaProceso = (cfg->marcosMaximos - cantidadFramesUsadosCarpincho)*cfg->tamanioPagina;

		}else{
			memoriaDisponibleParaProceso = (cfg->marcosMaximos - cantidadFramesUsadosCarpincho)*cfg->tamanioPagina + sizeHastaFinalPagina;
		}
	}else{
		memoriaDisponibleParaProceso = cantidad_frames_libres()*cfg->tamanioPagina + sizeHastaFinalPagina;

	}
	int sizeRestante = size - sizeHastaFinalPagina;

	int sizeRestante2 = sizeRestante + sizeof(HeapMetadata);
	int cantidadFramesNecesarios;

	if(cantidadFramesUsadosCarpincho == 0){
		cantidadFramesNecesarios = cantidad_paginas(size + 2*sizeof(HeapMetadata));
	}else{
		cantidadFramesNecesarios = cantidad_paginas(sizeRestante2);
	}

	if(!send_cantidad_reserva_swamp(socketSwamp, pid, cantidadFramesNecesarios)){
		if(ultimoHeapPartido){
			free(ultimoHeap);
		}
		return (uint32_t) NULL;
	}

	MEM_SWAP_MESSAGE respuestaCantidadReserva;

	recv(socketSwamp, &respuestaCantidadReserva, sizeof(MEM_SWAP_MESSAGE), 0);

	if(respuestaCantidadReserva == ERROR_NO_SPACE_IN_PROCESS_FILE){
		printf("No hay espacio en swamp para reservar memoria del pid: %d \n", pid);

		if(ultimoHeapPartido){
			free(ultimoHeap);
		}
		return (uint32_t) NULL;
	}


	//-----------------------------------------NO HAY ESPACIO-------------------------------------
/* COMENTADO PORQUE HAY QUE IR REEMPLAZANDO PAGINAS A MEDIDA QUE LAS VAMOS CREANDO Y NO TENGAMOS FRAME DISPONIBLE
	if(memoriaDisponibleParaProceso < size){
		//algoritmo de reemplazo
		//hay q ver cuantas pags son ese size (redondear para arriba)
		//hay q ver cuantas de esas requeridas estan libres
		//y hacer un for(cantidadOcupadas) que adentro haga ejecutar_reemplazo()



		//hay que ver cuanto podria meter en la ultima pagina del proceso
		t_info_pagina* paginasAReemplazar[cantidadFramesTotal];
		int contador = 0;
		while(memoriaDisponibleParaProceso < size){
			t_info_pagina* paginaAReemplazar = pagina_a_reemplazar(pid);

			paginaAReemplazar->bitPresencia = 0;
			vaciar_frame(paginaAReemplazar->frame);
			paginasAReemplazar[contador] = paginaAReemplazar;
			contador++;

			memoriaDisponibleParaProceso += cfg->tamanioPagina;
		}

		//preguntar a swamp si puede recibir esa cantidad de paginas

		for(int i=0; i < contador; i++){
			send_pag_swamp(socketSwamp,paginasAReemplazar[i], pid);

			MEM_SWAP_MESSAGE respuesta;
			recv(socketSwamp, &respuesta, sizeof(MEM_SWAP_MESSAGE), 0);

			if(respuesta != PAGE_ADDED){
				if(ultimoHeapPartido){
					free(ultimoHeap);
				}
				return (uint32_t) NULL;
			}

		}

	}
*/
	//-----------------------------------------HAY ESPACIO Y NO ENTRA EN FREE-------------------------------------


	void* direcFisicaFinalUltimoHeap;
	void* direcFisicaInicioUltimoHeap = calcular_direccion_fisica(pid, direcLogicaUltimoHeap);

	//aca ya deberia haber lugar para hacer las cositas

	int cantidadPaginasCarpincho = list_size(tpCarpincho->paginas);

	bool casoParticularHuequito = false;

	if(pagina == NULL && cantidadPaginasCarpincho > 0){//no tiene bytes disponibles

		if(!ultimoHeap->isFree){
			numFrameDisponible = buscar_frame_disponible();
			if(numFrameDisponible == -1 || ((cantidadFramesUsadosCarpincho == cfg->marcosMaximos) && cfg->esFija)){
				numFrameDisponible = liberar_frame_para_carpincho(pid);
			}
			pagina = list_get(tpCarpincho->paginas, (int)(cantidadPaginasCarpincho - 1));
			pagina->bitDeModificado=1;
			pagina->tiempo_uso = obtener_tiempo();

			t_info_pagina* nuevaPagina = malloc(sizeof(t_info_pagina));
			nuevaPagina->bitPresencia = 1;
			nuevaPagina->frame = numFrameDisponible;
			nuevaPagina->indice = nuevo_indice_pagina(pid);
			nuevaPagina->tiempo_uso = obtener_tiempo();
			nuevaPagina->bitDeUso = 1;
			nuevaPagina->bitDeModificado = 0;

			//semaforo???
			list_add(tpCarpincho->paginas,nuevaPagina);


			if(!send_reserva_pagina_swamp(socketSwamp,pid,nuevaPagina->indice)){
				return NULL;
			}

			pagina = nuevaPagina;

			numFrameDisponible = pagina->frame;

			//aca crear el heap metadata nuevo
			HeapMetadata* heapMetadataNuevo = memoriaPrincipal + numFrameDisponible * cfg->tamanioPagina;
			heapMetadataNuevo->prevAlloc = direcLogicaUltimoHeap;
			heapMetadataNuevo->isFree = false;

			direcLogicaUltimoHeap = pagina->indice*cfg->tamanioPagina;
			direcLogicaFinalUltimoHeap = direcLogicaUltimoHeap + sizeof(HeapMetadata);

			if(sizeRestante == 0 || (((cfg->tamanioPagina - sizeof(HeapMetadata) - size) == 0) && (!ultimoEstabaFree))){
				heapMetadataNuevo->nextAlloc = (uint32_t) NULL;

			}else{
				heapMetadataNuevo->nextAlloc = direcLogicaFinalUltimoHeap + size;
			}
			ocupar_frame(numFrameDisponible);

			//actualizar heapAnterior
			ultimoHeap->nextAlloc = direcLogicaUltimoHeap;
			ultimoHeap->isFree = false;
			HeapMetadata* ultimoHeapAux = malloc(sizeof(HeapMetadata));
			memcpy(ultimoHeapAux,ultimoHeap,sizeof(HeapMetadata));
			void* posicionUltimoHeap = ultimoHeap;
			ultimoHeap = heapMetadataNuevo;
			memcpy(posicionUltimoHeap,ultimoHeapAux,sizeof(HeapMetadata));
			free(ultimoHeapAux);

			casoParticularHuequito = true;

		}else{
			pagina = list_get(tpCarpincho->paginas, (int)(cantidadPaginasCarpincho - 1));
			pagina->tiempo_uso = obtener_tiempo();
			offset = cfg->tamanioPagina - sizeHastaFinalPagina;
			numFrameDisponible = pagina->frame;
			ultimoHeap->nextAlloc = direcLogicaFinalUltimoHeap + size;
			ultimoHeap->isFree = false;
		}


		direcFisicaFinalUltimoHeap = calcular_direccion_fisica(pid, direcLogicaFinalUltimoHeap);

	}else if(pagina == NULL && cantidadPaginasCarpincho == 0){ //es la primera pagina del proceso

		numFrameDisponible = buscar_frame_disponible();
		if(numFrameDisponible == -1 || ((cantidadFramesUsadosCarpincho == cfg->marcosMaximos) && cfg->esFija)){
			numFrameDisponible = liberar_frame_para_carpincho(pid);
		}
		offset = sizeof(HeapMetadata);

		pagina = malloc(sizeof(t_info_pagina));
		pagina->bitPresencia = 1;
		pagina->frame = numFrameDisponible;
		pagina->indice = nuevo_indice_pagina(pid);
		pagina->tiempo_uso = obtener_tiempo();
		pagina->bitDeUso = 1;
		pagina->bitDeModificado = 0;

		//semaforo???
		list_add(tpCarpincho->paginas,pagina);


		if(!send_reserva_pagina_swamp(socketSwamp,pid,pagina->indice)){
			return NULL;
		}


		//aca crear el heap metadata nuevo
		HeapMetadata* heapMetadataNuevo = memoriaPrincipal + numFrameDisponible * cfg->tamanioPagina;
		heapMetadataNuevo->prevAlloc = (uint32_t) NULL;
		heapMetadataNuevo->nextAlloc = (uint32_t) NULL;
		heapMetadataNuevo->isFree = true;

		ocupar_frame(numFrameDisponible);

		ultimoHeap = heapMetadataNuevo;

		if((direcLogicaUltimoHeap + sizeof(HeapMetadata) + size) != cfg->tamanioPagina){
			ultimoHeap->nextAlloc = direcLogicaUltimoHeap + sizeof(HeapMetadata) + size;
		}
		ultimoHeap->isFree = false;

		direcFisicaInicioUltimoHeap = calcular_direccion_fisica(pid, direcLogicaUltimoHeap);
		direcFisicaFinalUltimoHeap = calcular_direccion_fisica(pid, direcLogicaFinalUltimoHeap);

	} else{

		numFrameDisponible = pagina->frame;
		offset = cfg->tamanioPagina - sizeHastaFinalPagina;

	}

	if(ultimoHeapPartido){
		if(sizeRestante == 0){
			ultimoHeap->isFree = false;
		}
			memcpy(direcFisicaInicioUltimoHeap,ultimoHeap,sizeof(HeapMetadata)-resto);
			memcpy(direcFisicaFinalUltimoHeap - resto,ultimoHeap+(sizeof(HeapMetadata)-resto),resto);

			free(ultimoHeap);
	}

	//void* direccionARetornar = memoriaPrincipal + pagina->frame*cfg->tamanioPagina + offset;
	//uint32_t direccionLogicaARetornar = calcular_dir_logica(direccionARetornar);


	if(sizeRestante == 0 || (((cfg->tamanioPagina - sizeof(HeapMetadata) - size) == 0) && (!ultimoEstabaFree))){ //entra justo en la pagina y no se crea otro heap
		pagina->bitDeModificado = 1;
		pagina->tiempo_uso = obtener_tiempo();
		ultimoHeap->nextAlloc = NULL;
		//no hace nada mas porque no crea otra pagina ni nada


	}else if((cantPaginas == 1 && (sizeRestante < 0)) || casoParticularHuequito){ //entra enterito en el huequito
		pagina->bitDeModificado=1;
		pagina->tiempo_uso = obtener_tiempo();

		if(((sizeHastaFinalPagina - size) >= sizeof(HeapMetadata)) || casoParticularHuequito){ //abarca el caso donde se crea justo al final el heap
			//aca crear el heap metadata nuevo

			HeapMetadata* heapMetadataNuevo;
			if((sizeHastaFinalPagina - size) == sizeof(HeapMetadata)){
				direcFisicaFinalUltimoHeap = direcFisicaInicioUltimoHeap + sizeof(HeapMetadata);
			}
			heapMetadataNuevo = direcFisicaFinalUltimoHeap + size;
			heapMetadataNuevo->nextAlloc = (uint32_t) NULL;
			heapMetadataNuevo->prevAlloc = direcLogicaUltimoHeap;
			heapMetadataNuevo->isFree = true;

			//actualizar heapAnterior
			//ultimoHeap->nextAlloc = direcFisicaFinalUltimoHeap + size;
			//ultimoHeap->isFree = false;


			} else{ //se crea el nuevo heap partido

			sizeHastaFinalPagina -= size;

			int numFrameParaHeap = buscar_frame_disponible();
			if(numFrameParaHeap == -1 || ((cantidadFramesUsadosCarpincho == cfg->marcosMaximos) && cfg->esFija)){
				numFrameParaHeap = liberar_frame_para_carpincho(pid);
			}

			ocupar_frame(numFrameParaHeap);

			void* posicionPrincipioHeapPartido1 = (direcFisicaFinalUltimoHeap + size);

			void* posicionPrincipioHeapPartido2 = (memoriaPrincipal + numFrameParaHeap* cfg->tamanioPagina);

			crear_heap_partido2(tpCarpincho, direcLogicaUltimoHeap, pagina,direcLogicaFinalUltimoHeap + size, numFrameParaHeap);

			//crear_heap_partido(tpCarpincho, direcLogicaUltimoHeap, pagina, sizeHastaFinalPagina,posicionPrincipioHeapPartido1, posicionPrincipioHeapPartido2, numFrameParaHeap);

		}


	}else if(cantPaginas >= 1){ //hacer las iteraciones de metida de paginas
		int iteraciones;
		int sizeIteracion;

		pagina->bitDeModificado = 1;
		pagina->tiempo_uso = obtener_tiempo();
		iteraciones = cantidad_paginas(sizeRestante);
		numFrameDisponible = buscar_frame_disponible();
		if(numFrameDisponible == -1 || ((cantidadFramesUsadosCarpincho == cfg->marcosMaximos) && cfg->esFija)){
			numFrameDisponible = liberar_frame_para_carpincho(pid);
		}
		sizeIteracion = sizeRestante;

		t_info_pagina* paginaNueva = malloc(sizeof(t_info_pagina)); //esta pagina es para empezar las iteraciones
		paginaNueva->bitPresencia = 1;
		paginaNueva->frame = numFrameDisponible;
		paginaNueva->indice = nuevo_indice_pagina(pid);
		paginaNueva->tiempo_uso = obtener_tiempo();
		paginaNueva->bitDeUso = 1;
		paginaNueva->bitDeModificado = 0;

		ocupar_frame(paginaNueva->frame);


		list_add(tpCarpincho->paginas,paginaNueva);


		if(!send_reserva_pagina_swamp(socketSwamp,pid,paginaNueva->indice)){
			return NULL;
		}


		pagina = paginaNueva;

		for(int i = 0; i<iteraciones; i++){

			t_info_pagina* paginaNuevaIteracion;

			if(i>0){
				paginaNuevaIteracion = malloc(sizeof(t_info_pagina));

				paginaNuevaIteracion->bitPresencia = 1;
				paginaNuevaIteracion->frame = numFrameDisponible;
				paginaNuevaIteracion->indice = nuevo_indice_pagina(pid);
				paginaNuevaIteracion->tiempo_uso = obtener_tiempo();
				paginaNuevaIteracion->bitDeUso = 1;
				paginaNuevaIteracion->bitDeModificado = 0;


				list_add(tpCarpincho->paginas,paginaNuevaIteracion);


				if(!send_reserva_pagina_swamp(socketSwamp,pid,paginaNuevaIteracion->indice)){
					return NULL;
				}

			}else{
				paginaNuevaIteracion = pagina;
			}

			if( (double) sizeIteracion/cfg->tamanioPagina <= 1){ //en este caso entra solo la ultima iteracion, por eso creamos el heapNuevo

				//actualizar las paginas, se llena el pedacito restante
				int sizeHastaFinalPaginaIteracion = cfg->tamanioPagina - sizeIteracion;

				//actualizar heapAnterior YA LO HACEMOS ANTES
				//ultimoHeap->nextAlloc = direcLogicaUltimoHeap + sizeof(HeapMetadata) + size;
				//ultimoHeap->isFree = false;



				ocupar_frame(numFrameDisponible);

				if(sizeHastaFinalPaginaIteracion == 0){

					break;
				}

				if(sizeHastaFinalPaginaIteracion < sizeof(HeapMetadata)){//va heap partido


					int numFrameParaHeap = buscar_frame_disponible();
					if(numFrameParaHeap == -1 || ((cantidadFramesUsadosCarpincho == cfg->marcosMaximos) && cfg->esFija)){
						numFrameParaHeap = liberar_frame_para_carpincho(pid);
					}

					void* posicionPrincipioHeapPartido1 = (memoriaPrincipal + paginaNuevaIteracion->frame*cfg->tamanioPagina + sizeIteracion);

					void* posicionPrincipioHeapPartido2 = (memoriaPrincipal + numFrameParaHeap* cfg->tamanioPagina);

					crear_heap_partido(tpCarpincho, direcLogicaUltimoHeap, paginaNuevaIteracion, sizeHastaFinalPaginaIteracion, posicionPrincipioHeapPartido1 , posicionPrincipioHeapPartido2, numFrameParaHeap);


				}else{


					//aca crear el heap metadata nuevo
					HeapMetadata* heapMetadataNuevo = memoriaPrincipal + numFrameDisponible*cfg->tamanioPagina + sizeIteracion;
					heapMetadataNuevo->prevAlloc = direcLogicaUltimoHeap;
					heapMetadataNuevo->nextAlloc = (uint32_t) NULL;
					heapMetadataNuevo->isFree = true;

				}

				paginaNuevaIteracion->bitDeModificado=1;

				break;

			} else{
				sizeIteracion -= cfg->tamanioPagina;

			}

			//paginaNuevaIteracion->bitDeModificado=1;


			ocupar_frame(numFrameDisponible);
			numFrameDisponible = buscar_frame_disponible();

			if(numFrameDisponible == -1 || ((cantidadFramesUsadosCarpincho == cfg->marcosMaximos) && cfg->esFija)){
				numFrameDisponible = liberar_frame_para_carpincho(pid);
			}
		}

	}


	return direcLogicaFinalUltimoHeap;


}

bool pagina_tiene_numFrame(void* x) {
	if(x == NULL){

			return false;
		}else{
	t_info_pagina* elem = x;
    return (elem->frame == staticNumFrame) && (elem->bitPresencia==1);
		}
}

void* algoritmo_first_fit_v2(uint32_t pid, int size, int* offset, bool* entraHeapFree){
	tpCarpincho* tpCarpincho = obtener_tpCarpincho_con_pid(pid);

	bool heapSiguientePartido = false;
	int restoSiguienteHeap = 0;

	HeapMetadata* heap = heap_correspondiente(sizeof(HeapMetadata), pid, &heapSiguientePartido, &restoSiguienteHeap);

	if(heap == NULL){
		return NULL;

	}


	uint32_t direcLogicaUltimoHeap = 0;

	while(heap->nextAlloc != (uint32_t)NULL){
		if(heap->isFree && (size == (heap->nextAlloc - direcLogicaUltimoHeap + sizeof(HeapMetadata)) || (size + sizeof(HeapMetadata)) <= (heap->nextAlloc - direcLogicaUltimoHeap + sizeof(HeapMetadata)))){
			//*offset = calcular_dir_logica(heap)%cfg->tamanioPagina ;
			*offset = restoSiguienteHeap;
			int numPag = direcLogicaUltimoHeap/cfg->tamanioPagina ;
			t_info_pagina* pagina = t_info_pagina_con_numero_pagina(numPag, tpCarpincho);
			*entraHeapFree = true;
			if(heapSiguientePartido){
				free(heap);
			}
			return pagina;
		}

		direcLogicaUltimoHeap = heap->nextAlloc;
		if(heapSiguientePartido){
			free(heap);
		}

		heap = heap_correspondiente(direcLogicaUltimoHeap + sizeof(HeapMetadata), pid,&heapSiguientePartido,&restoSiguienteHeap);

		if(heap == NULL){
			printf("El heap es NULL en el first fit \n");
			return NULL;
		}
	}
	if(heapSiguientePartido){
		free(heap);
	}
	return NULL;
}


uint32_t calcular_dir_logica(void* direcFisica){

	int dirFisica = (int) direcFisica - (int) memoriaPrincipal;

	int numFrame = (int) floor(dirFisica/cfg->tamanioPagina);
	int offset = dirFisica%cfg->tamanioPagina;
	int numPag = num_pagina_con_num_frame(numFrame);

	if(numPag == -1){
		return -1;
	}

	return numPag*cfg->tamanioPagina + offset;
}

int num_pagina_con_num_frame(int numFrame){

	void* tp = tpCarpincho_con_num_frame(numFrame);
	staticNumFrame = numFrame;
	//semaforo

	if(tp == NULL){
		return -1;
	}
	void* tInfoPagina = list_find( ((tpCarpincho*) tp)->paginas, &pagina_tiene_numFrame);
	//semaforo

	if(tInfoPagina == NULL){
		return -1;
	}else{
		return ((t_info_pagina*) tInfoPagina)->indice;
	}


}

tpCarpincho*  tpCarpincho_con_num_frame(int numFrame){

	staticNumFrame = numFrame;

	bool tiene_num_frame(void* elem){
		tpCarpincho* carpincho = elem;

		bool pagina_tiene_num_frame(void* elem){
			t_info_pagina* pagina = elem;
			return (pagina->frame == numFrame) && (pagina->bitPresencia == 1);
		}
		return list_any_satisfy(carpincho->paginas, &pagina_tiene_num_frame);
	}

	tpCarpincho* a = list_find(listaTablasCarpinchos, &tiene_num_frame);

	return a;
}

bool tpCarpincho_tiene_num_frame(void* x){


	tpCarpincho* elem = x;

	//semaforo
	return  list_any_satisfy(elem->paginas, &pagina_tiene_numFrame);
	//semaforo
}

bool suspender_proceso(uint32_t pid){
	tpCarpincho* tpCarpincho = obtener_tpCarpincho_con_pid(pid);

	if(tpCarpincho == NULL){
		return false;
	}

	t_list_iterator* iteradorPaginas = list_iterator_create(tpCarpincho->paginas);



	while(list_iterator_has_next(iteradorPaginas)){
		t_info_pagina* pagActual = list_iterator_next(iteradorPaginas);

		if(pagActual->bitPresencia){
			continue;
		}
		int bitM = pagActual->bitDeModificado;
		if(send_pag_swamp(socketSwamp,pagActual, pid) == true){ // TODO falta recibir la respuesta
			if(bitM != 0){
				MEM_SWAP_MESSAGE respuesta;
				recv(socketSwamp, &respuesta, sizeof(MEM_SWAP_MESSAGE), 0);
				if(respuesta != PAGE_ADDED){
					printf("La pagina %d del proceso %d no se pudo enviar a swamp \n",pagActual->indice,pid);
					return false;
				}
			}

			vaciar_frame(pagActual->frame);
			printf("La pagina %d del proceso %d se envio a swamp \n",pagActual->indice,pid);
		}else{
			printf("La pagina %d del proceso %d no se pudo enviar a swamp \n",pagActual->indice,pid);
			return false;
		};
	}



	return true;
}

bool send_pag_swamp(int socket,t_info_pagina* t_info_pagina, uint32_t pid){

	if(!t_info_pagina->bitDeModificado){
		printf("Pagina no modificada, no se envio \n");
		return true;
	}

	void* contenidoPagina = leer_memoria_pag(t_info_pagina->frame);
	size_t sizeStream;
	void* stream = serializar_pagina(pid, t_info_pagina->indice,contenidoPagina,&sizeStream);

	if (send(socket, stream, sizeStream, 0) == -1) {
			free(stream);
			free(contenidoPagina);
			return false;
	}

	printf("Pagina %d  modificada del pid %d, se envio \n", t_info_pagina->indice, pid);

	t_info_pagina->bitDeModificado = 0;
	free(stream);
	free(contenidoPagina);
	return true;
}



bool get_pag_swamp(int socket, int numPagina, uint32_t pid){

	printf("Pedido de pagina: %d, de pid: %d \n", numPagina, pid);
	size_t sizeStream;
	void* stream = serializar_get_pag(numPagina,pid,&sizeStream);

	if (send(socket, stream, sizeStream, 0) == -1) {
			free(stream);
			return false;
	}
	free(stream);

	return true;
}

bool send_reserva_pagina_swamp(int socketCliente, uint32_t pid, int numPagina){

	send_swap_out(socketCliente, 1, pid);
	MEM_SWAP_MESSAGE respuesta;
	recv(socketCliente, &respuesta, sizeof(MEM_SWAP_MESSAGE), 0);

	if(respuesta != SUFFICIENT_SPACE){
		return false;
	}

	void* buffer = malloc(cfg->tamanioPagina);
	memset(buffer, 0, cfg->tamanioPagina);

	size_t sizeStream = sizeof(int)*2 + cfg->tamanioPagina;

	void* stream = malloc(sizeStream);

	memcpy(stream , &numPagina, sizeof(int));
	memcpy(stream + sizeof(int), &(cfg->tamanioPagina), sizeof(int));
	memcpy(stream + sizeof(int)+ sizeof(uint32_t), buffer, cfg->tamanioPagina);
	free(buffer);

	if(send(socketCliente, stream, sizeStream, 0) == -1) {
		 free(stream);
		 return false;
	  }

	recv(socketCliente, &respuesta, sizeof(MEM_SWAP_MESSAGE), 0);

	if(respuesta != PAGE_ADDED){
		free(stream);
		return false;
	}

	 free(stream);
	 return true;
}

bool send_remover_pag(int socketCliente, uint32_t pid,int indice){
	 void* stream = malloc(sizeof(int)+ sizeof(uint32_t) + sizeof(MEM_SWAP_MESSAGE));
	 size_t sizeStream = sizeof(int)+ sizeof(uint32_t) + sizeof(MEM_SWAP_MESSAGE);
	 MEM_SWAP_MESSAGE opCode = DELETE_PAGE;

	 memcpy(stream, &opCode , sizeof(MEM_SWAP_MESSAGE));
	 memcpy(stream + sizeof(MEM_SWAP_MESSAGE) , &pid, sizeof(uint32_t));
	 memcpy(stream + sizeof(MEM_SWAP_MESSAGE) + sizeof(int), &indice, sizeof(int));

	 if (send(socketCliente, stream, sizeStream, 0) == -1) {
		 free(stream);
		 return false;
	  }
	 free(stream);
	 return true;
}

bool send_swap_out(int socketCliente, int numero, uint32_t pid) {

	 void* stream = malloc(sizeof(int)+ sizeof(uint32_t) + sizeof(MEM_SWAP_MESSAGE));
	 size_t sizeStream = sizeof(int)+ sizeof(uint32_t) + sizeof(MEM_SWAP_MESSAGE);
	 MEM_SWAP_MESSAGE opCode = ADD_PAGES;

	 memcpy(stream, &opCode , sizeof(MEM_SWAP_MESSAGE));
	 memcpy(stream + sizeof(MEM_SWAP_MESSAGE) , &pid, sizeof(uint32_t));
	 memcpy(stream + sizeof(MEM_SWAP_MESSAGE) + sizeof(int), &numero, sizeof(int));

	 if (send(socketCliente, stream, sizeStream, 0) == -1) {
		 free(stream);
		 return false;
	  }
	 free(stream);
	 return true;
}

bool send_cantidad_reserva_swamp(int socketCliente,uint32_t pid, int cantidadFramesNecesarios){
	MEM_SWAP_MESSAGE opCode = IS_SPACE_SUFFICIENT;

	if(send(socketCliente,&opCode,sizeof(MEM_SWAP_MESSAGE),0) == -1){
		return false;
	}

	if(send(socketCliente,&pid,sizeof(uint32_t),0) == -1){
		return false;
	}

	if(send(socketCliente,&cantidadFramesNecesarios,sizeof(int),0) == -1){
		return false;
	}

	return true;
}

bool send_finalizar_proceso_swamp(int socketCliente,uint32_t pid){
	MEM_SWAP_MESSAGE opCode = RELEASE_PROCESS;

	if(send(socketCliente,&opCode,sizeof(MEM_SWAP_MESSAGE),0) == -1){
		return false;
	}

	if(send(socketCliente,&pid,sizeof(uint32_t),0) == -1){
		return false;
	}

	return true;
}

void* leer_memoria_pag(int frame) {
    //desplazamiento en memoria
    int desplazamiento = frame * cfg->tamanioPagina;

	void* pagina = malloc(cfg->tamanioPagina);

	printf("Se va a leer la pagina en RAM que arranca en %d \n", desplazamiento);
	pthread_mutex_lock(&mutexEscribirMemoria);
	memcpy(pagina, memoriaPrincipal+desplazamiento, cfg->tamanioPagina);
	pthread_mutex_unlock(&mutexEscribirMemoria);


	return pagina;

}

void* serializar_pagina(uint32_t pid, int indice,void* contenidoPagina,size_t* sizeStream){

	*sizeStream =
	sizeof(MEM_SWAP_MESSAGE)+
	sizeof(uint32_t) +
	sizeof(int)+
	sizeof(int)+
	cfg->tamanioPagina
	;

	void* stream = malloc(*sizeStream);
	MEM_SWAP_MESSAGE codOp = ADD_PAGE;

	memcpy(
		stream,
		&codOp,
		sizeof(MEM_SWAP_MESSAGE)
		);
	memcpy(
		stream+sizeof(MEM_SWAP_MESSAGE),
		&pid,
		sizeof(uint32_t)
	);
	memcpy(
		stream+sizeof(MEM_SWAP_MESSAGE) + sizeof(uint32_t),
		&indice,
		sizeof(int)
	);
	memcpy(
		stream+sizeof(MEM_SWAP_MESSAGE) + sizeof(uint32_t) + sizeof(int),
		&(cfg->tamanioPagina),
		sizeof(int)
	);
	memcpy(
		stream+sizeof(MEM_SWAP_MESSAGE) + sizeof(uint32_t) + sizeof(int) + sizeof(int),
		contenidoPagina,
		cfg->tamanioPagina
	);


	return stream;
}

void* serializar_get_pag(int indice,uint32_t pid,size_t* sizeStream){

	*sizeStream =
	sizeof(MEM_SWAP_MESSAGE)+
	sizeof(int)+
	sizeof(uint32_t)
	;

	void* stream = malloc(*sizeStream);
	MEM_SWAP_MESSAGE opCode = READ_PAGE;

	memcpy(
		stream,
		&opCode,
		sizeof(MEM_SWAP_MESSAGE)
		);
	memcpy(
		stream + sizeof(MEM_SWAP_MESSAGE),
		&pid,
		sizeof(uint32_t)
	);
	memcpy(
		stream + sizeof(MEM_SWAP_MESSAGE) + sizeof(uint32_t),
		&indice,
		sizeof(int)
	);


	return stream;
}



void* recibir_pagina(int socketCliente, bool* recibido){
	void * buffer;
	int tamanioPagina;

	recv(socketCliente, &tamanioPagina, sizeof(int), MSG_WAITALL);
	//recv(socketCliente, &tamanioPagina, sizeof(int), MSG_WAITALL);

	buffer = malloc(tamanioPagina);

	if (recv(socketCliente, buffer, tamanioPagina, MSG_WAITALL) != tamanioPagina) {

		*recibido = false;
			}


	//printf("Me llego el size %d \n", *size);
	//printf("Me llego el mensaje %d \n", *b);
	*recibido = true;

	return buffer;
}

t_info_pagina* pagina_a_reemplazar(uint32_t pid) {

	t_info_pagina* info_paginaAReemplazar;
	t_list* paginas_ppal;

	if(cfg->esFija == false){ //aca va dinamica-global
		//BUSCAMOS TODAS LAS PAGINAS PORQUE LA SUSTITUCION ES GLOBAL
		paginas_ppal = buscarInfosPaginasEnRam();

	}else{
		paginas_ppal = buscarInfosPaginasCarpinchoEnRam(pid);
	}

	bool ordenarPorFrame(t_info_pagina* info_pagina1, t_info_pagina* info_pagina2) {
			return info_pagina1->frame < info_pagina2->frame;
		}

		list_sort(paginas_ppal, (void*) ordenarPorFrame);

	if(cfg->esClock == false) //usar LRU
	{
		//ordeno las info_pagina por tiempo de uso (la que tenga el menor tiempo sera la victima)

		t_info_pagina* _LRU(t_info_pagina* pag1, t_info_pagina* pag2)
		{
			if(pag2->tiempo_uso > pag1->tiempo_uso) return pag1;
			else return pag2;
		}
		// obtengo la pagina LRU
		info_paginaAReemplazar = list_get_minimum(paginas_ppal, (void*) _LRU);
		tpCarpincho* tpCarpincho = tpCarpincho_con_num_frame(info_paginaAReemplazar->frame);
		log_info(logger, "PID de la pagina a meter: %d", pid);
		log_info(logger, "Victima LRU: pagina:%d - frame:%d - tiempo: %d - pid: %d \n", info_paginaAReemplazar->indice,
		info_paginaAReemplazar->frame, info_paginaAReemplazar->tiempo_uso, tpCarpincho->pid);
	}

	if(cfg->esClock == true)
	{


		t_info_pagina* recorredorPaginas;
		int cantidadFrames = list_size(paginas_ppal);

		//esta es la primera vuelta para encontrar 0|0
		for(int i = 0; i < cantidadFrames ; i++){
			if(punteroClock == cantidadFrames)
			{
				punteroClock = 0;
			}

			recorredorPaginas = list_get(paginas_ppal, punteroClock);
			punteroClock++;

			if(recorredorPaginas->bitDeUso == 0 && recorredorPaginas->bitDeModificado == 0 ){
				tpCarpincho* tpCarpincho = tpCarpincho_con_num_frame(recorredorPaginas->frame);
				log_info(logger, "PID de la pagina a meter: %d", pid);
				log_info(logger, "Victima CLOCK-M: pagina:%d - frame:%d - tiempo: %d - pid: %d \n", recorredorPaginas->indice,
						recorredorPaginas->frame, recorredorPaginas->tiempo_uso, tpCarpincho->pid);
				return recorredorPaginas;
			}

		}

		//esta segunda vuelta es para encontrar 0|1 modificando el bit de uso
		for(int i = 0; i < cantidadFrames ; i++){
			if(punteroClock == cantidadFrames)
			{
				punteroClock = 0;
			}

			recorredorPaginas = list_get(paginas_ppal, punteroClock);
			punteroClock++;

			if(recorredorPaginas->bitDeUso == 0 && recorredorPaginas->bitDeModificado == 1 ){
				tpCarpincho* tpCarpincho = tpCarpincho_con_num_frame(recorredorPaginas->frame);
				log_info(logger, "PID de la pagina a meter: %d", pid);
				log_info(logger, "Victima CLOCK-M: pagina:%d - frame:%d - tiempo: %d - pid: %d \n", recorredorPaginas->indice,
				recorredorPaginas->frame, recorredorPaginas->tiempo_uso, tpCarpincho->pid);
				return recorredorPaginas;
			}

			recorredorPaginas->bitDeUso = 0;

		}

		//esta tercera vuelta es para encontrar 0|0
		for(int i = 0; i < cantidadFrames ; i++){
			if(punteroClock == cantidadFrames)
			{
				punteroClock = 0;
			}

			recorredorPaginas = list_get(paginas_ppal, punteroClock);
			punteroClock++;

			if(recorredorPaginas->bitDeUso == 0 && recorredorPaginas->bitDeModificado == 0 ){
				tpCarpincho* tpCarpincho = tpCarpincho_con_num_frame(recorredorPaginas->frame);
				log_info(logger, "PID de la pagina a meter: %d", pid);
				log_info(logger, "Victima CLOCK-M: pagina:%d - frame:%d - tiempo: %d- pid:  \n", recorredorPaginas->indice,
				recorredorPaginas->frame, recorredorPaginas->tiempo_uso, tpCarpincho->pid);
				return recorredorPaginas;
			}

		}

		//esta segunda vuelta es para encontrar 0|1
		for(int i = 0; i < cantidadFrames ; i++){
			if(punteroClock == cantidadFrames)
			{
				punteroClock = 0;
			}

			recorredorPaginas = list_get(paginas_ppal, punteroClock);
			punteroClock++;

			if(recorredorPaginas->bitDeUso == 0 && recorredorPaginas->bitDeModificado == 1 ){
				tpCarpincho* tpCarpincho = tpCarpincho_con_num_frame(recorredorPaginas->frame);
				log_info(logger, "PID de la pagina a meter: %d", pid);
				log_info(logger, "Victima CLOCK-M: pagina:%d - frame:%d - tiempo: %d - pid: %d \n", recorredorPaginas->indice,
				recorredorPaginas->frame, recorredorPaginas->tiempo_uso, tpCarpincho->pid);
				return recorredorPaginas;
			}
		}
	}

	list_destroy(paginas_ppal);
	return info_paginaAReemplazar;

}

t_list* buscarInfosPaginasEnRam() {

	t_list* listaInfoPagsEnRam = list_create();

	void buscarPagsEnRam(tpCarpincho* tablaPCarpincho) {

		void pagsEnRam(t_info_pagina* info_pagina) {

			if(info_pagina->bitPresencia == 1)
			{
				list_add(listaInfoPagsEnRam, info_pagina);
			}
		}

		list_iterate(tablaPCarpincho->paginas, (void*) pagsEnRam);
	}

	list_iterate(listaTablasCarpinchos, (void*) buscarPagsEnRam);
	return listaInfoPagsEnRam;
}

t_list* buscarInfosPaginasCarpinchoEnRam(uint32_t pid){

	tpCarpincho* tablaPCarpincho = obtener_tpCarpincho_con_pid(pid);

	t_list* listaInfoPagsEnRam = list_create();

	void crearLista(t_info_pagina* pagina) {

		if(pagina->bitPresencia){
			list_add(listaInfoPagsEnRam, pagina);
		}

	}

	list_iterate(tablaPCarpincho->paginas, (void*) crearLista);
	return listaInfoPagsEnRam;
}

int cantidad_frames_usados_carpincho(tpCarpincho* tpCarpincho){
	int contador = 0;

	void esta_en_memoria(t_info_pagina* pagina) {

		if(pagina->bitPresencia){
			contador++;
		}
	}

	list_iterate(tpCarpincho->paginas, (void*) esta_en_memoria);
	return contador;
}

t_info_pagina* t_info_pagina_con_num_frame(int numFrame){
	t_list* paginasEnRam = buscarInfosPaginasEnRam();

	bool pagina_esta_en_frame(void* x) {
		if(x == NULL){

				return false;
			}else{
		t_info_pagina* elem = x;
	    return (elem->frame == numFrame) && (elem->bitPresencia==1);
			}
	}
	t_info_pagina* pagina = list_find(paginasEnRam, &pagina_esta_en_frame);
	list_destroy(paginasEnRam);

	return pagina;
}

//los parametros es lo que queres meter en memoria
bool ejecutar_reemplazo(void* buffer, t_info_pagina* info_pagina, int pid) {

	t_info_pagina* info_paginaAReemplazar =  pagina_a_reemplazar(pid);
	tpCarpincho* carpincho = tpCarpincho_con_num_frame(info_paginaAReemplazar->frame);

	int frame = info_paginaAReemplazar->frame;
	int biteme = info_paginaAReemplazar->bitDeModificado;
	info_paginaAReemplazar->bitPresencia = 0;

	if(!send_pag_swamp(socketSwamp,info_paginaAReemplazar, carpincho->pid)){
		return false;
	}

	if(biteme != 0){
		MEM_SWAP_MESSAGE respuesta;
		recv(socketSwamp, &respuesta, sizeof(MEM_SWAP_MESSAGE), 0);

		if(respuesta != PAGE_ADDED){
			return false;
		}
	}


	//GUARDAMOS EN RAM LO QUE SE QUIERE USAR
	info_pagina->frame = frame;
	info_pagina->bitPresencia = 1;
	info_pagina->bitDeUso = 1;
	//info_pagina->tiempo_uso = obtener_tiempo();

	sobreescribir_pagina(frame, buffer);

	return true;
}

int obtener_tiempo() {
	pthread_mutex_lock(&mutexTiempo);
    double t = tiempo;
    tiempo++;
    pthread_mutex_unlock(&mutexTiempo);
    return t;
}

int obtener_tiempo_TLB() {
	pthread_mutex_lock(&mutexTiempoTLB);
    double t = tiempoTLB;
    tiempoTLB++;
    pthread_mutex_unlock(&mutexTiempoTLB);
    return t;
}

//en caso de usar esto en memalloc buffer tiene que ser puro ceros
void sobreescribir_con_ceros(int frame, int desplInicialDentroPagina, int bytesAEscribir) {

	void* buffer = malloc(bytesAEscribir);
	memset(buffer, 0, bytesAEscribir);

	int desp = frame * cfg->tamanioPagina + desplInicialDentroPagina;


	pthread_mutex_lock(&mutexEscribirMemoria);
	memcpy(memoriaPrincipal+desp, buffer, bytesAEscribir);
	pthread_mutex_unlock(&mutexEscribirMemoria);
	printf("Se sobreescribio en RAM: FRAME: %d | DESDE: %d | HASTA: %d \n", frame,
	desplInicialDentroPagina, bytesAEscribir + desplInicialDentroPagina -1);

	free(buffer);
}

void sobreescribir_pagina(int frame, void* buffer) {

	int desp = frame * cfg->tamanioPagina;

	pthread_mutex_lock(&mutexEscribirMemoria);
	memcpy(memoriaPrincipal+desp, buffer, cfg->tamanioPagina);
	pthread_mutex_unlock(&mutexEscribirMemoria);
	printf("Se sobreescribio en RAM: FRAME: %d | DESDE: %d | HASTA: %d \n", frame,
	0, cfg->tamanioPagina -1);

}

uint32_t buscar_frame_disponible() {

    for(uint32_t f = 0; f < cantidadFramesTotal; f++)
    {
        if(!valor_frame(f)) {

            return f;
        }
    }


    printf("No se encontro un frame disponible en RAM \n");

    return -1;
}


int cantidad_paginas(int size){
	if(size < 0){
		return 0;
	}

	double a = (double) size/cfg->tamanioPagina;

	return ceil(a);
}

bool valor_frame(int frame) {
    //capaz ponersemaforo lo dejo porque es un buen recordatorio
	bool a = bitarray_test_bit(framesOcupados, frame);
	//capaz ponersemaforo lo dejo porque es un buen recordatorio

	return a;
}

void ocupar_frame(int frame) {
    //capaz poner semaforo
    bitarray_set_bit(framesOcupados, frame);
    //capaz ponersemaforo lo dejo porque es un buen recordatorio
}


void vaciar_frame(int frame){
	//lock(&mutexBitarray);
	bitarray_clean_bit(framesOcupados, frame);
	//unlock(&mutexBitarray);

}

int cantidad_frames_libres(){

	int libres=0;
	//semaforo
	for(int i = 0;i<cantidadFramesTotal;i++){
		bool a = bitarray_test_bit(framesOcupados, i);
		if(!a){
			libres++;
		}
	}
	//semaforo
	return libres;
}

int nuevo_indice_pagina(uint32_t pid){
	tpCarpincho* tpCarpincho = obtener_tpCarpincho_con_pid(pid);

	int cantidadPaginasCarpincho = list_size(tpCarpincho->paginas);

	return cantidadPaginasCarpincho;
}

HeapMetadata* primer_heapMetadata_carpincho(uint32_t pid){
	tpCarpincho* tpCarpincho = obtener_tpCarpincho_con_pid(pid);

	if(list_is_empty(tpCarpincho->paginas)){
		return NULL;
	}

	t_info_pagina* primeraPagina = list_get(tpCarpincho->paginas , 0);



	HeapMetadata* primerHeap = memoriaPrincipal + primeraPagina->frame * cfg->tamanioPagina;
	return primerHeap;
}

HeapMetadata* ultimo_heapMetadata_carpincho(uint32_t pid,bool* ultimoHeapPartido,int* restante, uint32_t* direcLogicaUltimoHeap){
	HeapMetadata* heap = heap_correspondiente(sizeof(HeapMetadata), pid, ultimoHeapPartido, restante);
	if(heap == NULL){
		return NULL;
	}

	*direcLogicaUltimoHeap = 0;
	uint32_t nextAlloc = heap->nextAlloc;

	while(heap->nextAlloc != (uint32_t)NULL){
		nextAlloc = heap->nextAlloc;

		if(*ultimoHeapPartido){
			free(heap);
		}

		*direcLogicaUltimoHeap = nextAlloc;
		heap = heap_correspondiente(nextAlloc + sizeof(HeapMetadata), pid, ultimoHeapPartido, restante);

		if(heap == NULL){
			printf("El heap es NULL en el ultimo_heapmetadata_carpincho \n");
			return NULL;
		}
		//printf("prev ultimoHeapMetadata: %d \n", heap->prevAlloc);
		//printf("next ultimoHeapMetadata: %d \n", heap->nextAlloc);
	}
	return heap;
}

void* calcular_direccion_fisica_siguiente_heap(uint32_t pid,uint32_t direcLogica){

	int numeroPagina =(int) floor(direcLogica/cfg->tamanioPagina);
	tpCarpincho* tpCarpincho = obtener_tpCarpincho_con_pid(pid);
	t_info_pagina* pagina = t_info_pagina_con_numero_pagina(numeroPagina, tpCarpincho);

	if(hay_heap_partido(pagina->indice, direcLogica)){
		HeapMetadata* siguienteHeap = malloc(sizeof(HeapMetadata));
		void* comienzoPrimeraMitad = calcular_direccion_fisica(pid, direcLogica);
		int tamanioPrimeraMitad =  (pagina->indice + 1) * cfg->tamanioPagina - direcLogica;
		memcpy(siguienteHeap,comienzoPrimeraMitad, tamanioPrimeraMitad);

		t_info_pagina* siguientePagina = t_info_pagina_con_numero_pagina(pagina->indice + 1, tpCarpincho);
		void* comienzoSegundaMitad = memoriaPrincipal + siguientePagina->frame*cfg->tamanioPagina;
		memcpy(siguienteHeap + tamanioPrimeraMitad, comienzoSegundaMitad, sizeof(HeapMetadata) - tamanioPrimeraMitad);

		return siguienteHeap;

	}else{
		return calcular_direccion_fisica( pid, direcLogica);
	}


}

bool hay_heap_partido(int indice, uint32_t direcLogica){

	if(direcLogica + sizeof(HeapMetadata) > (indice + 1) * cfg->tamanioPagina){
		return true;
	}else{
		return false;
	}
}

void* calcular_direccion_fisica(uint32_t pid,uint32_t direcLogica){
	int numeroPagina =(int)(direcLogica/cfg->tamanioPagina);
	int offset = direcLogica%cfg->tamanioPagina;
	int frameTLB = buscar_frame_tlb(numeroPagina, pid);

	tpCarpincho* tpCarpincho = obtener_tpCarpincho_con_pid(pid);
	t_info_pagina* pagina = t_info_pagina_con_numero_pagina(numeroPagina, tpCarpincho);

	if(pagina == NULL){

		return NULL;
	}

	if(!pagina->bitPresencia){
		if(!traer_pag_correspondiente(pagina->indice,pid)){
			return NULL;
		}
	}

	//pagina->tiempo_uso=obtener_tiempo();
	pagina->bitDeUso = 1;

	if(frameTLB != -1){
		return memoriaPrincipal + frameTLB * cfg->tamanioPagina + offset;
	}
	actualizarTLB(pid, numeroPagina, pagina->frame);


	return memoriaPrincipal + pagina->frame * cfg->tamanioPagina + offset;


}

t_info_pagina* t_info_pagina_con_numero_pagina(int numeroPagina,tpCarpincho* tpCarpincho){

	bool pagina_tiene_indice(void* elem){
		return ((t_info_pagina*) elem)->indice == numeroPagina;
	}

	//semaforo
	t_info_pagina* t_info_pagina = list_find(tpCarpincho->paginas,&pagina_tiene_indice);
	//semaforo
	if(t_info_pagina != NULL){
		t_info_pagina->tiempo_uso = obtener_tiempo();

	}

	return t_info_pagina;
}

//----------------------------------------MEMREAD-------------------------------------------------------------

void* leer_memoria(uint32_t direcLogica ,int size, uint32_t pid){
	//sacar la pagina con la direc logica
	if(direcLogica == (uint32_t) NULL){
		return NULL;
	}

	bool heapEstaPartido = false;
	int restoHeap;

	HeapMetadata* heap = heap_correspondiente_a_DL(pid,direcLogica,&heapEstaPartido,&restoHeap);
	//HeapMetadata* heap = heap_correspondiente(direcLogica, pid ,&heapEstaPartido,&restoHeap);

	if(heap == NULL){
		return NULL;
	}


	void* contenido = malloc(size);
	tpCarpincho* tpCarpincho = obtener_tpCarpincho_con_pid(pid);

	if(tpCarpincho ==  NULL){
		if(heapEstaPartido){
			free(heap);
		}
		free(contenido);
		return NULL;
	}

	void* direcFisicaPrincipio = calcular_direccion_fisica(pid, direcLogica);

	int numPagina = direcLogica/cfg->tamanioPagina;


	if(heap->nextAlloc - direcLogica < size || heap->isFree){//por si me pide mas de lo que tiene ese alloc

		log_info(logger, "El carpincho de pid: %d no puede leer mas de lo que tiene disponible el alloc");

		if(heapEstaPartido){
			free(heap);
		}
		free(contenido);
		return NULL;
	}

	void* direcFisicaFinal =  calcular_direccion_fisica(pid, direcLogica + size);

	int cantPaginasALeer;

	int numPagFinal = (direcLogica + size)/cfg->tamanioPagina;

	cantPaginasALeer = numPagFinal - numPagina + 1;
	if((direcLogica + size)%cfg->tamanioPagina == 0){
		cantPaginasALeer--;
	}

	int desplazamiento = 0;

	for(int i=0; i < cantPaginasALeer; i++){
		t_info_pagina* pagina = t_info_pagina_con_numero_pagina(numPagina, tpCarpincho);

		//enviar el numero de pagina que queremos recibir junto con el pid
		//recibir la pagina
		//actualizar las estructuras (bitPresencia)

		if(!traer_pag_correspondiente(numPagina,pid)){
			if(heapEstaPartido){
				free(heap);
			}
			free(contenido);

			return NULL;
		}


		if(i != 0){
			direcFisicaPrincipio = memoriaPrincipal + pagina->frame*cfg->tamanioPagina;

		}

		void* finalPagina = memoriaPrincipal + (pagina->frame + 1)*cfg->tamanioPagina;

		if(!heap->isFree && heap->nextAlloc == 0){
			t_info_pagina* paginaFinal = t_info_pagina_con_numero_pagina(numPagFinal - 1, tpCarpincho);
			direcFisicaFinal =  memoriaPrincipal + (paginaFinal->frame +1)*cfg->tamanioPagina;
		}

		if(finalPagina <= direcFisicaFinal){
			memcpy(contenido + desplazamiento, direcFisicaPrincipio, finalPagina - direcFisicaPrincipio);
			desplazamiento += finalPagina - direcFisicaPrincipio;
		}	else{
			memcpy(contenido + desplazamiento, direcFisicaPrincipio, direcFisicaFinal - direcFisicaPrincipio);
			desplazamiento += direcFisicaFinal - direcFisicaPrincipio;

		}

		numPagina++;
	}

	if(heapEstaPartido){ //contemplar que la direcLogica no siempre estara al final del heap
		//memcpy(calcular_direccion_fisica(pid, direcLogica - sizeof(HeapMetadata)),heap,sizeof(HeapMetadata)-restoHeap);
		//memcpy(calcular_direccion_fisica(pid, direcLogica - restoHeap) - restoHeap,heap+(sizeof(HeapMetadata)-restoHeap),restoHeap);

		free(heap);
	}

	return contenido;

}

//----------------------------------------MEMWRITE-------------------------------------------------------------


bool escribir_memoria(uint32_t direcLogica,uint32_t pid, int size, void* contenido){

	//sacar la pagina con la direc logica
	if(direcLogica == (uint32_t) NULL){
		return false;
	}

	tpCarpincho* tpCarpincho = obtener_tpCarpincho_con_pid(pid);

	if(tpCarpincho ==  NULL){
		return false;
	}


	int numPagina = direcLogica/cfg->tamanioPagina;

	bool heapEstaPartido = false;
	int restoHeap;

	HeapMetadata* heap = heap_correspondiente_a_DL(pid,direcLogica,&heapEstaPartido,&restoHeap);
	//HeapMetadata* heap = heap_correspondiente(direcLogica, pid ,&heapEstaPartido,&restoHeap);

	void* direcFisicaPrincipio = calcular_direccion_fisica(pid, direcLogica);

	if(heap == NULL){
		return false;
	}

	if(heap->nextAlloc - direcLogica < size || heap->isFree){//por si quiere leer mas de lo que tiene el alloc
		log_info(logger, "El carpincho de pid: %d no puede escribir mas de lo que tiene disponible el alloc");
		if(heapEstaPartido){ //contemplar que la direcLogica no siempre estara al final del heap
			free(heap);
		}

		return false;
	}

	void* direcFisicaFinal =  calcular_direccion_fisica(pid, direcLogica + size);

	int cantPaginasALeer;

	int numPagFinal = (direcLogica + size)/cfg->tamanioPagina;

	cantPaginasALeer = numPagFinal - numPagina + 1;
	if((direcLogica + size)%cfg->tamanioPagina == 0){
		cantPaginasALeer--;
	}

	int desplazamiento = 0;

	for(int i=0; i < cantPaginasALeer; i++){
		t_info_pagina* pagina = t_info_pagina_con_numero_pagina(numPagina, tpCarpincho);

		//enviar el numero de pagina que queremos recibir junto con el pid
		//recibir la pagina
		//actualizar las estructuras (bitPresencia)
		if(!traer_pag_correspondiente(numPagina,pid)){
			if(heapEstaPartido){
				free(heap);
			}
			return NULL;
		}

		if(i != 0){
			direcFisicaPrincipio = memoriaPrincipal + pagina->frame*cfg->tamanioPagina;

		}

		void* finalPagina = memoriaPrincipal + (pagina->frame + 1)*cfg->tamanioPagina;

		if(!heap->isFree && heap->nextAlloc == 0){
			t_info_pagina* paginaFinal = t_info_pagina_con_numero_pagina(numPagFinal - 1, tpCarpincho);
			direcFisicaFinal =  memoriaPrincipal + (paginaFinal->frame +1)*cfg->tamanioPagina;
		}

		if(finalPagina <= direcFisicaFinal){
			memcpy(direcFisicaPrincipio, contenido + desplazamiento, finalPagina - direcFisicaPrincipio);
			desplazamiento += finalPagina - direcFisicaPrincipio;
		}	else{
			memcpy(direcFisicaPrincipio, contenido + desplazamiento, direcFisicaFinal - direcFisicaPrincipio);
			desplazamiento += direcFisicaFinal - direcFisicaPrincipio;

		}
		pagina->bitDeModificado = 1;
		numPagina++;
	}

	if(heapEstaPartido){ //contemplar que la direcLogica no siempre estara al final del heap
		//memcpy(calcular_direccion_fisica(pid, direcLogica - sizeof(HeapMetadata)),heap,sizeof(HeapMetadata)-restoHeap);
		//memcpy(calcular_direccion_fisica(pid, direcLogica - restoHeap) - restoHeap,heap+(sizeof(HeapMetadata)-restoHeap),restoHeap);

		free(heap);
	}

	return true;
}

//----------------------------------------MEMFREE-------------------------------------------------------------


bool liberar_memoria(uint32_t direcLogica,uint32_t pid){

	//buscamos el heap correspondiente
	tpCarpincho* tpCarpi = obtener_tpCarpincho_con_pid(pid);

	if(tpCarpi ==  NULL){
		return false;
	}

	int resto = 0;
	HeapMetadata* heap;
	bool heapActualPartido = false;
	uint32_t direcLogicaHeap;

	heap = heap_apto_free(direcLogica, pid, &heapActualPartido, &resto, &direcLogicaHeap);

	if(heap == NULL || (heap->nextAlloc == (uint32_t) NULL)){
		return false;
	}

	if(heap->isFree){
		printf("Esta direccion logica ya estaba libre \n");
		return true;
	}

	//lo ponemos en isFree true
	heap->isFree = true;
	//memoriaDisponible += heap->nextAlloc - direcLogica;
	//veo los heaps de atras y adelante a ver si alguno esta libre para consolidar
	HeapMetadata* heapSiguiente;
	HeapMetadata* heapAnterior;
	bool heapSiguientePartido = false;
	int restoSiguiente = 0;
	bool heapAnteriorPartido = false;
	int restoAnterior = 0;

	bool esPrimerHeap = heap == primer_heapMetadata_carpincho(pid);
	if(esPrimerHeap){
		heapAnterior = NULL;
	}else{
		HeapMetadata* heapAnteriorAux = heap_correspondiente(heap->prevAlloc + sizeof(HeapMetadata), pid, &heapAnteriorPartido, &restoAnterior);
		if(heapAnteriorAux == NULL){
			return false;
		}
		if(heapAnteriorPartido){
			heapAnterior = malloc(sizeof(HeapMetadata));
			memcpy(heapAnterior,heapAnteriorAux,sizeof(HeapMetadata));
			free(heapAnteriorAux);
		}else{
			heapAnterior = heapAnteriorAux;
		}

	}

	bool esUltimoHeap = heap->nextAlloc == (uint32_t) NULL;
	if(esUltimoHeap){
		heapSiguiente = NULL;
	}else{
		HeapMetadata* heapSiguienteAux = heap_correspondiente(heap->nextAlloc + sizeof(HeapMetadata), pid, &heapSiguientePartido, &restoSiguiente);
		if(heapSiguienteAux == NULL){
			return false;
		}

		if(heapSiguientePartido){
			heapSiguiente = malloc(sizeof(HeapMetadata));
			memcpy(heapSiguiente,heapSiguienteAux,sizeof(HeapMetadata));
			free(heapSiguienteAux);
		}else{
			heapSiguiente = heapSiguienteAux;
		}

	}

	//siguiente del siguiente y anterior del anterior
	HeapMetadata* heapSiguienteSiguiente = NULL;
	bool heapSiguienteSiguientePartido = false;
	int restoSiguienteSiguiente = 0;
	uint32_t dirLogicaSiguiente = heap->nextAlloc;
	bool siguienteEsUltimo = false;

	if(heapSiguiente != NULL){
		if(heapSiguiente->nextAlloc == (uint32_t) NULL){
			siguienteEsUltimo = true;
		}

		if(heapSiguiente->isFree){//actualizo el siguiente siguiente y el actual

			if(heapSiguiente->nextAlloc != (uint32_t) NULL){
				heapSiguienteSiguiente = heap_correspondiente(heapSiguiente->nextAlloc + sizeof(HeapMetadata), pid, &heapSiguienteSiguientePartido, &restoSiguienteSiguiente);

				if(heapSiguienteSiguiente != NULL){
					heapSiguienteSiguiente->prevAlloc = heapSiguiente->prevAlloc;
				}

			}

			heap->nextAlloc = heapSiguiente->nextAlloc;
			//memoriaDisponible += sizeof(HeapMetadata);
		}
	}

	if(heapAnterior != NULL){
		if(heapAnterior->isFree){ //me quedo con ese y al actual no lo apunta nadie
			heapAnterior->nextAlloc = heap->nextAlloc;

			if(heapSiguiente != NULL){
				if(heapSiguiente->isFree){
					if(heapSiguienteSiguiente != NULL){
						heapSiguienteSiguiente->prevAlloc = heap->prevAlloc;
					}else{
						heapAnterior->nextAlloc = (uint32_t) NULL;
					}

				}else{
					heapSiguiente->prevAlloc = heap->prevAlloc;
				}
			}
			//memoriaDisponible += sizeof(HeapMetadata);

		}
	}


	//si no estan libres los de los costados solo se pone en free
	//actualizamos los heaps que modificamos si estan partidos
	if(heapActualPartido){
		void* direcFisicaActual = calcular_direccion_fisica(pid, direcLogica - sizeof(HeapMetadata));
		memcpy(direcFisicaActual, heap, sizeof(HeapMetadata)-resto);
		memcpy(calcular_direccion_fisica(pid, direcLogica -(sizeof(HeapMetadata)-resto)),heap + (sizeof(HeapMetadata)-resto),resto);
	}

	if(heapSiguientePartido){
		void* direcFisicaActual = calcular_direccion_fisica(pid, heap->nextAlloc);
		memcpy(direcFisicaActual, heapSiguiente, sizeof(HeapMetadata)-restoSiguiente);
		memcpy(calcular_direccion_fisica(pid, heap->nextAlloc +(sizeof(HeapMetadata)-restoSiguiente)),heapSiguiente + (sizeof(HeapMetadata)-restoSiguiente),restoSiguiente);
	}

	if(heapAnteriorPartido){
		void* direcFisicaActual = calcular_direccion_fisica(pid, heap->prevAlloc);
		memcpy(direcFisicaActual, heapAnterior, sizeof(HeapMetadata)-restoAnterior);
		memcpy(calcular_direccion_fisica(pid, heap->prevAlloc +(sizeof(HeapMetadata)-restoAnterior)),heapAnterior + (sizeof(HeapMetadata)-restoAnterior),restoAnterior);
	}


	if(heapSiguienteSiguientePartido){
		void* direcFisicaActual = calcular_direccion_fisica(pid, heapSiguiente->nextAlloc);
		memcpy(direcFisicaActual, heapSiguienteSiguiente, sizeof(HeapMetadata)-restoSiguienteSiguiente);
		memcpy(calcular_direccion_fisica(pid, heapSiguiente->nextAlloc + (sizeof(HeapMetadata)-restoSiguienteSiguiente)),heapSiguienteSiguiente + (sizeof(HeapMetadata)-restoSiguienteSiguiente),restoSiguienteSiguiente);
	}

	//ver si libero la pag entera para liberarla tmb (marcar el frame como libre)
	if(heapAnterior != NULL){
		if(heapAnterior->isFree){
			direcLogicaHeap = heap->prevAlloc;

			if(heapActualPartido){
				free(heap);
			}
			heap = heapAnterior;
		}
	}


	uint32_t direcLogicaPrincipioAux = direcLogicaHeap;
	uint32_t direcLogicaFinalPagina = ((direcLogicaPrincipioAux/cfg->tamanioPagina) + 1 ) *cfg->tamanioPagina;

	if(esPrimerHeap){
		direcLogicaPrincipioAux -= sizeof(HeapMetadata);
	}

	while(direcLogicaFinalPagina <= dirLogicaSiguiente){

		if(direcLogicaFinalPagina - direcLogicaPrincipioAux == cfg->tamanioPagina){
			t_info_pagina* paginaALiberar =  t_info_pagina_con_numero_pagina(direcLogicaPrincipioAux/cfg->tamanioPagina, tpCarpi);
			//liberar pagina

			vaciar_frame(paginaALiberar->frame);

			if(heapSiguienteSiguiente == NULL){

				send_remover_pag(socketSwamp,pid,paginaALiberar->indice);

				MEM_SWAP_MESSAGE respuesta;
				if (recv(socketSwamp,&respuesta, sizeof(MEM_SWAP_MESSAGE), 0) != sizeof(MEM_SWAP_MESSAGE)) {
					printf("No se pudo recibir la respuesta de eliminacion de pagina de swamp");
				}

				if(respuesta != PAGE_DELETED){//TODO capaz no eliminarla nosotros si swamp no pudo pero qcy
					printf("No se pudo eliminar la pagina de swamp");
				}

				remover_pagina_con_indice(tpCarpi->paginas, paginaALiberar->indice);
				eliminar_entradas_tlb_pid(tpCarpi->pid, paginaALiberar->indice);
				if(tpCarpi->paginas->elements_count == 0){ //el tp carpincho ya no pinta nada en memoria
				list_destroy_and_destroy_elements(tpCarpi->paginas,free);
				bool tiene_pid(void* elem){
					tpCarpincho* tp = elem;
					return (tp->pid == tpCarpi->pid);
				}
				list_remove_and_destroy_by_condition(listaTablasCarpinchos, &tiene_pid, free);

			}

			}

			printf("Se libero el frame numero: %d \n", paginaALiberar->frame);
		}
		direcLogicaPrincipioAux = direcLogicaFinalPagina;
		direcLogicaFinalPagina = direcLogicaPrincipioAux + cfg->tamanioPagina;

	}

	int numPagHeap = direcLogicaHeap/cfg->tamanioPagina;
	int numPagHeapSiguiente = dirLogicaSiguiente/cfg->tamanioPagina;

	//TODO no libera la pagina si hago un unico heap
	if((siguienteEsUltimo && (numPagHeap != numPagHeapSiguiente)) || (esPrimerHeap && siguienteEsUltimo && (numPagHeap == numPagHeapSiguiente))){
		t_info_pagina* paginaALiberar =  t_info_pagina_con_numero_pagina(direcLogicaPrincipioAux/cfg->tamanioPagina, tpCarpi);
		//liberar pagina

		vaciar_frame(paginaALiberar->frame);

		send_remover_pag(socketSwamp,pid,paginaALiberar->indice);

		MEM_SWAP_MESSAGE respuesta;
		if (recv(socketSwamp,&respuesta, sizeof(MEM_SWAP_MESSAGE), 0) != sizeof(MEM_SWAP_MESSAGE)) {
			printf("No se pudo recibir la respuesta de eliminacion de pagina de swamp");
		}

		if(respuesta != PAGE_DELETED){//TODO capaz no eliminarla nosotros si swamp no pudo pero qcy
			printf("No se pudo eliminar la pagina de swamp");
		}


		remover_pagina_con_indice(tpCarpi->paginas, paginaALiberar->indice);
		eliminar_entradas_tlb_pid(tpCarpi->pid, paginaALiberar->indice);

		if(tpCarpi->paginas->elements_count == 0){ //el tp carpincho ya no pinta nada en memoria
			list_destroy_and_destroy_elements(tpCarpi->paginas,free);
			bool tiene_pid(void* elem){
				tpCarpincho* tp = elem;
				return (tp->pid == tpCarpi->pid);
			}
			list_remove_and_destroy_by_condition(listaTablasCarpinchos, &tiene_pid, free);

		}

		printf("Se libero el frame numero: %d \n", paginaALiberar->frame);
	}

	if(heapAnterior == NULL){
		if(heapActualPartido){
			free(heap);
		}
	}


	if(heapSiguientePartido){
		free(heapSiguiente);
	}

	if(heapAnteriorPartido){
		free(heapAnterior);
	}

	if(heapSiguienteSiguientePartido){
		free(heapSiguienteSiguiente);
	}

	return true;

}

void remover_pagina_con_indice(t_list* paginas, int indice){

	bool tiene_indice(void* elem){
		t_info_pagina* pagina = elem;
		return pagina->indice == indice;
	}

	list_remove_and_destroy_by_condition(paginas,&tiene_indice,free);

}

void* heap_correspondiente(uint32_t direcLogica,uint32_t pid, bool* partido, int* resto){
	int numPag = direcLogica/cfg->tamanioPagina;
	if(direcLogica%cfg->tamanioPagina == 0){
		numPag--;
	}
	if(!traer_pag_correspondiente(numPag,pid)){
		return NULL;
	}


	if((direcLogica%cfg->tamanioPagina < sizeof(HeapMetadata)) && direcLogica%cfg->tamanioPagina != 0){ //si el heap esta partido

		if(!traer_pag_correspondiente(numPag-1,pid)){
			printf("No se pudo hacer swap in de la pag %d \n",numPag-1);
			return NULL;
		}

		tpCarpincho* tpCarpincho = obtener_tpCarpincho_con_pid(pid);
		bool tieneDirecLogica(void* elem){
			return ((HeapMetadataPartido*)elem)->direcLogicaHeap == (direcLogica-9);
		}

		*partido = false;
		*resto = 0;

		HeapMetadataPartido* heapAux = list_find(tpCarpincho->heapsPartidos, &tieneDirecLogica);

		return heapAux->heapPartido;
	}else{
		*partido = false;
		*resto = 0;
		return calcular_direccion_fisica(pid, direcLogica - sizeof(HeapMetadata));
	}
}

bool traer_pag_correspondiente(int numPag,uint32_t pid){

	tpCarpincho* tpCarpincho = obtener_tpCarpincho_con_pid(pid);
	t_info_pagina* pagina = t_info_pagina_con_numero_pagina(numPag, tpCarpincho);

	if(pagina == NULL){
		return false;
	}

	if(!pagina->bitPresencia){
		if(!get_pag_swamp(socketSwamp,numPag,pid)){
			return false;
		}

		MEM_SWAP_MESSAGE respuesta;
		recv(socketSwamp, &respuesta, sizeof(MEM_SWAP_MESSAGE), 0);
		if(respuesta != PAGE_READ){
			return false;
		}
		bool recibido;
		void* buffer = recibir_pagina(socketSwamp,&recibido);


		if(!recibido){
		free(buffer);

		return false;
		}
		int frameDisponible = buscar_frame_disponible();

		int cantidadFramesUsadosCarpincho = cantidad_frames_usados_carpincho(tpCarpincho);

		if(frameDisponible == -1 || (cfg->esFija && cantidadFramesUsadosCarpincho == cfg->marcosMaximos)){
			if(!ejecutar_reemplazo(buffer,pagina,pid)){
				printf("No se pudo hacer swap in de la pag %d \n",numPag);
				free(buffer);

				return false;
			}
		}else{
				pagina->bitPresencia = 1;
				pagina->frame = frameDisponible;
				ocupar_frame(frameDisponible);
				sobreescribir_pagina(frameDisponible, buffer);
			}
		actualizarTLB(pid, numPag, pagina->frame);
		free(buffer);
		}

		return true;

}

HeapMetadata* heap_correspondiente_a_DL(uint32_t pid, uint32_t direcLogica,bool* heapPartido,int* restante){
	HeapMetadata* heap = heap_correspondiente(sizeof(HeapMetadata), pid, heapPartido, restante);
	if(heap == NULL || direcLogica < sizeof(HeapMetadata)){
		log_info(logger, "La direccion logica %d no le pertenece al carpincho de pid %d", direcLogica, pid);
		return NULL;
	}

	if(heap->nextAlloc <= direcLogica && (heap->nextAlloc + sizeof(HeapMetadata)) > direcLogica){
		log_info(logger, "La direccion logica %d no le pertenece al carpincho de pid %d", direcLogica, pid);

		return NULL;
	}

	while((heap->nextAlloc + sizeof(HeapMetadata)) <= direcLogica ){

		if(heap->nextAlloc <= direcLogica && (heap->nextAlloc + sizeof(HeapMetadata)) > direcLogica){

			if(*heapPartido){
				free(heap);
			}
			return NULL;
		}

		heap = heap_correspondiente(heap->nextAlloc + sizeof(HeapMetadata), pid, heapPartido, restante);
		if(heap->nextAlloc == 0 && !heap->isFree){
			break;
		}

		if(heap == NULL){
			return NULL;
		}

		//printf("prev ultimoHeapMetadata: %d \n", heap->prevAlloc);
		//printf("next ultimoHeapMetadata: %d \n", heap->nextAlloc);
	}
	if(heap->nextAlloc <= direcLogica && (heap->nextAlloc + sizeof(HeapMetadata)) > direcLogica){
		if(*heapPartido){
			free(heap);
		}
		return NULL;
	}
	return heap;
}

HeapMetadata* heap_apto_free(uint32_t direcLogica, uint32_t pid,bool* heapPartido,int* restante, uint32_t* direcLogicaHeap){
	HeapMetadata* heap = heap_correspondiente(sizeof(HeapMetadata), pid,heapPartido,restante);
	if(heap == NULL || direcLogica < sizeof(HeapMetadata)){
		return NULL;
	}

	if(direcLogica == sizeof(HeapMetadata)){
		*direcLogicaHeap = sizeof(HeapMetadata);
		return heap;
	}

	while((heap->nextAlloc + sizeof(HeapMetadata)) <= direcLogica){

		if(heap->nextAlloc <= direcLogica && (heap->nextAlloc + sizeof(HeapMetadata)) > direcLogica){
			if(*heapPartido){
				free(heap);
			}
			return NULL;
		}

		*direcLogicaHeap = heap->nextAlloc;

		if((heap->nextAlloc + sizeof(HeapMetadata)) == direcLogica){
			heap = heap_correspondiente(heap->nextAlloc + sizeof(HeapMetadata), pid, heapPartido, restante);

			if(heap == NULL){
				return NULL;
			}

			//printf("prev ultimoHeapMetadata: %d \n", heap->prevAlloc);
			//printf("next ultimoHeapMetadata: %d \n", heap->nextAlloc);
			return heap;
		}

		heap = heap_correspondiente(heap->nextAlloc + sizeof(HeapMetadata), pid, heapPartido, restante);

		if(heap == NULL){
			return NULL;
		}

		//printf("prev ultimoHeapMetadata: %d \n", heap->prevAlloc);
		//printf("next ultimoHeapMetadata: %d \n", heap->nextAlloc);

	}
	if(*heapPartido){
		free(heap);
	}
	return NULL;
}

void crear_heap_partido(tpCarpincho* tpCarpincho, uint32_t direcLogicaUltimoHeap, t_info_pagina* pagina, int sizeHastaFinalPagina, void* posicionPrincipioHeapPartido1 ,void* posicionPrincipioHeapPartido2, int numFrameParaHeap){

	HeapMetadata* heapMetadataNuevo = malloc(sizeof(HeapMetadata));
	heapMetadataNuevo->prevAlloc = direcLogicaUltimoHeap;
	heapMetadataNuevo->nextAlloc = (uint32_t) NULL;
	heapMetadataNuevo->isFree = true;

	t_info_pagina* nuevaPagina = malloc(sizeof(*nuevaPagina));
	nuevaPagina->bitPresencia = 1;
	//nuevaPagina->bytesDisponibles = cfg->tamanioPagina - (sizeof(HeapMetadata) - pagina->bytesDisponibles);
	nuevaPagina->frame = numFrameParaHeap;
	nuevaPagina->indice = nuevo_indice_pagina(tpCarpincho->pid);
	nuevaPagina->tiempo_uso = obtener_tiempo();
	nuevaPagina->bitDeUso = 1;
	nuevaPagina->bitDeModificado = 1;

	ocupar_frame(numFrameParaHeap);

	list_add(tpCarpincho->paginas,nuevaPagina);


/*
	bool partido = false;
	int resto = 0;
	HeapMetadata* heapRecuperado = heap_correspondiente(9 + 50 + sizeof(HeapMetadata),tpCarpincho->pid,&partido,&resto);

	if(partido)
		free(heapRecuperado);
*/

	if(!send_reserva_pagina_swamp(socketSwamp,tpCarpincho->pid,nuevaPagina->indice)){

	}



	memcpy(posicionPrincipioHeapPartido1, heapMetadataNuevo, sizeHastaFinalPagina);

	memcpy(posicionPrincipioHeapPartido2, heapMetadataNuevo + sizeHastaFinalPagina, sizeof(HeapMetadata) - sizeHastaFinalPagina);


	free(heapMetadataNuevo);

/*
	HeapMetadata* heap = malloc(sizeof(HeapMetadata));
	memcpy(heap,posicionPrincipioHeapPartido1,sizeHastaFinalPagina);
	memcpy(heap + sizeHastaFinalPagina,posicionPrincipioHeapPartido2,sizeof(HeapMetadata) - sizeHastaFinalPagina);

	free(heap);
*/

	//semaforo??? //TODO
	//actualizar las paginas
	//pagina->bytesDisponibles -= sizeHastaFinalPagina;
}

void crear_heap_partido2(tpCarpincho* tpCarpincho, uint32_t direcLogicaUltimoHeap, t_info_pagina* pagina, uint32_t direcLogicaHeapPartido, int numFrameParaHeap){

    HeapMetadata* heapMetadataNuevo = malloc(sizeof(HeapMetadata));
    heapMetadataNuevo->prevAlloc = direcLogicaUltimoHeap;
    heapMetadataNuevo->nextAlloc = (uint32_t) NULL;
    heapMetadataNuevo->isFree = true;

    t_info_pagina* nuevaPagina = malloc(sizeof(*nuevaPagina));
    nuevaPagina->bitPresencia = 1;
    nuevaPagina->frame = numFrameParaHeap;
    nuevaPagina->indice = nuevo_indice_pagina(tpCarpincho->pid);
    nuevaPagina->tiempo_uso = obtener_tiempo();
    nuevaPagina->bitDeUso = 1;
    nuevaPagina->bitDeModificado = 1;

    ocupar_frame(numFrameParaHeap);

    list_add(tpCarpincho->paginas,nuevaPagina);

    if(!send_reserva_pagina_swamp(socketSwamp,tpCarpincho->pid,nuevaPagina->indice)){

    }

    HeapMetadataPartido* heapMetadataPartido = malloc(sizeof(HeapMetadataPartido));
    heapMetadataPartido->direcLogicaHeap = direcLogicaHeapPartido;
    heapMetadataPartido->heapPartido = heapMetadataNuevo;
    list_add(tpCarpincho->heapsPartidos, heapMetadataPartido);

}

int buscar_frame_tlb(int numeroPagina, uint32_t pid){

	if(cfg->cantEntradasTLB == 0){
		return -1;
	}

	bool TLB_tiene_pag_de_pid(void* elem){
		return ((TLB*) elem)->numPagina == numeroPagina &&  ((TLB*) elem)->pid == pid;
	}
	TLB* entradaTLB = list_find(listaTLB, &TLB_tiene_pag_de_pid);

	uint32_t* pidAux = malloc(sizeof(uint32_t));
	memcpy(pidAux,&pid,sizeof(uint32_t));

	if(entradaTLB != NULL){

		entradaTLB->tiempo_uso = obtener_tiempo_TLB();
		log_info(logger,"TLB Hit-PID: %d|PAGINA: %d|FRAME: %d", pid, numeroPagina, entradaTLB->frame);
		cantTLBhits++;
		sumar_TLBhit_a_pid(pid);
		free(pidAux);
		usleep(cfg->retardoAciertoTLB * 1000);
		return entradaTLB->frame;
	}

	log_info(logger,"TLB Miss-PID: %d|PAGINA: %d", pid, numeroPagina);
	cantTLBmiss++;
	sumar_TLBmiss_a_pid(pid);
	free(pidAux);
	usleep(cfg->retardoFalloTLB * 1000);
	return -1;
}

void actualizarTLB(uint32_t pid,int numeroPagina, int frame){

	if(cfg->cantEntradasTLB == 0){
		return;
	}

	bool existe_entrada(void* elem){
		return ((TLB*) elem)->numPagina == numeroPagina &&  ((TLB*) elem)->pid == pid;
	}

	TLB* entrada = list_find(listaTLB, &existe_entrada);
	if(entrada != NULL){
		entrada->frame = frame;
		return;
	}

	TLB* nuevaEntrada = malloc(sizeof(TLB));
	nuevaEntrada->pid = pid;
	nuevaEntrada->numPagina = numeroPagina;
	nuevaEntrada->frame = frame;
	nuevaEntrada->tiempo_uso = obtener_tiempo_TLB();
	if(list_size(listaTLB) < cfg->cantEntradasTLB){

		log_info(logger, "N-ENTRADA-PID: %d|PAGINA: %d|FRAME: %d",pid,numeroPagina,frame);

		list_add(listaTLB, nuevaEntrada);
	}else{

	TLB* entradaARemover = entrada_a_remover();

	bool es_la_entrada(void* elem){
		return ((TLB*) elem) == entradaARemover;
	}

	log_info(logger, "REEMPLAZO ENTRADA TLB");
	log_info(logger, "VICTIMA-PID: %d|PAGINA: %d|FRAME: %d",entradaARemover->pid,entradaARemover->numPagina,entradaARemover->frame);

	list_remove_and_destroy_by_condition(listaTLB,  &es_la_entrada, &free);
	list_add(listaTLB, nuevaEntrada);

	log_info(logger, "N-ENTRADA-PID: %d|PAGINA: %d|FRAME: %d",pid,numeroPagina,frame);

	}

}

uint32_t min(uint32_t a, uint32_t b) {
	return (a < b) ? a : b;
}

TLB* entrada_a_remover(){
	TLB* entradaARemover;


	if(!cfg->esFifo){

	TLB* LRU(TLB* TLB1, TLB* TLB2)
		{
			if(TLB2->tiempo_uso > TLB1->tiempo_uso) return TLB1;
			else return TLB2;
		}

	entradaARemover = list_get_minimum(listaTLB,(void*) LRU);
	return entradaARemover;

	}else{
		entradaARemover = list_get(listaTLB, 0);
		return entradaARemover;
	}

	return entradaARemover;
}

void sumar_TLBhit_a_pid(uint32_t pid){
	HitMiss* HitMissEncontrada = buscar_pid_listaHitMiss(pid);

	if(HitMissEncontrada == NULL){

		crear_nueva_HitMiss(pid);

		HitMissEncontrada = buscar_pid_listaHitMiss(pid);
	}

	HitMissEncontrada->cantHits += 1;

}

void sumar_TLBmiss_a_pid(uint32_t pid){
	HitMiss* HitMissEncontrada = buscar_pid_listaHitMiss(pid);

	if(HitMissEncontrada == NULL){

		crear_nueva_HitMiss(pid);

		HitMissEncontrada = buscar_pid_listaHitMiss(pid);
	}

	HitMissEncontrada->cantMiss += 1;

}

void crear_nueva_HitMiss(uint32_t pid){
	HitMiss* HitMissNuevo = malloc(sizeof(HitMiss));
	HitMissNuevo->pid = pid;
	HitMissNuevo->cantHits = 0;
	HitMissNuevo->cantMiss = 0;

	list_add(listaHitMiss,HitMissNuevo);
}

HitMiss* buscar_pid_listaHitMiss(uint32_t pid){

	bool tiene_pid(void* x) {

		if(x == NULL){

			return false;
		}else{
			HitMiss* HitMiss  = x;
		    return HitMiss->pid == pid;
		}
	}

	HitMiss* HitMiss = list_find(listaHitMiss, &tiene_pid);

	return HitMiss;
}

void limpiar_TLB(){
	list_clean_and_destroy_elements(listaTLB,&free);
	printf("Se limpiaron las entradas de la TLB \n");
}

void imprimir_metricas(){
	printf("\n");
	printf("CANTIDAD HITS TOTALES: %d \n", cantTLBhits);
	printf("CANTIDAD HITS POR CARPINCHO \n");

	void imprimirHits(HitMiss* HitMiss) {

		printf("PID: %d, HITS: %d \n", HitMiss->pid, HitMiss->cantHits);
	}

	list_iterate(listaHitMiss, (void*) imprimirHits);


	printf("CANTIDAD MISS TOTALES: %d \n", cantTLBmiss);

	printf("CANTIDAD MISSES POR CARPINCHO \n");

	void imprimirMisses(HitMiss* HitMiss) {

		printf("PID: %d, MISSES: %d \n", HitMiss->pid, HitMiss->cantMiss);
	}

	list_iterate(listaHitMiss, (void*) imprimirMisses);

}

bool finalizar_proceso(uint32_t pid, bool* existeCarpincho){
	//destruir lista de paginas y sacarlo de la lista de tpCarpinchos
	tpCarpincho* tpCarpinchoPID = obtener_tpCarpincho_con_pid(pid);

	if(tpCarpinchoPID == NULL){
		*existeCarpincho = false;
		return false;
	}

	*existeCarpincho = true;

	if(list_size(tpCarpinchoPID->paginas) == 0){
		return true;
	}

	t_list_iterator* iteradorPaginas = list_iterator_create(tpCarpinchoPID->paginas);


	//vaciar frames
	while(list_iterator_has_next(iteradorPaginas)){
		t_info_pagina* pagActual = list_iterator_next(iteradorPaginas);

		if(pagActual->bitPresencia){
			vaciar_frame(pagActual->frame);
		}

	}

	list_destroy_and_destroy_elements(tpCarpinchoPID->paginas, free);

	bool tp_carpincho_tiene_pid(void* x) {

			if(x == NULL){

				return false;
			}else{
			tpCarpincho* elem  = x;
			    return elem->pid == pid;
			}
		}

	list_iterator_destroy(iteradorPaginas);
	list_remove_and_destroy_by_condition(listaTablasCarpinchos, &tp_carpincho_tiene_pid, free);



	return true;
}

bool dump_tlb(char* carpeta, t_list* listaTLB)
{
	char datetime[20], timestamp[16];
	time_t now = time(NULL);
	strftime(datetime, 20, "%d/%m/%Y %H:%M:%S", localtime(&now));
	strftime(timestamp, 16, "%Y%m%d-%H%M%S", localtime(&now));

	char path[100];
	sprintf(path,"%s/Dump_%s.tlb", carpeta, timestamp);

	FILE* dump = fopen(path , "w");
	if(dump == NULL) return false;

	for (int i = 0; i < 75; i++)
		fprintf(dump, "-");
	fprintf(dump, "\n");

	fprintf(dump, "Dump: %s\n", datetime);

	for (int i = 0; i < listaTLB->elements_count; i++)
	{
		TLB* tlb = list_get(listaTLB, i);
		fprintf(dump, "Entrada: %d \t", i);
		fprintf(dump, "Estado: Ocupado \t");
		fprintf(dump, "Carpincho: %d \t", tlb->pid);
		fprintf(dump, "Pagina: %d \t", tlb->numPagina);
		fprintf(dump, "Marco: %d \t", tlb->frame);
		fprintf(dump, "\n");
	}

	int cantEntradasLibres = cfg->cantEntradasTLB - listaTLB->elements_count;

	fprintf(dump, "Hay %d de entradas libres \n", cantEntradasLibres);

	for (int i = 0; i < 75; i++)
		fprintf(dump, "-");
	fprintf(dump, "\n");

	fclose(dump);
	return true;
}

void imprimir_estado_frames(){


	for(int i = 0; i < cantidadFramesTotal; i ++){
		tpCarpincho* tpCarpincho = tpCarpincho_con_num_frame(i);
		if(tpCarpincho == NULL){
			continue;
		}

		t_info_pagina* pagina = t_info_pagina_con_num_frame(i);
		printf("FRAME: %d | C%dP%d | AC: %d | M: %d\n", i, tpCarpincho->pid, pagina->indice, pagina->tiempo_uso, pagina->bitDeModificado);

	}

}

int liberar_frame_para_carpincho(uint32_t pid){
	t_info_pagina* pagina = pagina_a_reemplazar(pid);
	tpCarpincho* carpincho = tpCarpincho_con_num_frame(pagina->frame);
	pagina->bitPresencia = 0;
	int bitM = pagina->bitDeModificado;
	//TODO CAPAZ HAY QUE PODIFICAR MAS COSAS DE LA PAGINA
	send_pag_swamp(socketSwamp, pagina, carpincho->pid);

	if(bitM != 0){
		MEM_SWAP_MESSAGE respuesta;
		recv(socketSwamp, &respuesta, sizeof(MEM_SWAP_MESSAGE), 0);

		if(respuesta != PAGE_ADDED){
			printf("No se pudo enviar la pagina %d del pid %d a swamp", pagina->indice, carpincho->pid);
		}
	}

	vaciar_frame(pagina->frame);

	return pagina->frame;
}


void eliminar_entradas_tlb_pid(uint32_t pid, int numeroPagina){

	bool existe_entrada(void* elem){
			return ((TLB*) elem)->numPagina == numeroPagina &&  ((TLB*) elem)->pid == pid;
		}


	list_remove_and_destroy_by_condition(listaTLB, &existe_entrada, free);
}


