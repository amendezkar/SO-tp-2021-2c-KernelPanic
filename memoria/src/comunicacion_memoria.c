/*
 * comunicacion_memoria.c
 *
 *  Created on: 13 sep. 2021
 *      Author: utnso
 */
#include "comunicacion_memoria.h"



bool init_config(){

	char* contenidoConfig[] = {
			"IP",
			"IP_SWAMP",
			"PUERTO",
			"PUERTO_SWAMP",
			"TAMANIO",
			"TAMANIO_PAGINA",
			"TIPO_ASIGNACION",
			"MARCOS_MAXIMOS",
			"ALGORITMO_REEMPLAZO_MMU",
			"CANTIDAD_ENTRADAS_TLB",
			"ALGORITMO_REEMPLAZO_TLB",
			"RETARDO_ACIERTO_TLB",
			"RETARDO_FALLO_TLB",
			"PATH_DUMP_TLB",
			NULL
	    };

	 config = iniciar_config("cfg/memoria.config");

	 if(!config_tiene_todas_las_propiedades(config, contenidoConfig)) {
	         log_error(logger, "Propiedades faltantes en el archivo de configuracion");
	         config_destroy(config);
	         return false;
	 }

	 cfg = malloc(sizeof(t_cfg));

	 cfg->ip = obtener_de_config(config,"IP");
	 cfg->ipSwamp = obtener_de_config(config,"IP_SWAMP");
	 cfg->puerto = obtener_de_config(config,"PUERTO");
	 cfg->puertoSwamp = obtener_de_config(config,"PUERTO_SWAMP");
	 cfg->tamanio = atoi(obtener_de_config(config,"TAMANIO"));
	 cfg->tamanioPagina = atoi(obtener_de_config(config,"TAMANIO_PAGINA"));
	 cfg->cantPaginas = cfg->tamanio / cfg->tamanioPagina;
	 cfg->esFija = strcmp(obtener_de_config(config,"TIPO_ASIGNACION"), "FIJA")==0;
	 cfg->marcosMaximos = atoi(obtener_de_config(config,"MARCOS_MAXIMOS"));
	 cfg->esClock = strcmp(obtener_de_config(config,"ALGORITMO_REEMPLAZO_MMU"), "CLOCK-M")==0 ;
	 cfg->cantEntradasTLB = atoi(obtener_de_config(config,"CANTIDAD_ENTRADAS_TLB"));
	 cfg->esFifo = strcmp(obtener_de_config(config,"ALGORITMO_REEMPLAZO_TLB"), "FIFO")==0;
	 cfg->retardoAciertoTLB = atoi(obtener_de_config(config,"RETARDO_ACIERTO_TLB"));
	 cfg->retardoFalloTLB = atoi(obtener_de_config(config,"RETARDO_FALLO_TLB"));
	 cfg->pathDump = obtener_de_config(config,"PATH_DUMP_TLB");
	 return true;

}

bool generar_conexiones(){

	 socketMemoria = iniciar_servidor(logger,"memoria",cfg->ip,cfg->puerto);
	 if(socketMemoria == -1)
		 return 1;
	 log_info(logger, "Servidor listo para recibir al cliente");

	 socketSwamp = crear_conexion(logger,"swamp",cfg->ipSwamp,cfg->puertoSwamp);
	 return socketMemoria != -1 && socketSwamp != -1 ;
}


int server_escuchar(char* server_name, int server_socket) {
    int socketCliente = esperar_cliente(logger, server_name, server_socket);

    handshakeServidor(socketCliente, MEMORIA, logger);

    if (socketCliente != -1) {
        pthread_t hilo;
        t_procesar_conexion_args* args = malloc(sizeof(t_procesar_conexion_args));
        args->logger = logger;
		args->socket = socketCliente;
		args->nombreServidor = server_name;
        pthread_create(&hilo, NULL, (void*) procesar_conexion, (void*) args);
        pthread_detach(hilo);
        return 1;
    }
    return 0;
}

