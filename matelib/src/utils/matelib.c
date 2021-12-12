#include "../../include/utils/matelib.h"

//------------------General Functions---------------------/


int mate_init(mate_instance *lib_ref, char *config)//Este es el path del config del proceso carpincho
{
    lib_ref->group_info = malloc(sizeof(mate_posta));

    ((mate_posta *) lib_ref->group_info)->config = iniciar_config(config); //Este es el config del proceso carpincho
    ((mate_posta *) lib_ref->group_info)->nombreServidor = obtener_de_config(((mate_posta *) lib_ref->group_info)->config, "NOMBRE_SERVIDOR");
    ((mate_posta *) lib_ref->group_info)->nombreCarpincho = obtener_de_config(((mate_posta *) lib_ref->group_info)->config, "NOMBRE_CARPINCHO");
    ((mate_posta *) lib_ref->group_info)->ipServidor = obtener_de_config(((mate_posta *) lib_ref->group_info)->config, "IP");
    ((mate_posta *) lib_ref->group_info)->puertoServidor = obtener_de_config(((mate_posta *) lib_ref->group_info)->config, "PUERTO");
    ((mate_posta *) lib_ref->group_info)->loggerPath = obtener_de_config(((mate_posta *) lib_ref->group_info)->config, "PATH_LOGGER");

    ((mate_posta *) lib_ref->group_info)->logger = iniciar_logger(((mate_posta *) lib_ref->group_info)->loggerPath, ((mate_posta *) lib_ref->group_info)->nombreCarpincho, true, LOG_LEVEL_DEBUG);

    ((mate_posta *) lib_ref->group_info)->socketCliente = crear_conexion(((mate_posta *) lib_ref->group_info)->logger, ((mate_posta *) lib_ref->group_info)->nombreServidor, ((mate_posta *) lib_ref->group_info)->ipServidor, ((mate_posta *) lib_ref->group_info)->puertoServidor);

    ((mate_posta *) lib_ref->group_info)->handshakeServidor = handshakeCliente(((mate_posta *) lib_ref->group_info)->socketCliente, &(((mate_posta *) lib_ref->group_info)->pid));

    if(((mate_posta *) lib_ref->group_info)->handshakeServidor == KERNEL){

    	sem_code autorizacion;

    	//printf("ESPERANDO AUTORIZACION TRAS TERMINAR MATE_INIT PID %d\n", ((mate_posta *) lib_ref->group_info)->pid);

    	if(recv(((mate_posta *) lib_ref->group_info)->socketCliente, &autorizacion, sizeof(sem_code), 0) != sizeof(sem_code) || autorizacion != CONTINUE){
    		log_warning(((mate_posta *) lib_ref->group_info)->logger, "%s PID %d tuvo problemas al recibir la senial de continue tras terminar mate_init", ((mate_posta *) lib_ref->group_info)->nombreCarpincho, ((mate_posta *) lib_ref->group_info)->pid);
    		return false;
    	}
    }

    return 0;
}

