/*
 * planificacion.c
 *
 *  Created on: 21 sep. 2021
 *      Author: utnso
 */

#include "planificacion.h"


// ---------------------------CONFIG-----------------------------------------------------------------------
t_algoritmo_planificacion obtener_algoritmo(){

	 t_algoritmo_planificacion switcher;
	 char* algoritmo = obtener_de_config(config, "ALGORITMO_PLANIFICACION");

	    //HRRN
	    if (strcmp(algoritmo,"HRRN") == 0)
	    {
	        switcher = HRRN;
	        log_info(logger, "El algoritmo de planificacion elegido es HRRN.");
	    }

	    //SFJ SIN DESALOJO
	    if (strcmp(algoritmo,"SJF") == 0)
	    {
	        switcher = SJF;
	        log_info(logger, "El algoritmo de planificacion elegido es SJF.");
	    }
	    return switcher;
}

// -----------------------COLAS / LISTAS-------------------------------------------------------------------

void agregarAPotencialesRetensores(pcb_carpincho* pcb){

	pthread_mutex_lock(&mutexPotencialesRetensores);

	list_add(listaPotencialesRetensores, pcb);
	log_info(logger, "[POTENCIALES RETENSORES DE SEMAFOROS] Entra el carpincho de PID: %d a la lista.", pcb->carpinchoPID);

	pthread_mutex_unlock(&mutexPotencialesRetensores);
}

void sacarDePotencialesRetensores(pcb_carpincho* pcb){

	bool tienenMismoPID(void* elemento){

		if(pcb->carpinchoPID == ((pcb_carpincho *) elemento)->carpinchoPID)
			return true;
		else
			return false;
	}

	pthread_mutex_lock(&mutexPotencialesRetensores);

	list_remove_by_condition(listaPotencialesRetensores, tienenMismoPID);
	log_info(logger, "[POTENCIALES RETENSORES DE SEMAFOROS] Sale el carpincho de PID: %d de la lista.", pcb->carpinchoPID);

	pthread_mutex_unlock(&mutexPotencialesRetensores);
}

void agregarANew(pcb_carpincho* carpincho) {

	pthread_mutex_lock(&mutexNew);

	queue_push(colaNew, carpincho);
	log_info(logger, "[NEW] Entra el carpincho de PID: %d a la cola.", carpincho->carpinchoPID);

	pthread_mutex_unlock(&mutexNew);

	sem_post(&analizarSuspension); // Despierta al planificador de mediano plazo
	sem_wait(&suspensionFinalizada); // Espera a que ya se haya hecho, o no, la suspension

	sem_post(&contadorNew); // Despierta al planificador de largo plazo
	sem_post(&largoPlazo);
}

pcb_carpincho* sacarDeNew(){

	sem_wait(&contadorNew);
	pthread_mutex_lock(&mutexNew);

	pcb_carpincho* carpincho = queue_pop(colaNew);
	log_info(logger, "[NEW] Se saca el carpincho de PID: %d de la cola", carpincho->carpinchoPID);

	pthread_mutex_unlock(&mutexNew);

	return carpincho;
}

void agregarAReady(pcb_carpincho* carpincho){

	time_t a = time(NULL);
	carpincho->horaDeIngresoAReady = ((float) a)*1000;
	carpincho->tiempoEspera = 0;
	//sem_wait(&multiprogramacion); Lo sacamos de aca para usarlo en el contexto en el que se llama a la funcion, porque no siempre que se agrega a ready, se toca la multiprogramacion
	pthread_mutex_lock(&mutexReady);

	carpincho->suspendido = false;
	list_add(colaReady, carpincho);
	log_info(logger, "[READY] Entra el carpincho de PID: %d a la cola.", carpincho->carpinchoPID);

	pthread_mutex_unlock(&mutexReady);
	sem_post(&contadorReady);
	//sem_post(&contadorProcesosEnMemoria); Lo sacamos de aca para usarlo en el contexto en el que se llama a la funcion, porque no siempre que se agrega a ready, se toca la multiprogramacion
}

void agregarABlock(pcb_carpincho* carpincho){		//ver semaforos

	sem_wait(&contadorExe);

	bool tienenMismoPID(void* elemento){

		if(carpincho->carpinchoPID == ((pcb_carpincho *) elemento)->carpinchoPID)
			return true;
		else
			return false;
	}

	list_remove_by_condition(listaExe, tienenMismoPID);

	pthread_mutex_lock(&mutexBlock);

	list_add(listaBlock, carpincho);
	log_info(logger, "[BLOCK] Entra el carpincho de PID: %d a la cola.", carpincho->carpinchoPID);

	pthread_mutex_unlock(&mutexBlock);
	sem_post(&multiprocesamiento);
	sem_post(&contadorBlock);

	sem_post(&analizarSuspension);
	sem_wait(&suspensionFinalizada);
}

void sacarDeBlock(pcb_carpincho* carpincho){

	sem_wait(&contadorBlock);

	bool tienenMismoPID(void* elemento){

		if(carpincho->carpinchoPID == ((pcb_carpincho *) elemento)->carpinchoPID)
			return true;
		else
			return false;
	}

	pthread_mutex_lock(&mutexBlock);

	list_remove_by_condition(listaBlock, tienenMismoPID);
	log_info(logger, "[BLOCK] Sale el carpincho de PID: %d de la cola.", carpincho->carpinchoPID);

	pthread_mutex_unlock(&mutexBlock);
}

void agregarABlockSuspended(pcb_carpincho* carpincho){

	pthread_mutex_lock(&mutexBlockSuspended);

	carpincho->suspendido = true;
	list_add(listaBlockSuspended, carpincho);

	log_info(logger, "[BLOCK-SUSPENDED] Ingresa el carpincho de PID: %d a la cola.", carpincho->carpinchoPID);

	pthread_mutex_unlock(&mutexBlockSuspended);

	size_t size = sizeof(sem_code)+sizeof(uint32_t);

	void* stream = malloc(size);

	sem_code opCode = SUSPEND;

	memcpy(stream, &opCode, sizeof(sem_code));
	memcpy(stream + sizeof(sem_code), &(carpincho->carpinchoPID), sizeof(uint32_t));

	send(carpincho->socketMemoria, stream, size, 0);

	free(stream);
}

