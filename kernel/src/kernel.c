#include "../include/kernel.h"
/*
void sighandler(int x) {
    switch (x) {
        case SIGINT:
        	abortar_planificacion();
        	destruir_semaforos();
        	destruir_listas();
        	terminar_programa(socketMemoria, logger, config);
        	exit(EXIT_SUCCESS);
    }
}*/

int main(int argc, char* argv[]) {

	//signal(SIGINT , sighandler);

	inicializar_configuracion();
	inicializar_listas();
	inicializar_semaforos();
	inicializar_planificacion();

	if(!generar_conexiones()){

		abortar_planificacion();
		destruir_semaforos();
		destruir_listas();
		terminar_programa(socketMemoria, logger, config);
		return EXIT_FAILURE;
	}

	while(server_escuchar(logger, "kernel", socketServidor));

	abortar_planificacion();
	destruir_semaforos();
	destruir_listas();

	terminar_programa(socketMemoria, logger, config);
	return EXIT_SUCCESS;
}

void inicializar_configuracion(){

	logger = iniciar_logger("bin/kernel.log", "kernel", true, LOG_LEVEL_INFO);
	config = iniciar_config("cfg/kernel.config");

	ip = obtener_de_config(config, "IP");
	puerto = obtener_de_config(config, "PUERTO_ESCUCHA");
	ipMemoria = obtener_de_config(config, "IP_MEMORIA");
	puertoMemoria = obtener_de_config(config, "PUERTO_MEMORIA");

	algoritmoPlanificacion = obtener_algoritmo();
	estimacionInicial = obtener_float_de_config(config, "ESTIMACION_INICIAL");
	alfa = obtener_float_de_config(config, "ALFA");
	gradoMultiprogramacion = obtener_int_de_config(config, "GRADO_MULTIPROGRAMACION");
	gradoMultiprocesamiento = obtener_int_de_config(config, "GRADO_MULTIPROCESAMIENTO");
	intervaloDeadlock = obtener_int_de_config(config, "TIEMPO_DEADLOCK");

	pidKernel = 0;

	if(gradoMultiprocesamiento > gradoMultiprogramacion || gradoMultiprocesamiento <= 0 || gradoMultiprogramacion <= 0){

		perror("Puede que el grado de multiprocesamiento sea mayor al de multiprogramacion, o que alguno de los dos sea menor o igual a cero, revise el archivo de configuracion del Kernel para verificar que eso no suceda.\n");
		abort();
	}

	if(algoritmoPlanificacion != SJF && algoritmoPlanificacion != HRRN){

		perror("El algoritmo de planificacion ingresado no esta disponible en este sistema o esta mal escrito, revise el archivo de configuracion del Kernel para verificar que eso no suceda.\n");
		abort();
	}

	if(alfa < 0 || alfa > 1){

		perror("El alfa debe estar acotado entre 0 y 1, revise el archivo de configuracion del Kernel para verificar que ese sea el caso.\n");
		abort();
	}

	if(intervaloDeadlock < 0){

		perror("El intervalo de ejecucion del algoritmo de deteccion y recuperacion de deadlock debe ser mayor o igual a 0, revise el archivo de configuracion del Kernel para verificar que ese sea el caso.\n");
		abort();
	}
}

void inicializar_listas(){

	listaSemaforos = list_create();
	listaPotencialesRetensores = list_create();
	listaIO = list_create();
	listaHilosDeDispositivos = list_create();
	colaNew = queue_create();
	colaReady = list_create();
	listaExe = list_create();
	listaBlock = list_create();
	listaExit = list_create();
	listaBlockSuspended = list_create();
	colaReadySuspended = queue_create();

	char** dispositivosIO = config_get_array_value(config, "DISPOSITIVOS_IO");
	char** duracionesIO = config_get_array_value(config, "DURACIONES_IO");

	int cantidadDispositivos = cantidadDeElementosEnArray(dispositivosIO);

	for(int i = 0; i < cantidadDispositivos; i++){

		t_io* dispIO = malloc(sizeof(t_io));

		dispIO->io = dispositivosIO[i];
		dispIO->duracion = atoi(duracionesIO[i]);
		dispIO->listaDeEspera = queue_create();
		sem_init(&(dispIO->contadorEspera), 0, 0);
		pthread_mutex_init(&(dispIO->mutexDispositivo), NULL);
		list_add(listaIO, dispIO);
	}

	free(dispositivosIO);
	free(duracionesIO);
}