int mate_close(mate_instance *lib_ref)
{
	sem_code autorizacion;
	bool respuesta;

	op_code cop = TERMINAR_EJECUCION;
	if(send(((mate_posta *) lib_ref->group_info)->socketCliente, &cop, sizeof(op_code), 0) != sizeof(op_code)){
		log_warning(((mate_posta *) lib_ref->group_info)->logger, "%s PID %d tuvo problemas al enviar la senial de TERMINAR_EJECUCION en mate_close", ((mate_posta *) lib_ref->group_info)->nombreCarpincho, ((mate_posta *) lib_ref->group_info)->pid);
	}

	if(((mate_posta *) lib_ref->group_info)->handshakeServidor == KERNEL){
		
		recv(((mate_posta *) lib_ref->group_info)->socketCliente, &autorizacion, sizeof(sem_code), 0);

		while(autorizacion != READY_TO_FINISH){
			recv(((mate_posta *) lib_ref->group_info)->socketCliente, &autorizacion, sizeof(sem_code), 0);
		}
	}else{
		if(((mate_posta *) lib_ref->group_info)->handshakeServidor == MEMORIA){
			recv_respuesta_bool(((mate_posta *) lib_ref->group_info)->socketCliente,&respuesta);
		}else{
			perror("Debe hacer mate_init antes de poder utilizar la funcion mate_close de la matelib");
			return false;
		}
	}

	/*if(((mate_posta *) lib_ref->group_info)->handshakeServidor == KERNEL){

		sem_code autorizacion;

		printf("ESPERANDO AUTORIZACION PARA MATE_CLOSE PID %d\n", ((mate_posta *) lib_ref->group_info)->pid);

		if(recv(((mate_posta *) lib_ref->group_info)->socketCliente, &autorizacion, sizeof(sem_code), 0) != sizeof(sem_code) || autorizacion != CONTINUE){
			log_warning(((mate_posta *) lib_ref->group_info)->logger, "%s PID %d tuvo problemas al recibir la senial de continue en mate_close", ((mate_posta *) lib_ref->group_info)->nombreCarpincho, ((mate_posta *) lib_ref->group_info)->pid);
			return false;
		}

		op_code cop = TERMINAR_EJECUCION;
		if(send(((mate_posta *) lib_ref->group_info)->socketCliente, &cop, sizeof(op_code), 0) != sizeof(op_code)){
			log_warning(((mate_posta *) lib_ref->group_info)->logger, "%s PID %d tuvo problemas al enviar la senial de TERMINAR_EJECUCION en mate_close", ((mate_posta *) lib_ref->group_info)->nombreCarpincho, ((mate_posta *) lib_ref->group_info)->pid);
		}

		recv(((mate_posta *) lib_ref->group_info)->socketCliente, &autorizacion, sizeof(sem_code), 0);

		while(autorizacion != READY_TO_FINISH){
			recv(((mate_posta *) lib_ref->group_info)->socketCliente, &autorizacion, sizeof(sem_code), 0);
		}

	}else{
		if(((mate_posta *) lib_ref->group_info)->handshakeServidor == MEMORIA){

			op_code cop = TERMINAR_EJECUCION;
			if(send(((mate_posta *) lib_ref->group_info)->socketCliente, &cop, sizeof(op_code), 0) != sizeof(op_code)){
				log_warning(((mate_posta *) lib_ref->group_info)->logger, "%s PID %d tuvo problemas al enviar la senial de TERMINAR_EJECUCION en mate_close", ((mate_posta *) lib_ref->group_info)->nombreCarpincho, ((mate_posta *) lib_ref->group_info)->pid);
			}
		}else{

			perror("Debe hacer mate_init antes de poder utilizar la funcion mate_close de la matelib");
			return false;
		}
	}*/

  log_info(((mate_posta *) lib_ref->group_info)->logger, "%s PID %d finaliza la ejecucion.", ((mate_posta *) lib_ref->group_info)->nombreCarpincho, ((mate_posta *) lib_ref->group_info)->pid);

  terminar_programa(((mate_posta *) lib_ref->group_info)->socketCliente, ((mate_posta *) lib_ref->group_info)->logger, ((mate_posta *) lib_ref->group_info)->config);
  //free(((mate_posta *) lib_ref->group_info)->loggerPath);
  //free(((mate_posta *) lib_ref->group_info)->nombreCarpincho);
  //free(((mate_posta *) lib_ref->group_info)->ipServidor);
  //free(((mate_posta *) lib_ref->group_info)->puertoServidor);
  //free(((mate_posta *) lib_ref->group_info)->nombreServidor);
  free(lib_ref->group_info);
 // free(lib_ref);

   pthread_exit(NULL);

  //return 0;
}

//-----------------Semaphore Functions---------------------/

int mate_sem_init(mate_instance *lib_ref, mate_sem_name sem, unsigned int value) {

	/*if(((mate_posta *) lib_ref->group_info)->handshakeServidor == KERNEL){

	sem_code autorizacion;

	printf("ESPERANDO AUTORIZACION PARA MATE_SEM_INIT PID %d\n", ((mate_posta *) lib_ref->group_info)->pid);

	if(recv(((mate_posta *) lib_ref->group_info)->socketCliente, &autorizacion, sizeof(sem_code), 0) != sizeof(sem_code) || autorizacion != CONTINUE){

		log_warning(((mate_posta *) lib_ref->group_info)->logger, "%s PID %d tuvo problemas al recibir la senial de continue en mate_sem_init a %s", ((mate_posta *) lib_ref->group_info)->nombreCarpincho, ((mate_posta *) lib_ref->group_info)->pid, sem);
		return false;
	}
	}else{
		if(((mate_posta *) lib_ref->group_info)->handshakeServidor == MEMORIA){

			log_warning(((mate_posta *) lib_ref->group_info)->logger, "La funcion mate_sem_init no esta disponible, dado que es una funcion de Kernel");
			return false;
		}else{

			perror("Debe hacer mate_init y conectarse a Kernel antes de poder utilizar la funcion mate_sem_init de la matelib");
			return false;
		}
	}*/

	size_t size;
	void* stream = serializar_sem_init(&size, sem, value);

	sem_code resultado;

	if(send(((mate_posta *) lib_ref->group_info)->socketCliente, stream, size, 0) != size){

		//free(stream);
		log_warning(((mate_posta *) lib_ref->group_info)->logger, "%s PID %d no pudo enviar la instruccion de inicializar el semaforo %s", ((mate_posta *) lib_ref->group_info)->nombreCarpincho, ((mate_posta *) lib_ref->group_info)->pid, sem);
	}else{

		if(recv(((mate_posta *) lib_ref->group_info)->socketCliente, &resultado, sizeof(sem_code), 0) != sizeof(sem_code)){

			log_warning(((mate_posta *) lib_ref->group_info)->logger, "%s PID %d tuvo problemas para recibir el resultado de inicializar el semaforo %s", ((mate_posta *) lib_ref->group_info)->nombreCarpincho, ((mate_posta *) lib_ref->group_info)->pid, sem);
		}else{

			if(resultado == NOT_FOUND){
				log_info(((mate_posta *) lib_ref->group_info)->logger, "%s PID %d inicializa el semaforo %s con valor %d", ((mate_posta *) lib_ref->group_info)->nombreCarpincho, ((mate_posta *) lib_ref->group_info)->pid, sem, value);
			}else{
				log_warning(((mate_posta *) lib_ref->group_info)->logger, "%s PID %d intenta inicializar al semaforo ya existente %s", ((mate_posta *) lib_ref->group_info)->nombreCarpincho, ((mate_posta *) lib_ref->group_info)->pid, sem);
				//free(stream);
			}
		}
		//free(stream);
	}
	free(stream);

	if(((mate_posta *) lib_ref->group_info)->handshakeServidor == KERNEL){

		sem_code autorizacion;

		//printf("ESPERANDO AUTORIZACION TRAS TERMINAR MATE_SEM_INIT PID %d\n", ((mate_posta *) lib_ref->group_info)->pid);

		if(recv(((mate_posta *) lib_ref->group_info)->socketCliente, &autorizacion, sizeof(sem_code), 0) != sizeof(sem_code) || autorizacion != CONTINUE){
			log_warning(((mate_posta *) lib_ref->group_info)->logger, "%s PID %d tuvo problemas al recibir la senial de continue tras terminat mate_sem_init", ((mate_posta *) lib_ref->group_info)->nombreCarpincho, ((mate_posta *) lib_ref->group_info)->pid);
			return false;
		}
	}

	return true;
}