void sacarDeBlockSuspended(pcb_carpincho* carpincho){

	bool tienenMismoPID(void* elemento){

	if(carpincho->carpinchoPID == ((pcb_carpincho *) elemento)->carpinchoPID)
		return true;
	else
		return false;
	}

	pthread_mutex_lock(&mutexBlockSuspended);

	list_remove_by_condition(listaBlockSuspended, tienenMismoPID);
	log_info(logger, "[BLOCK-SUSPENDED] Sale el carpincho de PID: %d de la cola.", carpincho->carpinchoPID);

	pthread_mutex_unlock(&mutexBlockSuspended);
}

void agregarAReadySuspended(pcb_carpincho* carpincho){

	pthread_mutex_lock(&mutexReadySuspended);

	queue_push(colaReadySuspended, carpincho);
	log_info(logger, "[READY-SUSPENDED] Ingresa el carpincho de PID: %d de la cola.", carpincho->carpinchoPID);

	pthread_mutex_unlock(&mutexReadySuspended);

	sem_post(&contadorReadySuspended);
	sem_post(&medianoPlazo);
}

pcb_carpincho* sacarDeReadySuspended(){

	sem_wait(&contadorReadySuspended);

	pthread_mutex_lock(&mutexReadySuspended);

	pcb_carpincho* carpincho = queue_pop(colaReadySuspended);
	carpincho->suspendido = false;
	log_info(logger, "[READY-SUSPENDED] Sale el carpincho de PID: %d de la cola.", carpincho->carpinchoPID);

	pthread_mutex_unlock(&mutexReadySuspended);

	return carpincho;
}

// ----------------------------------HILOS---------------------------------------------------

// Hilo que maneja pasar los procesos de new a ready	-	CASO FIFO
void hiloNew_Ready(){

	while(1){

		sem_wait(&largoPlazo);

		if(queue_size(colaReadySuspended) != 0){

			sem_post(&medianoPlazo);
		}else{

			pcb_carpincho* carpincho = sacarDeNew();

			//carpincho->rafagaAnterior = 0;
			carpincho->estimacionAnterior = estimacionInicial;
			carpincho->estimacionActual = estimacionInicial;	//"estimacio_inicial" va a ser una variable que vamos a obtener del cfg
			carpincho->tiempoEspera = 0;

			sem_wait(&multiprogramacion);
			agregarAPotencialesRetensores(carpincho);
			agregarAReady(carpincho);
			sem_post(&contadorProcesosEnMemoria);
		}
	}
}

// Hilo que maneja los procesos de Ready a Execute     -    CASO SJF/HRRN
void hiloReady_Exe(){

	while(1){

		sem_wait(&multiprocesamiento);

		pcb_carpincho* carpinchoAEjecutar = obtenerSiguienteDeReady();

		// Aca se crea un hilo de cpu y se le pasa ese pcb, cuando el carpincho hace mate_close se pasa el pcb a EXIT y se mata el hilo

		if(carpinchoAEjecutar != NULL) {

			pthread_mutex_lock(&mutexExe);
			list_add(listaExe, carpinchoAEjecutar);
			pthread_mutex_unlock(&mutexExe);

			pthread_t hiloCPU;
			pthread_create(&hiloCPU, NULL, (void*) ejecutar, (void*) carpinchoAEjecutar);
			pthread_detach(hiloCPU);

			if(algoritmoPlanificacion == SJF){
				log_info(logger, "[EXEC] Ingresa el carpincho de PID: %d con una rafaga de ejecucion estimada de %f milisegundos.", carpinchoAEjecutar->carpinchoPID, carpinchoAEjecutar->estimacionActual);
			}else{
				log_info(logger, "[EXEC] Ingresa el carpincho de PID: %d con una rafaga de ejecucion estimada de %f milisegundos y una espera de %f milisegundos.", carpinchoAEjecutar->carpinchoPID, carpinchoAEjecutar->estimacionActual, carpinchoAEjecutar->tiempoEspera);
			}

			sem_post(&contadorExe);

			sem_post(&analizarSuspension); // Despues de que un carpincho se va de Ready y hace su transicion, se analiza la suspension
			sem_wait(&suspensionFinalizada);

		}else{
			sem_post(&multiprocesamiento);
		}
	}
}

// Hilo que maneja la suspension de procesos
void hiloBlockASuspension(){

	while(true){

		sem_wait(&analizarSuspension);

		if(condiciones_de_suspension()){

			sem_wait(&contadorProcesosEnMemoria);

			pcb_carpincho* pcb = list_get(listaBlock, list_size(listaBlock) - 1);
			sacarDeBlock(pcb);

			agregarABlockSuspended(pcb);

			sem_post(&multiprogramacion);
		}

		sem_post(&suspensionFinalizada);
	}
}

void hiloSuspensionAReady(){

	while(1){

		sem_wait(&medianoPlazo);

		if(queue_size(colaReadySuspended) == 0){

			sem_post(&largoPlazo);
		}else{

		pcb_carpincho* carpincho = sacarDeReadySuspended();

		sem_wait(&multiprogramacion);

		agregarAReady(carpincho);

		sem_post(&contadorProcesosEnMemoria);
		}
	}
}

bool condiciones_de_suspension(){

	int cantidadDeProcesosEnMemoria;

	sem_getvalue(&contadorProcesosEnMemoria, &cantidadDeProcesosEnMemoria);

	pthread_mutex_lock(&mutexBlock);
	pthread_mutex_lock(&mutexReady);
	pthread_mutex_lock(&mutexNew);

	bool respuesta = (list_size(listaBlock) != 0 &&
			list_size(colaReady) == 0 &&
			queue_size(colaNew) != 0 &&
			cantidadDeProcesosEnMemoria == gradoMultiprogramacion);

	pthread_mutex_unlock(&mutexNew);
	pthread_mutex_unlock(&mutexReady);
	pthread_mutex_unlock(&mutexBlock);

	return respuesta;
}