void inicializar_semaforos(){

	pthread_mutex_init(&mutexPotencialesRetensores, NULL);
	pthread_mutex_init(&mutexBlockSuspended, NULL);
	pthread_mutex_init(&mutexReadySuspended, NULL);
	pthread_mutex_init(&mutexListaSemaforos, NULL);
	pthread_mutex_init(&mutexNew, NULL);
	pthread_mutex_init(&mutexReady, NULL);
	pthread_mutex_init(&mutexBlock, NULL);
	pthread_mutex_init(&mutexExe, NULL);
	pthread_mutex_init(&mutexExit, NULL);

	sem_init(&analizarSuspension, 0, 0);
	sem_init(&suspensionFinalizada, 0, 0);
	sem_init(&contadorNew, 0, 0); // Estado New
	sem_init(&contadorReady, 0, 0); // Estado Ready
	sem_init(&contadorExe, 0, 0); // Estado Exe
	sem_init(&contadorProcesosEnMemoria, 0, 0);	// Memoria IMP HAY QUE VER COMO SE INICIALIZA PORQUE ESTO AFECTA LA DISPONIBILIDAD DE LA COLA READY
	sem_init(&multiprogramacion, 0, gradoMultiprogramacion); // Memoria
	sem_init(&multiprocesamiento, 0, gradoMultiprocesamiento); // CPU
	sem_init(&contadorBlock, 0, 0);
	sem_init(&largoPlazo, 0, 1);
	sem_init(&contadorReadySuspended, 0, 0);
	sem_init(&medianoPlazo, 0, 1);
}

void inicializar_planificacion(){

	pthread_create(&hiloQueDesuspende, NULL, (void*)hiloSuspensionAReady, NULL);
	pthread_create(&hiloMedianoPlazo, NULL, (void*)hiloBlockASuspension, NULL);
	pthread_detach(hiloQueDesuspende);
	pthread_detach(hiloMedianoPlazo);

	for(int i=0; i < list_size(listaIO); i++){

		pthread_t hiloDispositivoIO;
		list_add(listaHilosDeDispositivos, &hiloDispositivoIO);
		t_io* dispositivo = list_get(listaIO, i);
		pthread_create(&hiloDispositivoIO, NULL, (void*)funcionDispositivo, (void*) dispositivo);
		pthread_detach(hiloDispositivoIO);
	}

	pthread_create(&hiloNewReady, NULL, (void*)hiloNew_Ready, NULL);
	pthread_create(&hiloReady_Exec, NULL, (void*)hiloReady_Exe, NULL);
	pthread_detach(hiloNewReady);
	pthread_detach(hiloReady_Exec);

	pthread_create(&hiloDeadlock, NULL, (void*)deteccionYRecuperacion, NULL);
	pthread_detach(hiloDeadlock);
}