int mate_sem_wait(mate_instance *lib_ref, mate_sem_name sem) {

	sem_code autorizacion;

	/*if(((mate_posta *) lib_ref->group_info)->handshakeServidor == KERNEL){

		printf("ESPERANDO AUTORIZACION PARA MATE_SEM_WAIT PID %d\n", ((mate_posta *) lib_ref->group_info)->pid);

	if(recv(((mate_posta *) lib_ref->group_info)->socketCliente, &autorizacion, sizeof(sem_code), 0) != sizeof(sem_code) || autorizacion != CONTINUE){
		log_warning(((mate_posta *) lib_ref->group_info)->logger, "%s PID %d tuvo problemas al recibir la senial de continue en mate_sem_wait a %s", ((mate_posta *) lib_ref->group_info)->nombreCarpincho, ((mate_posta *) lib_ref->group_info)->pid, sem);
		return false;
	}
	}else{
		if(((mate_posta *) lib_ref->group_info)->handshakeServidor == MEMORIA){

			log_warning(((mate_posta *) lib_ref->group_info)->logger, "La funcion mate_sem_wait no esta disponible, dado que es una funcion de Kernel");
			return false;
		}else{

			perror("Debe hacer mate_init y conectarse a Kernel antes de poder utilizar la funcion mate_sem_wait de la matelib");
			return false;
		}
	}*/

	size_t size;
	void* stream = serializar_sem_wait(&size, sem);

	sem_code resultado;

	if(send(((mate_posta *) lib_ref->group_info)->socketCliente, stream, size, 0) != size){

		//free(stream);
		log_warning(((mate_posta *) lib_ref->group_info)->logger, "%s PID %d no pudo enviar la instruccion de wait al semaforo %s", ((mate_posta *) lib_ref->group_info)->nombreCarpincho, ((mate_posta *) lib_ref->group_info)->pid, sem);
	}else{

		if(recv(((mate_posta *) lib_ref->group_info)->socketCliente, &resultado, sizeof(sem_code), 0) != sizeof(sem_code)){

			log_warning(((mate_posta *) lib_ref->group_info)->logger, "%s PID %d tuvo problemas para recibir el resultado de hacer wait al semaforo %s", ((mate_posta *) lib_ref->group_info)->nombreCarpincho, ((mate_posta *) lib_ref->group_info)->pid, sem);
		}else{

			if(resultado == FOUND){
				log_info(((mate_posta *) lib_ref->group_info)->logger, "%s PID %d hace wait al semaforo %s", ((mate_posta *) lib_ref->group_info)->nombreCarpincho, ((mate_posta *) lib_ref->group_info)->pid, sem);
			}else{
				log_warning(((mate_posta *) lib_ref->group_info)->logger, "%s PID %d intenta hacer wait a al semaforo inexistente %s", ((mate_posta *) lib_ref->group_info)->nombreCarpincho, ((mate_posta *) lib_ref->group_info)->pid, sem);
				//free(stream);
			}
		}

		//free(stream);
	}

	free(stream);

	if(recv(((mate_posta *) lib_ref->group_info)->socketCliente, &autorizacion, sizeof(sem_code), 0) != sizeof(sem_code)){

		log_warning(((mate_posta *) lib_ref->group_info)->logger, "%s PID %d tuvo problemas al recibir la senial de continue de wait al semaforo %s", ((mate_posta *) lib_ref->group_info)->nombreCarpincho, ((mate_posta *) lib_ref->group_info)->pid, sem);
		return false;
	}else{
		if(autorizacion == CONTINUE){

			log_info(((mate_posta *) lib_ref->group_info)->logger, "%s PID %d continua ejecutando tras hacer wait al semaforo %s", ((mate_posta *) lib_ref->group_info)->nombreCarpincho, ((mate_posta *) lib_ref->group_info)->pid, sem);
			recv(((mate_posta *) lib_ref->group_info)->socketCliente, &autorizacion, sizeof(sem_code), 0);
		}else{

			recv(((mate_posta *) lib_ref->group_info)->socketCliente, &autorizacion, sizeof(sem_code), 0);
			mate_close(lib_ref);
			//return false;
		}
	}

	/*if(((mate_posta *) lib_ref->group_info)->handshakeServidor == KERNEL){

		sem_code autorizacionDelQueSigue;

		printf("ESPERANDO AUTORIZACION TRAS TERMINAR MATE_SEM_WAIT PID %d\n", ((mate_posta *) lib_ref->group_info)->pid);

		if(recv(((mate_posta *) lib_ref->group_info)->socketCliente, &autorizacionDelQueSigue, sizeof(sem_code), 0) != sizeof(sem_code) || autorizacion != CONTINUE){
			log_warning(((mate_posta *) lib_ref->group_info)->logger, "%s PID %d tuvo problemas al recibir la senial de continue tras terminar mate_sem_wait", ((mate_posta *) lib_ref->group_info)->nombreCarpincho, ((mate_posta *) lib_ref->group_info)->pid);
			return false;
		}
	}
*/
	return true;
}