void ejecutar(void* carpincho){

	pcb_carpincho* pcb = (pcb_carpincho *) carpincho;

	time_t a = time(NULL);
	pcb->horaDeIngresoAExe = ((float) a)*1000;

	t_semaforo sem;
	char* nombreDispositivo;
	char* mensaje;
	uint32_t direccionLogica;
	int size;
	void* buffer = NULL;

	uint32_t pidDummy;

	op_code cop;

	while(pcb->socketCarpincho != -1){


		sem_code autorizacion = CONTINUE;
		send(pcb->socketCarpincho, &autorizacion, sizeof(sem_code), 0);

		//printf("ESPERANDO CODIGO DE OPERACION DE CARPINCHO DE PID %d\n", pcb->carpinchoPID);

		if(recv(pcb->socketCarpincho, &cop, sizeof(op_code), 0) != sizeof(op_code)){
			log_error(logger, "Error al recibir codigo de operacion de PID %d", pcb->carpinchoPID);
			continue;
		}

		switch(cop){

		case SEM_INIT:{

			if(!recv_sem_init(pcb)){
				log_error(logger, "Fallo recibiendo mate_sem_init");
				break;
			}

			break;
		}

		case SEM_WAIT:{

			if(!recv_sem_wait((pcb), &(sem.nombre))){
				log_error(logger, "Fallo recibiendo mate_sem_wait");
				break;
			}

			break;
		}

		case SEM_POST:{

			if(!recv_sem_post(pcb, &(sem.nombre))){
				log_error(logger, "Fallo recibiendo mate_sem_post");
				break;
			}

			break;
		}

		case SEM_DESTROY:{

			if(!recv_sem_destroy(pcb, &(sem.nombre))){
				log_error(logger, "Fallo recibiendo mate_sem_destroy");
				break;
			}

			break;
		}

		case CALL_IO:{

			if(!recv_call_io(pcb, &(nombreDispositivo), &mensaje)){
				log_error(logger, "Fallo recibiendo call_io");
				break;
			}

			break;
		}

		case MEMALLOC:{

			if(!recv_memalloc(pcb->socketCarpincho, &size, &pidDummy)){
				log_error(logger, "Fallo recibiendo memalloc del carpincho");
				break;
			}

			if(!send_memalloc(pcb->socketMemoria, size, pcb->carpinchoPID)){
				log_error(logger, "Fallo enviando memalloc a memoria");
				break;
			}

			if(!recv_direcLogica(pcb->socketMemoria, &direccionLogica)){
				log_error(logger, "Fallo recibiendo direccion logica de memoria");
				break;
			}

			if(!send_direcLogica(pcb->socketCarpincho, direccionLogica)){
				log_error(logger, "Fallo enviando direccion logica al carpincho");
				break;
			}

			break;
		}

		case MEMFREE:{

			if(!recv_memfree(pcb->socketCarpincho, &direccionLogica, &pidDummy)){
				log_error(logger, "Fallo recibiendo memfree de carpincho");
				break;
			}

			if(!send_memfree(pcb->socketMemoria, direccionLogica, pcb->carpinchoPID)){
				log_error(logger, "Fallo enviando memfree a memoria");
				break;
			}

			int respuesta;

			if(recv(pcb->socketMemoria, &respuesta, sizeof(int), 0)!= sizeof(int)){
				log_error(logger, "Fallo recibiendo codigo respuesta de lectura de memoria");
				break;
			}

			if(send(pcb->socketCarpincho, &respuesta, sizeof(int), 0)!= sizeof(int)){
				log_error(logger, "Fallo enviando codigo respuesta de memfree al carpincho");
				break;
			}

			break;
		}

		case MEMREAD:{

			if(!recv_memread(pcb->socketCarpincho, &direccionLogica, &size,  &((pcb->carpinchoPID)))){
				log_error(logger, "Fallo recibiendo memread de carpincho");
				break;
			}

			if(!send_memread(pcb->socketMemoria, direccionLogica, size, pcb->carpinchoPID)){
				log_error(logger, "Fallo enviando memread a memoria");
				break;
			}

			int respuesta;

			if(recv(pcb->socketMemoria, &respuesta, sizeof(int), 0)!= sizeof(int)){
				log_error(logger, "Fallo recibiendo codigo respuesta de lectura de memoria");
				break;
			}

			send(pcb->socketCarpincho, &respuesta, sizeof(int), 0);

			if(respuesta == -6){
				break;
			}
			void* bufferAux = malloc(size);

			if(!recv_contenido_memread(pcb->socketMemoria, &bufferAux, &size)){
				log_error(logger, "Fallo recibiendo contenido de memread de memoria");
				free(bufferAux);
				break;
			}



			if(!send_contenido_memread(pcb->socketCarpincho, bufferAux, size) ){
				log_error(logger, "Fallo enviando contenido de memread al carpincho");
				free(bufferAux);
				break;
			}

			free(bufferAux);
			break;
		}

		case MEMWRITE:{

			if(!recv_memwrite(pcb->socketCarpincho, &direccionLogica, &buffer, &size, &pidDummy)){
				log_error(logger, "Fallo recibiendo memwrite de carpincho");
				break;
			}

			if(!send_memwrite(pcb->socketMemoria, direccionLogica, buffer, size, pcb->carpinchoPID)){
				log_error(logger, "Fallo enviando memwrite a memoria");
				break;
			}

			int respuesta;

			if(recv(pcb->socketMemoria, &respuesta, sizeof(int), 0)!= sizeof(int)){
				log_error(logger, "Fallo recibiendo respuesta bool de escritura de memoria");
				break;
			}

			if(send(pcb->socketCarpincho, &respuesta, sizeof(int), 0) != sizeof(int)){
				log_error(logger, "Fallo enviando respuesta bool de escritura al carpincho");
				break;
			}
			free(buffer);

			break;
		}

		case TERMINAR_EJECUCION:{

			// liberarSemaforosRetenidos

			sem_wait(&contadorExe);
			sem_wait(&contadorProcesosEnMemoria);
			pthread_mutex_lock(&mutexExe);

			int i;
			pcb_carpincho* carpinchoAux;
			for(i=0; i<list_size(listaExe); i++){
			    carpinchoAux = list_get(listaExe, i);
			   	if(carpinchoAux->carpinchoPID == pcb->carpinchoPID){
		 		break;
			 	}
			}
			list_remove(listaExe, i);

			pthread_mutex_unlock(&mutexExe);

			liberarSemaforosRetenidos(pcb);

			sem_code code = READY_TO_FINISH;
			send(pcb->socketCarpincho, &code, sizeof(sem_code), 0);

			sacarDePotencialesRetensores(pcb);
			terminarEjecucion(pcb);
			bool respuesta;
			recv_respuesta_bool(pcb->socketMemoria, &respuesta);
			/*if(!respuesta){
				log_warning(logger, "[EXIT] No se pudo terminar la ejecucion del carpincho de PID: %d en memoria", pcb->carpinchoPID);
			}*/
			close(pcb->socketMemoria);

			sem_post(&multiprocesamiento);
			sem_post(&multiprogramacion);


			pthread_exit(0);


			break;
		}

		default:
			break;
		}
	}
}