void procesar_conexion(void* void_args) {
    t_procesar_conexion_args* args = (t_procesar_conexion_args*) void_args;
    int socketCliente = args->socket;
    //char* server_name = args->server_name;
    free(args);

    op_code cod_op;
    while (socketCliente != -1) {

    		if(recv(socketCliente, &cod_op, sizeof(op_code), 0) != sizeof(op_code)){
    				log_info(logger, "Esperando un codigo de operacion...");
    				break;
    		}

            switch (cod_op) {

            		case ASIGNAR_PID:
            		{
            			pthread_mutex_lock(&mutexMemoria);
            			uint32_t pid = PIDGlobal++;
            			pthread_mutex_unlock(&mutexMemoria);

            			send(socketCliente,&pid,sizeof(uint32_t),0);

            			break;
            		}
                    case MEMALLOC:
                    {
                    	int size;
                    	uint32_t pid;
                    	if(!recv_memalloc(socketCliente,&size,&pid)){
                            log_error(logger, "No se pudo recibir el memalloc");
                    	}
                    	uint32_t direccionMemoria;

                    	pthread_mutex_lock(&mutexMemoria);
                    	direccionMemoria = reservar_memoria(pid,size);
                    	pthread_mutex_unlock(&mutexMemoria);



                    	if(!send_direcLogica(socketCliente, direccionMemoria)){
							log_error(logger, "No se pudo enviar la direccion de memoria allocado");

						}


                    	break;
                    }
                    case MEMFREE:
                    {
                    	uint32_t direcLogica;
                    	uint32_t pid;
						if(!recv_memfree(socketCliente,&direcLogica, &pid)){
							log_error(logger, "No se pudo recibir el memfree");
						}


						pthread_mutex_lock(&mutexMemoria);
						bool respuesta = liberar_memoria(direcLogica, pid);
                    	pthread_mutex_unlock(&mutexMemoria);

						int codigoRespuesta;
						if(!respuesta){
							codigoRespuesta = -5;

						}else{
							codigoRespuesta = 1;
						}

						if(!send(socketCliente, &codigoRespuesta, sizeof(int), 0)){
							log_error(logger, "No se pudo enviar el codigo de respuesta de memfree");

						}


                    	break;
                    }
                    case MEMREAD:
					{
						uint32_t direcLogica;
						int size;
						uint32_t pid;
						if(!recv_memread(socketCliente,&direcLogica,&size, &pid)){
							log_error(logger, "No se pudo recibir el memread");
						}

						void* respuesta;
                    	pthread_mutex_lock(&mutexMemoria);
						respuesta = leer_memoria(direcLogica , size, pid);
                    	pthread_mutex_unlock(&mutexMemoria);

						int codigoRespuesta;
						if(respuesta == NULL){
							codigoRespuesta = -6;
							if(!send(socketCliente, &codigoRespuesta, sizeof(int), 0)){
								log_error(logger, "No se pudo enviar el codigo de respuesta de memread");

							}
							break;

						}else{
							codigoRespuesta = 1;
							if(!send(socketCliente, &codigoRespuesta, sizeof(int), 0)){
								log_error(logger, "No se pudo enviar el codigo de respuesta de memread");

							}
						}

						if(!send_contenido_memread(socketCliente, respuesta, size)){

							log_error(logger, "No se pudo enviar el contenido de memread");

							}
						free(respuesta);

						break;
					}
                    case MEMWRITE:
					{
						uint32_t direcLogica;
						int size;
						uint32_t pid;
						void* contenido;

						if(!recv_memwrite(socketCliente,&direcLogica,&contenido,&size, &pid)){
							log_error(logger, "No se pudo recibir el memwrite");
						}

                    	pthread_mutex_lock(&mutexMemoria);
						bool respuesta = escribir_memoria(direcLogica, pid, size, contenido);
                    	pthread_mutex_unlock(&mutexMemoria);

                    	free(contenido);

						int codigoRespuesta;
						if(!respuesta){
							codigoRespuesta = -7;

						}else{
							codigoRespuesta = 1;
						}

						if(!send(socketCliente, &codigoRespuesta, sizeof(int), 0)){
							log_error(logger, "No se pudo enviar el codigo de respuesta de memwrite");

						}

						break;
					}
                    case TERMINAR_EJECUCION:
					{
						uint32_t pid;
						recv(socketCliente, &pid, sizeof(uint32_t), 0);

						bool existeCarpincho = false;
						bool respuestaMemoria = true;
                    	pthread_mutex_lock(&mutexMemoria);
						//respuestaMemoria = finalizar_proceso(pid, &existeCarpincho);
                    	pthread_mutex_unlock(&mutexMemoria);

                    	if(!existeCarpincho){
                    		//printf("No existe el carpincho que termina su ejecucion\n");
                    		send_respuesta_bool(socketCliente, true);
                    		break;
                    	}

						if(!respuestaMemoria){
							printf("Memoria no pudo finalizar el proceso\n");
							send_respuesta_bool(socketCliente, false);
							break;
						}
                    	pthread_mutex_lock(&mutexMemoria);

						if(!send_finalizar_proceso_swamp(socketSwamp, pid)){
							log_error(logger, "No se pudo enviar el finalizar proceso a swamp\n");
						}
                    	pthread_mutex_unlock(&mutexMemoria);

						MEM_SWAP_MESSAGE respuestaSwamp;

						if(recv(socketSwamp, &respuestaSwamp, sizeof(MEM_SWAP_MESSAGE), 0) == sizeof(MEM_SWAP_MESSAGE)){

							if(respuestaSwamp == PROCESS_RELEASED){
								send_respuesta_bool(socketCliente, true);
							}else{
								printf("Swamp no pudo finalizar el proceso\n");
								send_respuesta_bool(socketCliente, false);
							}
						}else{
							printf("No se recibio correctamente la respuesta de swamp\n");
							send_respuesta_bool(socketCliente, false);
						}


						break;
					}
                    case SUSPEND:
					{
						uint32_t pid;
						recv(socketCliente, &pid, sizeof(uint32_t), 0);

                    	pthread_mutex_lock(&mutexMemoria);
						if(!suspender_proceso(pid)){
							send_respuesta_bool(socketCliente, false);
						}
                    	pthread_mutex_unlock(&mutexMemoria);

						send_respuesta_bool(socketCliente, true);

						break;
					}

                    default:
                        log_info(logger,
                                "No hay operacion, continuo...");
                        break;
                    }
}
}