int mate_sem_post(mate_instance *lib_ref, mate_sem_name sem) {

	/*if(((mate_posta *) lib_ref->group_info)->handshakeServidor == KERNEL){

	sem_code autorizacion;

	printf("ESPERANDO AUTORIZACION PARA MATE_SEM_POST PID %d\n", ((mate_posta *) lib_ref->group_info)->pid);

	if(recv(((mate_posta *) lib_ref->group_info)->socketCliente, &autorizacion, sizeof(sem_code), 0) != sizeof(sem_code) || autorizacion != CONTINUE){
		log_warning(((mate_posta *) lib_ref->group_info)->logger, "%s PID %d tuvo problemas al recibir la senial de continue en mate_sem_post a %s", ((mate_posta *) lib_ref->group_info)->nombreCarpincho, ((mate_posta *) lib_ref->group_info)->pid, sem);
		return false;
	}
	}else{
		if(((mate_posta *) lib_ref->group_info)->handshakeServidor == MEMORIA){

			log_warning(((mate_posta *) lib_ref->group_info)->logger, "La funcion mate_sem_post no esta disponible, dado que es una funcion de Kernel");
			return false;
		}else{

			perror("Debe hacer mate_init y conectarse a Kernel antes de poder utilizar la funcion mate_sem_post de la matelib");
			return false;
		}
	}*/

	size_t size;
	void* stream = serializar_sem_post(&size, sem);

	sem_code resultado;

	if(send(((mate_posta *) lib_ref->group_info)->socketCliente, stream, size, 0) != size){
		//free(stream);
		log_warning(((mate_posta *) lib_ref->group_info)->logger, "%s PID %d no pudo enviar la instruccion de post al semaforo %s", ((mate_posta *) lib_ref->group_info)->nombreCarpincho, ((mate_posta *) lib_ref->group_info)->pid, sem);
	}else{

		if(recv(((mate_posta *) lib_ref->group_info)->socketCliente, &resultado, sizeof(sem_code), 0) != sizeof(sem_code)){

			log_warning(((mate_posta *) lib_ref->group_info)->logger, "%s PID %d tuvo problemas para recibir el resultado de hacer post al semaforo %s", ((mate_posta *) lib_ref->group_info)->nombreCarpincho, ((mate_posta *) lib_ref->group_info)->pid, sem);
		}else{

			if(resultado == FOUND){
				log_info(((mate_posta *) lib_ref->group_info)->logger, "%s PID %d hace post al semaforo %s", ((mate_posta *) lib_ref->group_info)->nombreCarpincho, ((mate_posta *) lib_ref->group_info)->pid, sem);
			}else{
				log_warning(((mate_posta *) lib_ref->group_info)->logger, "%s PID %d intenta hacer post al semaforo inexistente %s", ((mate_posta *) lib_ref->group_info)->nombreCarpincho, ((mate_posta *) lib_ref->group_info)->pid, sem);
				//free(stream);
			}
		}
		//free(stream);
	}
	free(stream);

	if(((mate_posta *) lib_ref->group_info)->handshakeServidor == KERNEL){

		sem_code autorizacion;

		//printf("ESPERANDO AUTORIZACION TRAS TERMINAR MATE_SEM_POST PID %d\n", ((mate_posta *) lib_ref->group_info)->pid);

		if(recv(((mate_posta *) lib_ref->group_info)->socketCliente, &autorizacion, sizeof(sem_code), 0) != sizeof(sem_code) || autorizacion != CONTINUE){
			log_warning(((mate_posta *) lib_ref->group_info)->logger, "%s PID %d tuvo problemas al recibir la senial de continue tras terminar mate_sem_post", ((mate_posta *) lib_ref->group_info)->nombreCarpincho, ((mate_posta *) lib_ref->group_info)->pid);
			return false;
		}
	}

	return true;
}