bool recv_call_io(pcb_carpincho* pcb, mate_io_resource* io, char** msg){

	size_t sizePayload;
	if(recv(pcb->socketCarpincho, &sizePayload, sizeof(size_t), 0) != sizeof(size_t))
		return false;

	void* stream = malloc(sizePayload);

	if(recv(pcb->socketCarpincho, stream, sizePayload, 0) != sizePayload){
		free(stream);
		return false;
	}

	deserializar_call_io(stream, io, msg);

	free(stream);

	bool tienenMismoNombre(void* elemento){

		if(strcmp(*io, ((t_io*) elemento)->io) == 0){
			return true;
		}else{
			return false;
		}
	}

	t_io* dispositivo = list_find(listaIO, tienenMismoNombre);

	if(dispositivo == NULL){

		sem_code resultado = NOT_FOUND;

		log_warning(logger, "[IO] El carpincho de PID: %d intenta realizar una E/S con el dispositivo inexistente %s", pcb->carpinchoPID, *io);

		if(send(pcb->socketCarpincho, &resultado, sizeof(sem_code), 0) != sizeof(sem_code)){
			log_warning(logger, "[IO] Hubo problemas al enviar la senial NOT_FOUND al carpincho de PID: %d por el dispositivo %s", pcb->carpinchoPID, *io);
		}

		free(*io);
		free(*msg);
	}else{

		free(*io);

		time_t a = time(NULL);
		float tiempoDeFin = ((float) a)*1000;
		pcb->rafagaAnterior = diferencia_de_tiempo(pcb->horaDeIngresoAExe, tiempoDeFin);
		pcb->estimacionActual = alfa*(pcb->rafagaAnterior) + (1-alfa)*(pcb->estimacionAnterior);
		pcb->estimacionAnterior = pcb->estimacionActual;

		informacionDeIO* info = malloc(sizeof(informacionDeIO));

		info->pcb = pcb;
		info->mensaje = *msg;

		pthread_mutex_lock(&(dispositivo->mutexDispositivo));

		queue_push(dispositivo->listaDeEspera, info);
		agregarABlock(pcb);
		sem_post(&(dispositivo->contadorEspera));

		pthread_mutex_unlock(&(dispositivo->mutexDispositivo));

		pthread_exit(0);
	}

	return true;
}

void funcionDispositivo(void* dispositivo){

	t_io* dispositivoIO = (t_io*) dispositivo;
	informacionDeIO* info;

	sem_code resultado = FOUND;

	while(true){

		sem_wait(&(dispositivoIO->contadorEspera));

		info = queue_pop(dispositivoIO->listaDeEspera);

		if(dispositivoIO->duracion == 0){
			usleep(1);
		}else{
			usleep((dispositivoIO->duracion)*1000);
		}

		log_info(logger, "[IO] El carpincho de PID: %d utiliza el dispositivo %s y loggea el mensaje: %s", info->pcb->carpinchoPID, dispositivoIO->io, info->mensaje);

		if(send(info->pcb->socketCarpincho, &resultado, sizeof(sem_code), 0) != sizeof(sem_code)){
			log_warning(logger, "[IO] Hubo problemas al enviar la senial FOUND al carpincho de PID: %d por el dispositivo %s", info->pcb->carpinchoPID, dispositivoIO->io);
		}

		if((info->pcb)->suspendido == true){

			sacarDeBlockSuspended((info->pcb));
			agregarAReadySuspended((info->pcb));
		}else{

			sacarDeBlock(info->pcb);
			agregarAReady(info->pcb);
		}

		free(info->mensaje);
		free(info);
	}
}

void terminarEjecucion(pcb_carpincho* pcb){

	pthread_mutex_lock(&mutexExit);

	list_add(listaExit, pcb);
	log_info(logger, "[EXIT] Finaliza el carpincho de PID: %d", pcb->carpinchoPID);

	pthread_mutex_unlock(&mutexExit);

	size_t size = sizeof(sem_code)+sizeof(uint32_t);

	void* stream = malloc(size);

	sem_code opCode = TERMINAR_EJECUCION;

	memcpy(stream, &opCode, sizeof(sem_code));
	memcpy(stream + sizeof(sem_code), &(pcb->carpinchoPID), sizeof(uint32_t));

	send(pcb->socketMemoria, stream, size, 0);


	free(stream);
}

void liberarSemaforosRetenidos(pcb_carpincho* pcb){

	t_semaforo* semaforoALiberar;

	while(list_size(pcb->semaforosRetenidos) != 0){

		semaforoALiberar = list_get(pcb->semaforosRetenidos, 0);

		realizar_operacion_sem(pcb, POST, semaforoALiberar);
	}

	bool tienenMismoPID(void* elemento){

		if(pcb->carpinchoPID == ((pcb_carpincho *) elemento)->carpinchoPID)
			return true;
		else
			return false;
	}

		t_semaforo* semaforoEsperado = pcb->semaforoEsperado;
		if(semaforoEsperado != NULL){
			list_remove_by_condition(semaforoEsperado->listaDeEspera->elements, tienenMismoPID);
			semaforoEsperado->valor++;
			pcb->semaforoEsperado=NULL;
		}
}

void deteccionYRecuperacion(){

	while(1){

	if(intervaloDeadlock == 0){
		usleep(1);
	}else{
		usleep(intervaloDeadlock*1000);
	}

	t_list* carpinchosEnDeadlock = list_create();

	pthread_mutex_lock(&mutexListaSemaforos);
	pthread_mutex_lock(&mutexPotencialesRetensores);

	analizarDeadlock(carpinchosEnDeadlock);

	if(list_size(carpinchosEnDeadlock) != 0){

		bool tieneMayorPID(void* unPCB, void* otroPCB){
			return (((pcb_carpincho*) unPCB)->carpinchoPID) > (((pcb_carpincho*) otroPCB)->carpinchoPID);
		}

		list_sort(carpinchosEnDeadlock, tieneMayorPID);

		pcb_carpincho* carpinchoLoggeado;

		log_info(logger, "[DEADLOCK] Estan en deadlock los siguientes carpinchos: \n");

		for(int i=0; i<list_size(carpinchosEnDeadlock); i++){

			carpinchoLoggeado = list_get(carpinchosEnDeadlock, i);
			log_info(logger, "[DEADLOCK] El carpincho de PID: %d pertenece al deadlock.", carpinchoLoggeado->carpinchoPID);
		}

		pcb_carpincho* pcb = list_remove(carpinchosEnDeadlock, 0);

		liberarSemaforosRetenidos(pcb);

		pthread_mutex_unlock(&mutexPotencialesRetensores);
		pthread_mutex_unlock(&mutexListaSemaforos);

		if(pcb->suspendido == true){
			sacarDeBlockSuspended(pcb);
			agregarAReadySuspended(pcb);
		}else{
			sacarDeBlock(pcb);
			agregarAReady(pcb);
		}


		sem_code autorizacion = FINALIZE;
		send(pcb->socketCarpincho, &autorizacion, sizeof(sem_code), 0);

		//sacarDePotencialesRetensores(pcb);
		//terminarEjecucion(pcb);

		log_info(logger, "[DEADLOCK] Se resuelve el deadlock eliminando al carpincho de PID: %d.", pcb->carpinchoPID);

		//printf("Saraza resuelve deadlock\n");
	}else{

		//log_info(logger, "[DEADLOCK] No hay deadlock actualmente.");
		pthread_mutex_unlock(&mutexPotencialesRetensores);
		pthread_mutex_unlock(&mutexListaSemaforos);
	}

	while(list_size(carpinchosEnDeadlock) != 0){
		list_remove(carpinchosEnDeadlock, 0);
	}

	list_destroy(carpinchosEnDeadlock);

	}
}