bool generar_conexiones(){

     socketServidor = iniciar_servidor(logger, "kernel", ip, puerto);
     if(socketServidor == -1)
         return 1;
     log_info(logger, "Servidor listo para recibir al cliente");

     socketMemoria = crear_conexion(logger, "memoria", ipMemoria, puertoMemoria);
     uint32_t* pid = malloc(sizeof(uint32_t));
     handshakeCliente(socketMemoria, pid);
     free(pid);
     return socketServidor != -1 && socketMemoria != -1 ;
}
int server_escuchar(t_log* logger, char* nombreServidor, int socketServidor){

	int socketCliente = esperar_cliente(logger, nombreServidor, socketServidor);

	handshakeServidor(socketCliente, KERNEL, logger);

	if(socketCliente != -1){

		op_code senial;

		if( (recv(socketCliente, &senial, sizeof(op_code), 0) == sizeof(op_code)) && (senial == CREAR_PCB) ){

			pcb_carpincho* pcb = malloc(sizeof(pcb_carpincho)); // No olvidarse el free
			pidKernel++;
			pcb->carpinchoPID = pidKernel;
			send(socketCliente, &(pcb->carpinchoPID), sizeof(uint32_t), 0);
			pcb->semaforosRetenidos = list_create();
			pcb->semaforoEsperado = NULL;
			pcb->socketCarpincho = socketCliente;
			int socketMemoriaCarpincho = crear_conexion(logger, "Memoria", ipMemoria, puertoMemoria);
			handshakeCliente(socketMemoriaCarpincho, &(pcb->carpinchoPID));
			pcb->socketMemoria = socketMemoriaCarpincho;
			agregarANew(pcb);
		}else{
			return 1;
		}
	}

	return 1; //OJO CON ESTO, PORQUE ES UN LOOP INFINITO
}

void abortar_planificacion(){

	for(int i=0; i < list_size(listaHilosDeDispositivos); i++){

		pthread_t* hiloADestruir = list_get(listaHilosDeDispositivos, i);
		pthread_cancel(*hiloADestruir);
	}

	pthread_cancel(hiloDeadlock);
	pthread_cancel(hiloNewReady); // O pthread_kill
	pthread_cancel(hiloReady_Exec);
	pthread_cancel(hiloMedianoPlazo);
	pthread_cancel(hiloQueDesuspende);
}

void destruir_semaforos(){

	pthread_mutex_destroy(&mutexPotencialesRetensores);
	pthread_mutex_destroy(&mutexListaSemaforos);
	pthread_mutex_destroy(&mutexNew);
	pthread_mutex_destroy(&mutexReady);
	pthread_mutex_destroy(&mutexBlock);
	pthread_mutex_destroy(&mutexExe);
	pthread_mutex_destroy(&mutexExit);
	pthread_mutex_destroy(&mutexBlockSuspended);
	pthread_mutex_destroy(&mutexReadySuspended);

	sem_destroy(&contadorNew);
	sem_destroy(&contadorReady);
	sem_destroy(&contadorExe);
	sem_destroy(&contadorProcesosEnMemoria);
	sem_destroy(&multiprogramacion);
	sem_destroy(&multiprocesamiento);
	sem_destroy(&contadorBlock);
	sem_destroy(&analizarSuspension);
	sem_destroy(&suspensionFinalizada);
	sem_destroy(&largoPlazo);
	sem_destroy(&contadorReadySuspended);
	sem_destroy(&medianoPlazo);
}

void destruir_listas(){

	destruirListaYElementos(listaPotencialesRetensores);
	liberarListaDeSemaforos();
	liberarListaIO();
	destruirListaYElementos(listaHilosDeDispositivos);
	destruirColaYElementos(colaNew);
	destruirListaYElementos(colaReady);
	destruirListaYElementos(listaExe);
	destruirListaYElementos(listaBlock);
	destruirListaYElementos(listaExit);
	destruirListaYElementos(listaBlockSuspended);
	destruirColaYElementos(colaReadySuspended);
}

void liberarListaDeSemaforos(){

	for(int i=0; i < list_size(listaSemaforos); i++){

		t_semaforo* semaforoAux = list_get(listaSemaforos, i);
		queue_destroy(semaforoAux->listaDeEspera);
		pthread_mutex_destroy(&(semaforoAux->mutexSemaforo));
		free(semaforoAux);
	}
}

void liberarListaIO(){

	for(int i=0; i < list_size(listaIO); i++){

		t_io* dispositivoAux = list_get(listaIO, i);
		queue_destroy(dispositivoAux->listaDeEspera);
		sem_destroy(&(dispositivoAux->contadorEspera));
		pthread_mutex_destroy(&(dispositivoAux->mutexDispositivo));
		free(dispositivoAux);
	}
}