int mate_sem_destroy(mate_instance *lib_ref, mate_sem_name sem) {

	/*if(((mate_posta *) lib_ref->group_info)->handshakeServidor == KERNEL){

	sem_code autorizacion;

	printf("ESPERANDO AUTORIZACION PARA MATE_SEM_DESTROY PID %d\n", ((mate_posta *) lib_ref->group_info)->pid);

	if(recv(((mate_posta *) lib_ref->group_info)->socketCliente, &autorizacion, sizeof(sem_code), 0) != sizeof(sem_code) || autorizacion != CONTINUE){
		log_warning(((mate_posta *) lib_ref->group_info)->logger, "%s PID %d tuvo problemas al recibir la senial de continue en mate_sem_destroy a %s", ((mate_posta *) lib_ref->group_info)->nombreCarpincho, ((mate_posta *) lib_ref->group_info)->pid, sem);
		return false;
	}
	}else{
		if(((mate_posta *) lib_ref->group_info)->handshakeServidor == MEMORIA){

			log_warning(((mate_posta *) lib_ref->group_info)->logger, "La funcion mate_sem_destroy no esta disponible, dado que es una funcion de Kernel");
			return false;
		}else{

			perror("Debe hacer mate_init y conectarse a Kernel antes de poder utilizar la funcion mate_sem_destroy de la matelib");
			return false;
		}
	}*/

	size_t size;
	void* stream = serializar_sem_destroy(&size, sem);

	sem_code resultado;

	if(send(((mate_posta *) lib_ref->group_info)->socketCliente, stream, size, 0) != size){
		//free(stream);
		log_warning(((mate_posta *) lib_ref->group_info)->logger, "%s PID %d no pudo enviar la instruccion de destruir el semaforo %s", ((mate_posta *) lib_ref->group_info)->nombreCarpincho, ((mate_posta *) lib_ref->group_info)->pid, sem);
	}else{

		if(recv(((mate_posta *) lib_ref->group_info)->socketCliente, &resultado, sizeof(sem_code), 0) != sizeof(sem_code)){

			log_warning(((mate_posta *) lib_ref->group_info)->logger, "%s PID %d tuvo problemas para recibir el resultado de destruir el semaforo %s", ((mate_posta *) lib_ref->group_info)->nombreCarpincho, ((mate_posta *) lib_ref->group_info)->pid, sem);
		}else{

			if(resultado == FOUND){

				log_info(((mate_posta *) lib_ref->group_info)->logger, "%s PID %d destruye el semaforo %s", ((mate_posta *) lib_ref->group_info)->nombreCarpincho, ((mate_posta *) lib_ref->group_info)->pid, sem);
			}else{

				log_warning(((mate_posta *) lib_ref->group_info)->logger, "%s PID %d intenta destruir el semaforo inexistente %s", ((mate_posta *) lib_ref->group_info)->nombreCarpincho, ((mate_posta *) lib_ref->group_info)->pid, sem);
				//free(stream);
			}
		}

		//free(stream);
	}
	free(stream);

	if(((mate_posta *) lib_ref->group_info)->handshakeServidor == KERNEL){

		sem_code autorizacion;

		//printf("ESPERANDO AUTORIZACION TRAS TERMINAR MATE_SEM_DESTROY PID %d\n", ((mate_posta *) lib_ref->group_info)->pid);

		if(recv(((mate_posta *) lib_ref->group_info)->socketCliente, &autorizacion, sizeof(sem_code), 0) != sizeof(sem_code) || autorizacion != CONTINUE){
			log_warning(((mate_posta *) lib_ref->group_info)->logger, "%s PID %d tuvo problemas al recibir la senial de continue tras terminar mate_sem_destroy", ((mate_posta *) lib_ref->group_info)->nombreCarpincho, ((mate_posta *) lib_ref->group_info)->pid);
			return false;
		}
	}

	return true;
}

//--------------------IO Functions------------------------/