void analizarDeadlock(t_list* carpinchosEnDeadlock){

	for(int i=0; i<list_size(listaPotencialesRetensores); i++){

	pcb_carpincho* potencialRetensor = list_get(listaPotencialesRetensores, i);
	t_semaforo* primerSemaforoEsperado;

	if((primerSemaforoEsperado = potencialRetensor->semaforoEsperado) != NULL && list_size(potencialRetensor->semaforosRetenidos) != 0){
	// Preguntamos si el primer PCB espera algun semaforo y retiene semaforos

		bool tienenMismoNombre(void* elemento){

				if(strcmp(primerSemaforoEsperado->nombre, ((t_semaforo *) elemento)->nombre) == 0){
					return true;
				}else{
					return false;
				}
		}

		for(int j=0; j<list_size(listaPotencialesRetensores); j++){

			pcb_carpincho* otroPCB = list_get(listaPotencialesRetensores, j);

			t_semaforo* unSemaforo = list_find(otroPCB->semaforosRetenidos, tienenMismoNombre);
			// unSemaforo apunta al mismo semaforo que primerSemaforoEsperado o apunta a NULL

			if((unSemaforo != NULL) && (otroPCB->semaforoEsperado != NULL) && (potencialRetensor->carpinchoPID != otroPCB->carpinchoPID)){
			// Preguntamos si el segundo PCB retiene al semaforo que espera el primer PCB, y si el segundo PCB espera algun semaforo
			// Lo de PID distinto es por el caso en el que un PCB retenga el mismo semaforo que espera, puede que le haga un wait y pase, y le haga otro wait y se bloquee

				bool tienenMismoNombre2(void* elemento){

					if(strcmp(otroPCB->semaforoEsperado->nombre, ((t_semaforo *) elemento)->nombre) == 0){
						return true;
					}else{
						return false;
					}
				}

				t_semaforo* otroSemaforo = list_find(potencialRetensor->semaforosRetenidos, tienenMismoNombre2);
				// otroSemaforo apunta al semaforo que espera el segundo PCB o apunta a NULL, preguntamos si el primer PCB retiene al semaforo que quiere el segundo PCB
				// Aca se busca la arista que cierra el ciclo entre 2 PCBs

				if(otroSemaforo != NULL){
				// Preguntamos si el semaforo que espera el segundo PCB esta retenido por el primer PCB

					list_add(carpinchosEnDeadlock, potencialRetensor);
					list_add(carpinchosEnDeadlock, otroPCB);

					return;
				}else{
					//printf("Saraza va a empezar casoRecursivo\n");
					pcb_carpincho* tercerPCB = casoRecursivo(carpinchosEnDeadlock, potencialRetensor, otroPCB);

					if(tercerPCB != NULL){
						list_add(carpinchosEnDeadlock, tercerPCB);
						list_add(carpinchosEnDeadlock, otroPCB);
						list_add(carpinchosEnDeadlock, potencialRetensor);

						return;
					}else{
						continue;
					}
				}
			}else{
				continue;
			}
		}
	}else{
		continue;
	}

	}
}

pcb_carpincho* casoRecursivo(t_list* carpinchosEnDeadlock, pcb_carpincho* potencialRetensor, pcb_carpincho* otroPCB){

	//printf("Saraza empezo casoRecursivo\n");

	bool tienenMismoNombreNMenos1(void* elemento){

		if(strcmp(otroPCB->semaforoEsperado->nombre, ((t_semaforo *) elemento)->nombre) == 0){
			return true;
		}else{
			return false;
		}
	}

	pcb_carpincho* tercerPCB;
	pcb_carpincho* ultimoPCB;

	for(int k=0; k<list_size(listaPotencialesRetensores); k++){

		tercerPCB = list_get(listaPotencialesRetensores, k);

		t_semaforo* semaforoEsperadoPorSegundoPCB = list_find(tercerPCB->semaforosRetenidos, tienenMismoNombreNMenos1);

			bool tienenMismoNombreN(void* elemento){

				if(tercerPCB->semaforoEsperado != NULL){
					if(strcmp(tercerPCB->semaforoEsperado->nombre, ((t_semaforo *) elemento)->nombre) == 0){
						return true;
					}else{
						return false;
					}
				}else{
					return false;
				}
			}

		t_semaforo* cuartoSemaforo = list_find(potencialRetensor->semaforosRetenidos, tienenMismoNombreN);

		if(semaforoEsperadoPorSegundoPCB != NULL && tercerPCB->semaforoEsperado != NULL && otroPCB->carpinchoPID != tercerPCB->carpinchoPID){

			if(cuartoSemaforo != NULL){

				return tercerPCB; // En este caso hay deadlock, caso de corte
			}else{
				if(tercerPCB->semaforoEsperado == NULL){

					continue;
				}else{

					ultimoPCB = casoRecursivo(carpinchosEnDeadlock, potencialRetensor, tercerPCB);
					//printf("Saraza casoRecursivo RETORNA \n"); // Esto nunca lo printea
					if(ultimoPCB != NULL){
						list_add(carpinchosEnDeadlock, ultimoPCB);
						return tercerPCB;
					}else{
						return NULL;
					}
				}
			}
		}else{
			continue;
		}
	}

	return NULL;
}

pcb_carpincho* obtenerSiguienteDeReady(){

	sem_wait(&contadorReady);

	pcb_carpincho* carpinchoPlanificado = NULL;

	int tamanioDeColaReady(){

		int tamanio;

		pthread_mutex_lock(&mutexReady);
		tamanio = list_size(colaReady);
		pthread_mutex_unlock(&mutexReady);

		return tamanio;
	}

	if (tamanioDeColaReady() > 0){

		// Aca dentro un SWITCH para los distintos algoritmos q llama a una funcion para cada uno
	  switch(algoritmoPlanificacion){

		//CASO HRRN
		case HRRN:
			carpinchoPlanificado = obtenerSiguienteHRRN();
		break;

		//CASO SJF sin desalojo
		case SJF:
			carpinchoPlanificado = obtenerSiguienteSJF();
		break;

	  }
	}

	// Devuelve NULL si no hay nada en ready
	// Caso contrario devuelve el que tiene mas prioridad segun el algoritmo que se este empleando
	return carpinchoPlanificado;
}

pcb_carpincho* obtenerSiguienteHRRN(){

	pcb_carpincho* carpinchoPlanificado = NULL;
	pcb_carpincho* carpinchoAux = NULL;
    int i;
    float elMejorResponseRatio;
    float responseRatioAux;
	int indexARemover;

	//sem_wait(mutexReady);

	pthread_mutex_lock(&mutexReady);
	carpinchoAux = list_get(colaReady,0);
	actualizarTiemposDeEspera();
	pthread_mutex_unlock(&mutexReady);

	//Uso a uno como auxiliar que va a ser el que tiene mejor response ratio
	elMejorResponseRatio = (carpinchoAux->tiempoEspera + carpinchoAux->estimacionActual) / carpinchoAux->estimacionActual;	// HAY QUE SACAR DE CONFIG ESTO (ARMAS FUNCION)

	indexARemover = 0;

	//Itero buscando al carpincho con mayor response ratio, cada vez que encuentro uno que supere al mejor,
	//piso el valor ancla (indice del carpincho en la lista ready)
	//que sera removido para ser planificado

	//sem_wait(&contadorReady);
	pthread_mutex_lock(&mutexReady);

    for(i=1;i<list_size(colaReady);i++){
    	carpinchoAux = list_get(colaReady,i);
    	responseRatioAux = (carpinchoAux->tiempoEspera + carpinchoAux->estimacionActual) / carpinchoAux->estimacionActual;
    	//piso el valor ancla (indice del carpincho en la lista ready)
    	//que sera removido para ser planificado solo si el iterado supera al mejor
    	if(elMejorResponseRatio < responseRatioAux){
    		elMejorResponseRatio = responseRatioAux;
   		    indexARemover = i;
    	}

    }

    carpinchoPlanificado = list_remove(colaReady, indexARemover);

    pthread_mutex_unlock(&mutexReady);

	return carpinchoPlanificado;
}

void actualizarTiemposDeEspera(){

	pcb_carpincho* auxiliarDeTiempo;
	time_t a = time(NULL);
	float horaDeReplanificacion = ((float) a)*1000;

	for(int j=0;j<list_size(colaReady);j++){
		auxiliarDeTiempo = list_get(colaReady, j);
		auxiliarDeTiempo->tiempoEspera = diferencia_de_tiempo(auxiliarDeTiempo->horaDeIngresoAReady, horaDeReplanificacion);
	}
}

pcb_carpincho* obtenerSiguienteSJF(){

	pcb_carpincho* carpinchoPlanificado = NULL;
	pcb_carpincho* carpinchoAux = NULL;
    int i;
	int indexARemover;
	float shortestJob;

	pthread_mutex_lock(&mutexReady);
	carpinchoAux = list_get(colaReady,0);
	pthread_mutex_unlock(&mutexReady);

	indexARemover = 0;
	shortestJob = carpinchoAux->estimacionActual;

	//itero por la lista de Ready
	//sem_wait(&contadorReady);
	pthread_mutex_lock(&mutexReady);

	printf("CARPINCHOS EN READY: %d \n", list_size(colaReady));

    for(i=1;i<list_size(colaReady);i++){
    	carpinchoAux = list_get(colaReady,i);
    	//idem HRRN pero en vez de response ratio solo comparo las estimaciones de las rafagas a realizar
    	if(shortestJob > carpinchoAux->estimacionActual){
    		shortestJob = carpinchoAux->estimacionActual;
    		indexARemover = i;
    	}

    }

    carpinchoPlanificado = list_remove(colaReady, indexARemover);

    pthread_mutex_unlock(&mutexReady);

	return carpinchoPlanificado;
}