int mate_call_io(mate_instance *lib_ref, mate_io_resource io, void *msg){

	/*if(((mate_posta *) lib_ref->group_info)->handshakeServidor == KERNEL){

	sem_code autorizacion;

	printf("ESPERANDO AUTORIZACION PARA MATE_CALL_IO PID %d\n", ((mate_posta *) lib_ref->group_info)->pid);

	if(recv(((mate_posta *) lib_ref->group_info)->socketCliente, &autorizacion, sizeof(sem_code), 0) != sizeof(sem_code) || autorizacion != CONTINUE){
		log_warning(((mate_posta *) lib_ref->group_info)->logger, "%s PID %d tuvo problemas al recibir la senial de continue", ((mate_posta *) lib_ref->group_info)->nombreCarpincho, ((mate_posta *) lib_ref->group_info)->pid);
		return false;
	}
	}else{
		if(((mate_posta *) lib_ref->group_info)->handshakeServidor == MEMORIA){

			log_warning(((mate_posta *) lib_ref->group_info)->logger, "La funcion mate_call_io no esta disponible, dado que es una funcion de Kernel");
			return false;
		}else{

			perror("Debe hacer mate_init y conectarse a Kernel antes de poder utilizar la funcion mate_call_io de la matelib");
			return false;
		}
	}*/

	size_t size;
	void* stream = serializar_call_io(&size, io, (char*) msg);

	sem_code resultado;

	if(send(((mate_posta *) lib_ref->group_info)->socketCliente, stream, size, 0) != size){
		//free(stream);
		log_warning(((mate_posta *) lib_ref->group_info)->logger, "%s PID %d no pudo enviar la instruccion de llamada al dispositivo %s", ((mate_posta *) lib_ref->group_info)->nombreCarpincho, ((mate_posta *) lib_ref->group_info)->pid, io);
	}else{

		log_info(((mate_posta *) lib_ref->group_info)->logger, "%s PID %d espera el resultado de realizar la E/S al dispositivo %s", ((mate_posta *) lib_ref->group_info)->nombreCarpincho, ((mate_posta *) lib_ref->group_info)->pid, io);
		if(recv(((mate_posta *) lib_ref->group_info)->socketCliente, &resultado, sizeof(sem_code), 0) != sizeof(sem_code)){

			log_warning(((mate_posta *) lib_ref->group_info)->logger, "%s PID %d tuvo problemas para recibir el resultado de realizar una E/S al dispositivo %s", ((mate_posta *) lib_ref->group_info)->nombreCarpincho, ((mate_posta *) lib_ref->group_info)->pid, io);
		}else{

			if(resultado == FOUND){

				log_info(((mate_posta *) lib_ref->group_info)->logger, "%s PID %d realiza la E/S al dispositivo %s loggeando el mensaje: %s", ((mate_posta *) lib_ref->group_info)->nombreCarpincho, ((mate_posta *) lib_ref->group_info)->pid, io, (char*) msg);
			}else{

				log_warning(((mate_posta *) lib_ref->group_info)->logger, "%s PID %d intenta realizar una E/S al dispositivo inexistente %s", ((mate_posta *) lib_ref->group_info)->nombreCarpincho, ((mate_posta *) lib_ref->group_info)->pid, io);
				//free(stream);
			}
		}

		//free(stream);
	}
	free(stream);

	if(((mate_posta *) lib_ref->group_info)->handshakeServidor == KERNEL){

		sem_code autorizacion;

		//printf("ESPERANDO AUTORIZACION TRAS TERMINAR MATE_CALL_IO PID %d\n", ((mate_posta *) lib_ref->group_info)->pid);

		if(recv(((mate_posta *) lib_ref->group_info)->socketCliente, &autorizacion, sizeof(sem_code), 0) != sizeof(sem_code) || autorizacion != CONTINUE){
			log_warning(((mate_posta *) lib_ref->group_info)->logger, "%s PID %d tuvo problemas al recibir la senial de continue tras terminar mate_call_io", ((mate_posta *) lib_ref->group_info)->nombreCarpincho, ((mate_posta *) lib_ref->group_info)->pid);
			return false;
		}
	}

  return true;
}

//--------------Memory Module Functions-------------------/

mate_pointer mate_memalloc(mate_instance *lib_ref, int size)
{

	/*if(((mate_posta *) lib_ref->group_info)->handshakeServidor == KERNEL){

	sem_code autorizacion;

	printf("ESPERANDO AUTORIZACION PARA MATE_MEMALLOC PID %d\n", ((mate_posta *) lib_ref->group_info)->pid);

	if(recv(((mate_posta *) lib_ref->group_info)->socketCliente, &autorizacion, sizeof(sem_code), 0) != sizeof(sem_code) || autorizacion != CONTINUE){
		log_warning(((mate_posta *) lib_ref->group_info)->logger, "%s PID %d tuvo problemas al recibir la senial de continue", ((mate_posta *) lib_ref->group_info)->nombreCarpincho, ((mate_posta *) lib_ref->group_info)->pid);
		return false;
	}

	}else{
		if(((mate_posta *) lib_ref->group_info)->handshakeServidor == MEMORIA){

		}else{

			perror("Debe hacer mate_init antes de poder utilizar la funcion mate_memalloc de la matelib");
			return false;
		}
	}*/

	uint32_t direccionLogica;

	if(!send_memalloc(((mate_posta *) lib_ref->group_info)->socketCliente, size, ((mate_posta *) lib_ref->group_info)->pid)){
		log_warning(((mate_posta *) lib_ref->group_info)->logger, "%s PID %d no pudo enviar la instruccion de alocar memoria", ((mate_posta *) lib_ref->group_info)->nombreCarpincho, ((mate_posta *) lib_ref->group_info)->pid);
		return false;
	}

	if(!recv_direcLogica(((mate_posta *) lib_ref->group_info)->socketCliente, &direccionLogica)){
		log_warning(((mate_posta *) lib_ref->group_info)->logger, "%s PID %d no pudo recibir la direccion logica de memoria alocada", ((mate_posta *) lib_ref->group_info)->nombreCarpincho, ((mate_posta *) lib_ref->group_info)->pid);
		return false;
	}

	if(((mate_posta *) lib_ref->group_info)->handshakeServidor == KERNEL){

		sem_code autorizacion;

		//printf("ESPERANDO AUTORIZACION TRAS TERMINAR MATE_MEMALLOC PID %d\n", ((mate_posta *) lib_ref->group_info)->pid);

		if((recv(((mate_posta *) lib_ref->group_info)->socketCliente, &autorizacion, sizeof(sem_code), 0) != sizeof(sem_code)) || (autorizacion != CONTINUE)){
			log_warning(((mate_posta *) lib_ref->group_info)->logger, "%s PID %d tuvo problemas al recibir la senial de continue tras terminar mate_memalloc", ((mate_posta *) lib_ref->group_info)->nombreCarpincho, ((mate_posta *) lib_ref->group_info)->pid);
			return direccionLogica;
		}
	}

  return direccionLogica;
}

int mate_memfree(mate_instance *lib_ref, mate_pointer addr)
{

	/*if(((mate_posta *) lib_ref->group_info)->handshakeServidor == KERNEL){

	sem_code autorizacion;
	printf("ESPERANDO AUTORIZACION PARA MATE_MEMFREE PID %d\n", ((mate_posta *) lib_ref->group_info)->pid);
	if(recv(((mate_posta *) lib_ref->group_info)->socketCliente, &autorizacion, sizeof(sem_code), 0) != sizeof(sem_code) || autorizacion != CONTINUE){
		log_warning(((mate_posta *) lib_ref->group_info)->logger, "%s PID %d tuvo problemas al recibir la senial de continue", ((mate_posta *) lib_ref->group_info)->nombreCarpincho, ((mate_posta *) lib_ref->group_info)->pid);
		return false;
	}
	}else{
		if(((mate_posta *) lib_ref->group_info)->handshakeServidor == MEMORIA){

		}else{

			perror("Debe hacer mate_init antes de poder utilizar la funcion mate_memfree de la matelib");
			return false;
		}
	}*/

	int respuesta;

	if(!send_memfree(((mate_posta *) lib_ref->group_info)->socketCliente,(uint32_t) addr, ((mate_posta *) lib_ref->group_info)->pid)){
		log_warning(((mate_posta *) lib_ref->group_info)->logger, "%s PID %d no pudo enviar la instruccion de liberar memoria", ((mate_posta *) lib_ref->group_info)->nombreCarpincho, ((mate_posta *) lib_ref->group_info)->pid);
		return false;
	}

	if(recv(((mate_posta *) lib_ref->group_info)->socketCliente, &respuesta, sizeof(int),0) != sizeof(int)){
		log_warning(((mate_posta *) lib_ref->group_info)->logger, "%s PID %d no pudo recibir la respuesta de memfree de memoria", ((mate_posta *) lib_ref->group_info)->nombreCarpincho, ((mate_posta *) lib_ref->group_info)->pid);
		//return false;
		//return MATE_FREE_FAULT;
	}

	/*if(respuesta == -5){
		return MATE_FREE_FAULT;
	}*/

	if(((mate_posta *) lib_ref->group_info)->handshakeServidor == KERNEL){

		sem_code autorizacion;

		//printf("ESPERANDO AUTORIZACION TRAS TERMINAR MATE_MEMFREE PID %d\n", ((mate_posta *) lib_ref->group_info)->pid);

		if((recv(((mate_posta *) lib_ref->group_info)->socketCliente, &autorizacion, sizeof(sem_code), 0) != sizeof(sem_code)) || (autorizacion != CONTINUE)){
			log_warning(((mate_posta *) lib_ref->group_info)->logger, "%s PID %d tuvo problemas al recibir la senial de continue tras terminar mate_memfree", ((mate_posta *) lib_ref->group_info)->nombreCarpincho, ((mate_posta *) lib_ref->group_info)->pid);
		}
	}

	if(respuesta == -5){
		return MATE_FREE_FAULT;
	}else{
		return respuesta;
	}
}