void realizar_operacion_sem(pcb_carpincho* pcb, sem_op tipoOP, t_semaforo* semaforo) {

	sem_code encontrado = FOUND;

	bool tienenMismoNombre(void* elemento){

		if(strcmp(semaforo->nombre, ((t_semaforo *) elemento)->nombre) == 0){
			return true;
		}else{
			return false;
		}
	}

	switch(tipoOP){

		case WAIT:{

			pthread_mutex_lock(&(semaforo->mutexSemaforo));

			semaforo->valor--;
			log_info(logger, "[SEM] El carpincho de PID: %d hace wait al semaforo %s que queda con valor %d", pcb->carpinchoPID, semaforo->nombre, semaforo->valor);

			if(send(pcb->socketCarpincho, &encontrado, sizeof(sem_code), 0) != sizeof(sem_code)){
				log_warning(logger, "[SEM] Hubo problemas al enviar la senial FOUND al carpincho de PID: %d por el semaforo %s", pcb->carpinchoPID, semaforo->nombre);
			}

			if(semaforo->valor < 0){

				time_t a = time(NULL);
				float tiempoDeFin = ((float) a)*1000;
				pcb->rafagaAnterior = diferencia_de_tiempo(pcb->horaDeIngresoAExe, tiempoDeFin);
				pcb->estimacionActual = alfa*(pcb->rafagaAnterior) + (1-alfa)*(pcb->estimacionAnterior);
				pcb->estimacionAnterior = pcb->estimacionActual;

				queue_push(semaforo->listaDeEspera, pcb);
				pcb->semaforoEsperado = semaforo;

				agregarABlock(pcb);
				pthread_mutex_unlock(&(semaforo->mutexSemaforo));
				pthread_exit(0);
			}

			list_add(pcb->semaforosRetenidos, semaforo);

			log_info(logger, "[SEM] El carpincho de PID: %d retiene al semaforo %s.", pcb->carpinchoPID, semaforo->nombre);

			pthread_mutex_unlock(&(semaforo->mutexSemaforo));

			sem_code autorizacion = CONTINUE;
			send(pcb->socketCarpincho, &autorizacion, sizeof(sem_code), 0);

			break;
		}

		case POST:{

			pthread_mutex_lock(&(semaforo->mutexSemaforo));

			semaforo->valor++;

			pcb_carpincho* carpinchoDesbloqueado = NULL;
			log_info(logger, "[SEM] El carpincho de PID: %d hace post al semaforo %s que queda con valor %d", pcb->carpinchoPID, semaforo->nombre, semaforo->valor);

			if(send(pcb->socketCarpincho, &encontrado, sizeof(sem_code), 0) != sizeof(sem_code)){
				log_warning(logger, "[SEM] Hubo problemas al enviar la senial FOUND al carpincho de PID: %d por el semaforo %s", pcb->carpinchoPID, semaforo->nombre);
			}

			if(semaforo->valor <= 0){

				carpinchoDesbloqueado = queue_pop(semaforo->listaDeEspera);
				carpinchoDesbloqueado->semaforoEsperado = NULL;
				list_add(carpinchoDesbloqueado->semaforosRetenidos, semaforo);

				log_info(logger, "[SEM] El carpincho de PID: %d retiene al semaforo %s tras desbloquearse.", carpinchoDesbloqueado->carpinchoPID, semaforo->nombre);

				sem_code autorizacion = CONTINUE;
				send(carpinchoDesbloqueado->socketCarpincho, &autorizacion, sizeof(sem_code), 0);

				if(carpinchoDesbloqueado->suspendido == true){

					sacarDeBlockSuspended(carpinchoDesbloqueado);
					agregarAReadySuspended(carpinchoDesbloqueado);
				}else{

					sacarDeBlock(carpinchoDesbloqueado);
					agregarAReady(carpinchoDesbloqueado);
				}
			}

			t_semaforo* semaforoLiberado = (t_semaforo*) list_remove_by_condition(pcb->semaforosRetenidos, tienenMismoNombre);

			if(semaforoLiberado != NULL){
				log_info(logger, "[SEM] El carpincho de PID: %d libera al semaforo que retenia: %s", pcb->carpinchoPID, semaforoLiberado->nombre);
			}

			pthread_mutex_unlock(&(semaforo->mutexSemaforo));

			break;
		}

		case DESTROY:{

			pthread_mutex_lock(&mutexListaSemaforos);

			log_info(logger, "[SEM] El carpincho de PID: %d destruye al semaforo %s con valor %d", pcb->carpinchoPID, semaforo->nombre, semaforo->valor);
			if(send(pcb->socketCarpincho, &encontrado, sizeof(sem_code), 0) != sizeof(sem_code)){
				log_warning(logger, "[SEM] Hubo problemas al enviar la senial FOUND al carpincho de PID: %d por el semaforo %s", pcb->carpinchoPID, semaforo->nombre);
			}

			list_remove_by_condition(listaSemaforos, tienenMismoNombre);

			pthread_mutex_unlock(&mutexListaSemaforos);

			pthread_mutex_lock(&(semaforo->mutexSemaforo));

			pcb_carpincho* pcbAuxiliar;

			while(semaforo->valor < 0){

				pcbAuxiliar = queue_peek(semaforo->listaDeEspera);
				realizar_operacion_sem(pcbAuxiliar, CLEAR, semaforo); // Aca se hace lock y ya hicimos lock antes del while, por eso se bloquea y no limpia la lista de espera del semaforo
			}

			pcb_carpincho* potencialRetensor;
			t_semaforo* semaforoAuxiliar;

			pthread_mutex_lock(&mutexPotencialesRetensores);

			for(int i=0; i<list_size(listaPotencialesRetensores); i++){

				potencialRetensor = list_get(listaPotencialesRetensores, i);

				while((semaforoAuxiliar = list_find(potencialRetensor->semaforosRetenidos, tienenMismoNombre)) != NULL){

					realizar_operacion_sem(potencialRetensor, ELIMINAR_RETENCION, semaforo);
				}

				log_info(logger, "[SEM] El carpincho de PID: %d libera al semaforo que retenia: %s", potencialRetensor->carpinchoPID, semaforo->nombre);
			}

			pthread_mutex_unlock(&mutexPotencialesRetensores);

			log_info(logger, "[SEM] Se destruyo al semaforo %s tras eliminar sus esperas y retenciones", semaforo->nombre);

			free(semaforo->nombre);
			destruirColaYElementos(semaforo->listaDeEspera);
			pthread_mutex_unlock(&(semaforo->mutexSemaforo));
			pthread_mutex_destroy(&(semaforo->mutexSemaforo));
			free(semaforo);

			break;
		}

		case CLEAR:{

			semaforo->valor++;
			pcb_carpincho* carpinchoDesbloqueado = NULL;
			log_info(logger, "[SEM] Se saca al carpincho de PID: %d del semaforo %s que queda con valor %d", pcb->carpinchoPID, semaforo->nombre, semaforo->valor);

			if(semaforo->valor <= 0){

				carpinchoDesbloqueado = queue_pop(semaforo->listaDeEspera);
				//list_add(carpinchoDesbloqueado->semaforosRetenidos, semaforo);

				//log_info(logger, "[SEM] El carpincho de PID: %d retiene al semaforo %s tras desbloquearse.", carpinchoDesbloqueado->carpinchoPID, semaforo->nombre);
				// Se saca porque como se va a destruir el semaforo, conceptualmente no lo retiene mas

				sem_code autorizacion = CONTINUE;
				send(carpinchoDesbloqueado->socketCarpincho, &autorizacion, sizeof(sem_code), 0);

				if(carpinchoDesbloqueado->suspendido == true){

					sacarDeBlockSuspended(carpinchoDesbloqueado);
					agregarAReadySuspended(carpinchoDesbloqueado);
				}else{

					sacarDeBlock(carpinchoDesbloqueado);
					agregarAReady(carpinchoDesbloqueado);
				}
			}

			t_semaforo* semaforoLiberado = (t_semaforo*) list_remove_by_condition(pcb->semaforosRetenidos, tienenMismoNombre);

			if(semaforoLiberado != NULL){
				    log_info(logger, "[SEM] El carpincho de PID: %d libera al semaforo que retenia: %s", pcb->carpinchoPID, semaforoLiberado->nombre);
			}

			break;
		}

		case ELIMINAR_RETENCION:{ // Este caso esta hecho exclusivamente para cuando se quieren eliminar las retenciones de un semaforo que ya se limpio su lista de espera y se va a destruir

			semaforo->valor++;

			list_remove_by_condition(pcb->semaforosRetenidos, tienenMismoNombre);

			log_info(logger, "[SEM] El carpincho de PID: %d elimina una instancia retenida del semaforo %s que queda con valor %d", pcb->carpinchoPID, semaforo->nombre, semaforo->valor);

			break;
		}
	}
}