int mate_memread(mate_instance *lib_ref, mate_pointer origin, void *dest, int size)
{

	/*if(((mate_posta *) lib_ref->group_info)->handshakeServidor == KERNEL){

	sem_code autorizacion;
	printf("ESPERANDO AUTORIZACION PARA MATE_MEMREAD PID %d\n", ((mate_posta *) lib_ref->group_info)->pid);

	if(recv(((mate_posta *) lib_ref->group_info)->socketCliente, &autorizacion, sizeof(sem_code), 0) != sizeof(sem_code) || autorizacion != CONTINUE){
		log_warning(((mate_posta *) lib_ref->group_info)->logger, "%s PID %d tuvo problemas al recibir la senial de continue", ((mate_posta *) lib_ref->group_info)->nombreCarpincho, ((mate_posta *) lib_ref->group_info)->pid);
		return false;
	}
	}else{
		if(((mate_posta *) lib_ref->group_info)->handshakeServidor == MEMORIA){

		}else{

			perror("Debe hacer mate_init antes de poder utilizar la funcion mate_memread de la matelib");
			return false;
		}
	}*/

	// el dest va a ser una variable que nos va a pasar el carpincho por parametro donde se va a guardar lo que se lee
	if(!send_memread(((mate_posta *) lib_ref->group_info)->socketCliente, origin, size, ((mate_posta *) lib_ref->group_info)->pid)){
		log_warning(((mate_posta *) lib_ref->group_info)->logger, "%s PID %d no pudo enviar la instruccion de leer memoria", ((mate_posta *) lib_ref->group_info)->nombreCarpincho, ((mate_posta *) lib_ref->group_info)->pid);
		return false;
	}

	int* respuesta = malloc(sizeof(int));
	recv(((mate_posta *) lib_ref->group_info)->socketCliente, respuesta, sizeof(int), 0);


	if(*respuesta != 1){
		printf("No pude recibir la respuesta de memread \n");
		return MATE_READ_FAULT;

	}
	free(respuesta);

	if(!recv_contenido_memread(((mate_posta *) lib_ref->group_info)->socketCliente, &dest, &size)){
		log_warning(((mate_posta *) lib_ref->group_info)->logger, "%s PID %d no pudo recibir contenido leido de memoria", ((mate_posta *) lib_ref->group_info)->nombreCarpincho, ((mate_posta *) lib_ref->group_info)->pid);
		printf("No pude recibir el contenido de memread \n");

		return MATE_READ_FAULT;
	}

	//char* mensaje = dest;
	//printf("El mensaje es %s \n", mensaje);

	if(((mate_posta *) lib_ref->group_info)->handshakeServidor == KERNEL){

		sem_code autorizacion;

		//printf("ESPERANDO AUTORIZACION TRAS TERMINAR MATE_MEMREAD PID %d\n", ((mate_posta *) lib_ref->group_info)->pid);

		if(recv(((mate_posta *) lib_ref->group_info)->socketCliente, &autorizacion, sizeof(sem_code), 0) != sizeof(sem_code) || autorizacion != CONTINUE){
			log_warning(((mate_posta *) lib_ref->group_info)->logger, "%s PID %d tuvo problemas al recibir la senial de continue tras terminar mate_memread", ((mate_posta *) lib_ref->group_info)->nombreCarpincho, ((mate_posta *) lib_ref->group_info)->pid);
			return true;
		}
	}

  return true;
}

int mate_memwrite(mate_instance *lib_ref, void *origin, mate_pointer dest, int size)
{

	/*if(((mate_posta *) lib_ref->group_info)->handshakeServidor == KERNEL){

	sem_code autorizacion;
	printf("ESPERANDO AUTORIZACION PARA MATE_MEMWRITE PID %d\n", ((mate_posta *) lib_ref->group_info)->pid);

	if(recv(((mate_posta *) lib_ref->group_info)->socketCliente, &autorizacion, sizeof(sem_code), 0) != sizeof(sem_code) || autorizacion != CONTINUE){
		log_warning(((mate_posta *) lib_ref->group_info)->logger, "%s PID %d tuvo problemas al recibir la senial de continue", ((mate_posta *) lib_ref->group_info)->nombreCarpincho, ((mate_posta *) lib_ref->group_info)->pid);
		return false;
	}
	}else{
		if(((mate_posta *) lib_ref->group_info)->handshakeServidor == MEMORIA){

		}else{

			perror("Debe hacer mate_init antes de poder utilizar la funcion mate_memwrite de la matelib");
			return false;
		}
	}*/

	int respuesta;

	// el origin va a ser una variable que nos va a pasar el carpincho por parametro para guardar
	if(!send_memwrite(((mate_posta *) lib_ref->group_info)->socketCliente,(uint32_t) dest, origin, size, ((mate_posta *) lib_ref->group_info)->pid)){
		log_warning(((mate_posta *) lib_ref->group_info)->logger, "%s PID %d no pudo enviar la instruccion de escribir en memoria", ((mate_posta *) lib_ref->group_info)->nombreCarpincho, ((mate_posta *) lib_ref->group_info)->pid);
		return false;
	}


	if(recv(((mate_posta *) lib_ref->group_info)->socketCliente, &respuesta, sizeof(int), 0) != sizeof(int)){
		log_warning(((mate_posta *) lib_ref->group_info)->logger, "%s PID %d no pudo recibir una respuesta bool de escritura de memoria", ((mate_posta *) lib_ref->group_info)->nombreCarpincho, ((mate_posta *) lib_ref->group_info)->pid);
		//return false;
		return MATE_WRITE_FAULT;
	}

	if(((mate_posta *) lib_ref->group_info)->handshakeServidor == KERNEL){

		sem_code autorizacion;

		//printf("ESPERANDO AUTORIZACION TRAS TERMINAR MATE_MEMWRITE PID %d\n", ((mate_posta *) lib_ref->group_info)->pid);

		if(recv(((mate_posta *) lib_ref->group_info)->socketCliente, &autorizacion, sizeof(sem_code), 0) != sizeof(sem_code) || autorizacion != CONTINUE){
			log_warning(((mate_posta *) lib_ref->group_info)->logger, "%s PID %d tuvo problemas al recibir la senial de continue tras terminar mate_memwrite", ((mate_posta *) lib_ref->group_info)->nombreCarpincho, ((mate_posta *) lib_ref->group_info)->pid);
			if(respuesta == -7){
					return MATE_WRITE_FAULT;
				}else{
					 return respuesta;
				}
		}
	}
	//printf("recibi la respuesta de memwrite que es: %d \n", respuesta);

	if(respuesta == -7){
		return MATE_WRITE_FAULT;
	}

  return respuesta;
}