void operacion_semaforo_creado(pcb_carpincho* pcb, mate_sem_name nombreSemaforo, sem_op tipoOP) {

	t_semaforo* semaforo;

	//strcpy(semaforo->nombre, nombreSemaforo);

	//size_t sizeNombre = strlen(nombreSemaforo) + 1;

	//memcpy(semaforo->nombre, nombreSemaforo, sizeNombre);

	bool tienenMismoNombre(void* elemento){

		if(strcmp(nombreSemaforo, ((t_semaforo *) elemento)->nombre) == 0){
			return true;
		}else{
			return false;
		}
	}

	pthread_mutex_lock(&mutexListaSemaforos);

	if((semaforo = list_find(listaSemaforos, tienenMismoNombre)) == NULL){

		pthread_mutex_unlock(&mutexListaSemaforos);

		sem_code noEncontrado = NOT_FOUND;

		switch(tipoOP){

		case WAIT:{
			log_warning(logger, "[SEM] El carpincho de PID: %d intenta hacer wait al semaforo inexistente %s.", pcb->carpinchoPID, nombreSemaforo);
			break;
		}

		case POST:{
			log_warning(logger, "[SEM] El carpincho de PID: %d intenta hacer post al semaforo inexistente %s.", pcb->carpinchoPID, nombreSemaforo);
			break;
		}

		case DESTROY:{
			log_warning(logger, "[SEM] El carpincho de PID: %d intenta destruir al semaforo inexistente %s.", pcb->carpinchoPID, nombreSemaforo);
			break;
		}

		default:
			break;
		}

		if(send(pcb->socketCarpincho, &noEncontrado, sizeof(sem_code), 0) != sizeof(sem_code)){
			log_warning(logger, "[SEM] Hubo problemas al enviar la senial NOT_FOUND al carpincho de PID: %d por el semaforo %s", pcb->carpinchoPID, nombreSemaforo);
		}

		free(nombreSemaforo);

	}else{

		pthread_mutex_unlock(&mutexListaSemaforos);
		free(nombreSemaforo);
		realizar_operacion_sem(pcb, tipoOP, semaforo);
	}
}

//---------------------RECV----------------------

bool recv_sem_init(pcb_carpincho* pcb){

	size_t sizePayload;
	unsigned int valorAuxiliar;

	if(recv(pcb->socketCarpincho, &sizePayload, sizeof(size_t),0) != sizeof(size_t))
		return false;

	void* stream = malloc(sizePayload);

	if(recv(pcb->socketCarpincho, stream, sizePayload, 0) != sizePayload){
		free(stream);
		return false;
	}

	t_semaforo* semaforo = malloc(sizeof(t_semaforo));

	deserializar_sem_init(stream, &(semaforo->nombre), &valorAuxiliar);

	free(stream);

// Aca se inicializa el semaforo y se lo agrega a la lista

	bool tienenMismoNombre(void* elemento){

	if(strcmp(semaforo->nombre, ((t_semaforo *) elemento)->nombre) == 0){
			return true;
	}else{
		return false;
		}
	}

	sem_code resultado;

	pthread_mutex_lock(&mutexListaSemaforos);

	if(list_find(listaSemaforos, tienenMismoNombre) == NULL){

		semaforo->valor = (int) valorAuxiliar;
		semaforo->listaDeEspera = queue_create();
		pthread_mutex_init(&(semaforo->mutexSemaforo), NULL);
		list_add(listaSemaforos, semaforo);
		log_info(logger, "[SEM] El carpincho de PID: %d inicializa el semaforo %s con valor %d", pcb->carpinchoPID, semaforo->nombre, semaforo->valor);

		resultado = NOT_FOUND;

		if(send(pcb->socketCarpincho, &resultado, sizeof(sem_code), 0) != sizeof(sem_code)){
			log_warning(logger, "[SEM] Hubo problemas al enviar la senial NOT_FOUND al carpincho de PID: %d por la inicializacion del semaforo %s", pcb->carpinchoPID, semaforo->nombre);
		}
	}else{

		log_warning(logger, "[SEM] El carpincho de PID: %d intenta inicializar el semaforo ya existente %s", pcb->carpinchoPID, semaforo->nombre);

		resultado = FOUND;

		if(send(pcb->socketCarpincho, &resultado, sizeof(sem_code), 0) != sizeof(sem_code)){
			log_warning(logger, "[SEM] Hubo problemas al enviar la senial FOUND al carpincho de PID: %d por el intento de inicializar del semaforo existente %s", pcb->carpinchoPID, semaforo->nombre);
		}

		free(semaforo);
	}

	pthread_mutex_unlock(&mutexListaSemaforos);

	return true;
}

bool recv_sem_wait(pcb_carpincho* pcb, mate_sem_name* nombreSemaforo){

	size_t sizePayload;
	if(recv(pcb->socketCarpincho, &sizePayload, sizeof(size_t), 0) != sizeof(size_t))
		return false;

	void* stream = malloc(sizePayload);

	if(recv(pcb->socketCarpincho, stream, sizePayload, 0) != sizePayload){
		free(stream);
		return false;
	}

	deserializar_sem_wait(stream, nombreSemaforo);

	free(stream);

	operacion_semaforo_creado(pcb, *nombreSemaforo, WAIT);

	return true;
}

bool recv_sem_post(pcb_carpincho* pcb, mate_sem_name* nombreSemaforo){

	size_t sizePayload;
	if(recv(pcb->socketCarpincho, &sizePayload, sizeof(size_t), 0) != sizeof(size_t))
		return false;

	void* stream = malloc(sizePayload);

	if(recv(pcb->socketCarpincho, stream, sizePayload, 0) != sizePayload){
		free(stream);
		return false;
	}

	deserializar_sem_post(stream, nombreSemaforo);

	free(stream);

	operacion_semaforo_creado(pcb, *nombreSemaforo, POST);

	return true;
}

bool recv_sem_destroy(pcb_carpincho* pcb, mate_sem_name* nombreSemaforo){

	size_t sizePayload;
	if(recv(pcb->socketCarpincho, &sizePayload, sizeof(size_t), 0) != sizeof(size_t))
		return false;

	void* stream = malloc(sizePayload);

	if(recv(pcb->socketCarpincho, stream, sizePayload, 0) != sizePayload){
		free(stream);
		return false;
	}

	deserializar_sem_destroy(stream, nombreSemaforo);

	free(stream);

	operacion_semaforo_creado(pcb, *nombreSemaforo, DESTROY);

	return true;
}
