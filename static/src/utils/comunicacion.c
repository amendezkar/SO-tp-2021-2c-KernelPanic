/*
 * comunicacion.c
 *
 *  Created on: 7 sep. 2021
 *      Author: utnso
 */
#include "../../include/utils/comunicacion.h"

void enviar_mensaje(char* mensaje, int socket_cliente) {
    t_paquete* paquete = malloc(sizeof(t_paquete));

    paquete->codigo_operacion = MENSAJE;
    paquete->buffer = malloc(sizeof(t_buffer));
    paquete->buffer->size = strlen(mensaje) + 1;
    paquete->buffer->stream = malloc(paquete->buffer->size);
    memcpy(paquete->buffer->stream, mensaje, paquete->buffer->size);

    int bytes = paquete->buffer->size + 2*sizeof(int);

    void* a_enviar = serializar_paquete(paquete, bytes);

    send(socket_cliente, a_enviar, bytes, 0);

    free(a_enviar);
    eliminar_paquete(paquete);
}

handshake_code handshakeCliente(int socketServidor, uint32_t* pid) { // Solo lo usa el carpincho
    uint32_t handshake = 1;
    handshake_code resultadoHandshake;

    send(socketServidor, &handshake, sizeof(uint32_t), 0);
    recv(socketServidor, &resultadoHandshake, sizeof(uint32_t), MSG_WAITALL);

    //Identificarse o manejar error
    if(resultadoHandshake == ERROR_HANDSHAKE)
    {
        perror("Error de handshake");
        abort();
    }

    else{
    	realizar_operacion_segun(socketServidor, resultadoHandshake, pid);
    }



    return resultadoHandshake;
}

void handshakeServidor(int socketCliente, handshake_code codigo, t_log* logger) { // Lo usa el kernel pero tendriamos que ver que lo use memoria en caso de que no este el kernel, y si lo usa memoria que le envie el PID al carpincho, o puede ser por afuera de esta funcion
    uint32_t handshake;
    handshake_code resultError = ERROR_HANDSHAKE;

    recv(socketCliente, &handshake, sizeof(uint32_t), MSG_WAITALL);
    if(handshake == 1){
        send(socketCliente, &codigo, sizeof(handshake_code), 0);
        //recibir_mensaje(socketCliente, logger);
    }else{
        send(socketCliente, &resultError, sizeof(handshake_code), 0);
    }
}

void realizar_operacion_segun(int socketServidor, handshake_code resultadoHandshake, uint32_t* pid){

	op_code senial;

	if(resultadoHandshake == KERNEL){
		senial = CREAR_PCB;
	}else{
		senial = ASIGNAR_PID;
	}

	send(socketServidor, &senial, sizeof(op_code), 0);
	recv(socketServidor, pid, sizeof(uint32_t), 0);

}

void* serializar_paquete(t_paquete* paquete, int bytes) {
    void * magic = malloc(bytes);
    int desplazamiento = 0;

    memcpy(magic + desplazamiento, &(paquete->codigo_operacion), sizeof(int));
    desplazamiento+= sizeof(int);
    memcpy(magic + desplazamiento, &(paquete->buffer->size), sizeof(int));
    desplazamiento+= sizeof(int);
    memcpy(magic + desplazamiento, paquete->buffer->stream, paquete->buffer->size);
    desplazamiento+= paquete->buffer->size;

    return magic;
}

int recibir_operacion(int socket_cliente) {
    int cod_op;
    if(recv(socket_cliente, &cod_op, sizeof(int), MSG_WAITALL) != 0)
        return cod_op;
    else
    {
        close(socket_cliente);
        return -1;
    }
}

void recibir_mensaje(int socket_cliente,t_log* logger) {
    int size;
    char* buffer = recibir_buffer(&size, socket_cliente);
    log_info(logger, "Me llego el mensaje %s", buffer);
    free(buffer);
}

void* recibir_buffer(int* size, int socket_cliente) {
    void * buffer;

    recv(socket_cliente, size, sizeof(int), MSG_WAITALL);
    buffer = malloc(*size);
    recv(socket_cliente, buffer, *size, MSG_WAITALL);

    return buffer;
}

void eliminar_paquete(t_paquete* paquete) {
    free(paquete->buffer->stream);
    free(paquete->buffer);
    free(paquete);
}

//----------------------------------SERIALIZACIONES---------------------------------------

void* serializar_sem_init(size_t* size, mate_sem_name nombre, unsigned int value) {

	size_t sizeNombre = strlen(nombre) + 1;
	*size = sizeof(op_code) + //OP_CODE
			sizeof(size_t)*2 + //Tamanio del sizePayload y Tamanio de la variable que indica cuanto ocupa el nombre
			sizeNombre + //Tamanio del nombre
			sizeof(unsigned int); //Tamanio de la variable que tiene el valor

	size_t sizePayload = *size - sizeof(op_code) - sizeof(size_t); //Se le resta el tamanio del OP_CODE y del sizePayload

    void* stream = malloc(*size);

    op_code cop = SEM_INIT;

    memcpy(stream, &cop, sizeof(op_code));
    memcpy(stream+sizeof(op_code), &sizePayload, sizeof(size_t));
    memcpy(stream+sizeof(op_code)+sizeof(size_t), &sizeNombre, sizeof(size_t));
    memcpy(stream+sizeof(op_code)+sizeof(size_t)*2, nombre, sizeNombre);
    memcpy(stream+sizeof(op_code)+sizeof(size_t)*2+sizeNombre, &value, sizeof(unsigned int));

    return stream;
}

void* serializar_sem_wait(size_t* size, mate_sem_name nombre){

	size_t sizeNombre = strlen(nombre) + 1;
	*size = sizeof(op_code) + //OP_CODE
			sizeof(size_t)*2 + //Tamanio del sizePayload y Tamanio de la variable que indica cuanto ocupa el nombre
			sizeNombre; //Tamanio del nombre

	size_t sizePayload = *size - sizeof(op_code) - sizeof(size_t); //Se le resta el tamanio del OP_CODE y del sizePayload

    void* stream = malloc(*size);

    op_code cop = SEM_WAIT;

    memcpy(stream, &cop, sizeof(op_code));
    memcpy(stream+sizeof(op_code), &sizePayload, sizeof(size_t));
    memcpy(stream+sizeof(op_code)+sizeof(size_t), &sizeNombre, sizeof(size_t));
    memcpy(stream+sizeof(op_code)+sizeof(size_t)*2, nombre, sizeNombre);

    return stream;
}

void* serializar_sem_post(size_t* size, mate_sem_name nombre){

	size_t sizeNombre = strlen(nombre) + 1;
	*size = sizeof(op_code) + //OP_CODE
			sizeof(size_t)*2 + //Tamanio del sizePayload y Tamanio de la variable que indica cuanto ocupa el nombre
			sizeNombre; //Tamanio del nombre

	size_t sizePayload = *size - sizeof(op_code) - sizeof(size_t); //Se le resta el tamanio del OP_CODE y del sizePayload

    void* stream = malloc(*size);

    op_code cop = SEM_POST;

    memcpy(stream, &cop, sizeof(op_code));
    memcpy(stream+sizeof(op_code), &sizePayload, sizeof(size_t));
    memcpy(stream+sizeof(op_code)+sizeof(size_t), &sizeNombre, sizeof(size_t));
    memcpy(stream+sizeof(op_code)+sizeof(size_t)*2, nombre, sizeNombre);

    return stream;
}

void* serializar_sem_destroy(size_t* size, mate_sem_name nombre){

	size_t sizeNombre = strlen(nombre) + 1;
	*size = sizeof(op_code) + //OP_CODE
			sizeof(size_t)*2 + //Tamanio del sizePayload y Tamanio de la variable que indica cuanto ocupa el nombre
			sizeNombre; //Tamanio del nombre

	size_t sizePayload = *size - sizeof(op_code) - sizeof(size_t); //Se le resta el tamanio del OP_CODE y del sizePayload

    void* stream = malloc(*size);

    op_code cop = SEM_DESTROY;

    memcpy(stream, &cop, sizeof(op_code));
    memcpy(stream+sizeof(op_code), &sizePayload, sizeof(size_t));
    memcpy(stream+sizeof(op_code)+sizeof(size_t), &sizeNombre, sizeof(size_t));
    memcpy(stream+sizeof(op_code)+sizeof(size_t)*2, nombre, sizeNombre);

    return stream;
}

void* serializar_call_io(size_t* size, mate_io_resource io, char* msg){

	size_t sizeNombreIO = strlen(io) + 1;
	size_t sizeMsg = strlen(msg) + 1;
	*size = sizeof(op_code) + //Tamanio del op_code
			sizeof(size_t)*2 + //Tamanio del sizePayload y Tamanio de la variable que indica cuanto ocupa el nombre del io
			sizeNombreIO + //Tamanio del nombre del io
			sizeof(size_t) +
			sizeMsg; //Tamanio del msg

	size_t sizePayload = *size - sizeof(op_code) - sizeof(size_t);

	void* stream = malloc(*size);

	op_code cop = CALL_IO;

	memcpy(stream, &cop, sizeof(op_code));
	memcpy(stream+sizeof(op_code), &sizePayload, sizeof(size_t));
	memcpy(stream+sizeof(op_code)+sizeof(size_t), &sizeNombreIO, sizeof(size_t));
	memcpy(stream+sizeof(op_code)+sizeof(size_t)*2, io, sizeNombreIO);
	memcpy(stream+sizeof(op_code)+sizeof(size_t)*2+sizeNombreIO, &sizeMsg, sizeof(size_t));
	memcpy(stream+sizeof(op_code)+sizeof(size_t)*3+sizeNombreIO, msg, sizeMsg);

	return stream;
}

void* serializar_memalloc(size_t* sizeStream, int size, uint32_t pid) {

    //// Stream completo
	*sizeStream =
        sizeof(op_code)+                        // COP
        sizeof(int)+						// stream que seria solo el size que se quiere alocar
		sizeof(uint32_t)
    ;
    void* stream = malloc(*sizeStream);

    op_code cop = MEMALLOC;

    memcpy(stream, &cop, sizeof(op_code));
    memcpy(stream+sizeof(op_code), &size, sizeof(int));
    memcpy(stream+sizeof(op_code)+sizeof(int), &pid, sizeof(uint32_t));

    return stream;
}

void* serializar_memfree(size_t* sizeStream, uint32_t direccionLogica, uint32_t pid) {

    //// Stream completo
	*sizeStream =
        sizeof(op_code)+                        // COP
        sizeof(uint32_t)*2						// direccion y pid
    ;
    void* stream = malloc(*sizeStream);

    op_code cop = MEMFREE;

    memcpy(stream, &cop, sizeof(op_code)); //cop
    memcpy(
        stream+sizeof(op_code),
        &direccionLogica,
        sizeof(uint32_t)
    );
    memcpy(
		stream+sizeof(op_code)+sizeof(uint32_t),
		&pid,
		sizeof(uint32_t)
        );

    return stream;
}

void* serializar_memread(size_t* sizeStream, uint32_t direccionLogica, int size, uint32_t pid) {

    //// Stream completo
	*sizeStream =
        sizeof(op_code)+                        // COP
        sizeof(uint32_t)+						// direccion
		sizeof(int)+						// tamanio a leer
		sizeof(uint32_t)						//pid
    ;
    void* stream = malloc(*sizeStream);

    op_code cop = MEMREAD;

    memcpy(stream, &cop, sizeof(op_code)); //cop
    memcpy(
        stream+sizeof(op_code),
        &direccionLogica,
        sizeof(uint32_t)
    );
    memcpy(
		stream+sizeof(op_code)+sizeof(uint32_t),
		&size,
		sizeof(int)
        );
    memcpy(
		stream+sizeof(op_code)+sizeof(uint32_t)+sizeof(int),
		&pid,
		sizeof(uint32_t)
		);


    return stream;
}

void* serializar_memwrite(size_t* sizeStream, uint32_t direccionLogica, void* contenido, int size, uint32_t pid) {

    //// Stream completo
	*sizeStream =
        sizeof(op_code)+                        // 	COP
        sizeof(uint32_t)*2+						// 	direccion y pid
		sizeof(int)+							// 	tamanio del contenido
		size									//	contenido
    ;
    void* stream = malloc(*sizeStream);

    op_code cop = MEMWRITE;

    memcpy(stream, &cop, sizeof(op_code)); //cop
    memcpy(
        stream+sizeof(op_code),
        &direccionLogica,
        sizeof(uint32_t)
    );
    memcpy(
		stream+sizeof(op_code)+sizeof(uint32_t),
		&pid,
		sizeof(uint32_t)
        );
    memcpy(
		stream+sizeof(op_code)+sizeof(uint32_t)*2,
		&size,
		sizeof(int)
    );
    memcpy(
		stream+sizeof(op_code)+sizeof(uint32_t)*2+sizeof(int),
		contenido,
		size
    );

    return stream;
}


void* serializar_contenido_memread(size_t* sizeStream,void* contenido, int size){

	*sizeStream =
			sizeof(int)+							// 	tamanio contenido
			size									//	contenido
	    ;
	    void* stream = malloc(*sizeStream);

	    memcpy(
	        stream,
	        &size,
	        sizeof(int)
	    );
	    memcpy(
			stream+sizeof(int),
			contenido,
			size
	    );


	    return stream;

}


//----------------------------------DESERIALIZACIONES-----------------------------------------

void deserializar_memalloc(void* stream, int* size, uint32_t* pid) {

    memcpy(size, stream, sizeof(int));
    memcpy(pid, stream+sizeof(int), sizeof(uint32_t));

}

void deserializar_memfree(void* stream, uint32_t* direccionLogica, uint32_t* pid) {

    memcpy(direccionLogica, stream, sizeof(uint32_t));
    memcpy(pid, stream+sizeof(uint32_t), sizeof(uint32_t));



}

//por ahora no se usa
void deserializar_memread(void* stream, uint32_t* direccionLogica, int* size, uint32_t* pid) {

    memcpy(direccionLogica, stream, sizeof(uint32_t));
    memcpy(size, stream+sizeof(uint32_t), sizeof(int));
    memcpy(pid, stream+sizeof(uint32_t)+sizeof(int), sizeof(uint32_t));
}

void deserializar_memwrite(void* stream, uint32_t* direccionLogica,void** contenido, int* size, uint32_t* pid) {

	memcpy(direccionLogica, stream, sizeof(uint32_t));

	memcpy(pid, stream, sizeof(uint32_t));

	memcpy(size, stream+sizeof(uint32_t), sizeof(int));

	memcpy(contenido, stream+sizeof(uint32_t)+sizeof(int),(size_t) *size);

}

void deserializar_contenido_memread(void* stream, void** contenido, int* size) {

    memcpy(size, stream, sizeof(int));

    memcpy(contenido, stream+sizeof(int),(size_t) *size);

}



//---------------------------------RECV------------------------------------------------

bool recv_memalloc(int socketCliente,int* size,uint32_t* pid) {
	void* stream = malloc(sizeof(uint32_t)+sizeof(int));

	if (recv(socketCliente, stream, (sizeof(uint32_t)+sizeof(int)), 0) != (sizeof(uint32_t)+sizeof(int))) {
	        free(stream);
	        return false;
	    }

	deserializar_memalloc(stream, size, pid);

	free(stream);
	return true;
}

bool recv_memfree(int socketCliente,uint32_t* direcLogica, uint32_t* pid) {
	void* stream = malloc(sizeof(uint32_t)*2);

	if (recv(socketCliente, stream, sizeof(uint32_t)*2, 0) != sizeof(uint32_t)*2) {
	        free(stream);
	        return false;
	    }

	deserializar_memfree(stream, direcLogica, pid);

	free(stream);
	return true;
}

bool recv_memread(int socketCliente,uint32_t* direcLogica,int* size, uint32_t* pid) {
	void* stream = malloc(sizeof(uint32_t)*2 + sizeof(int));

		if (recv(socketCliente, stream, sizeof(uint32_t)*2 + sizeof(int), 0) != sizeof(uint32_t)*2 + sizeof(int)) {
		        free(stream);
		        return false;
		    }

		deserializar_memread(stream, direcLogica, size, pid);

		free(stream);
		return true;
}

bool recv_memwrite(int socketCliente,uint32_t* direcLogica,void** contenido, int* size, uint32_t* pid) {

	if (recv(socketCliente, direcLogica, sizeof(uint32_t), 0) != sizeof(uint32_t)) {

			return false;
	}

	if (recv(socketCliente, pid, sizeof(uint32_t), 0) != sizeof(uint32_t)) {

			return false;
	}

	*contenido = recibir_buffer2(socketCliente,size);

	return true;
}

bool recv_direcLogica(int socketCliente, uint32_t* direcLogica){

	void* stream = malloc(sizeof(uint32_t));

		if (recv(socketCliente, stream, sizeof(uint32_t), 0) != sizeof(uint32_t)) {
		        free(stream);
		        return false;
		    }

		 memcpy(direcLogica, stream, sizeof(uint32_t));

		free(stream);
		return true;
}

bool recv_respuesta_bool(int socketCliente, bool* respuesta) {
    void* stream = malloc(sizeof(bool));
    if (recv(socketCliente, stream, sizeof(bool), 0) != sizeof(bool)) {
        free(stream);
        return false;
    }
    memcpy(respuesta, stream, sizeof(bool));

    free(stream);
    return true;
}

bool recv_contenido_memread(int socketCliente, void** contenido, int* size){


	recv(socketCliente, size, sizeof(int), MSG_WAITALL);

	char* contenidoRecibido = malloc(*size);
	recv(socketCliente, contenidoRecibido,(size_t) *size, MSG_WAITALL);


	memcpy(*contenido, contenidoRecibido, *size);

	free(contenidoRecibido);
	//char* mensaje = malloc(38);
	//mensaje = *contenido;
	//printf("El mensaje recibido es %s \n", mensaje);
	//void* contenidoAux = recibir_buffer2(socketCliente,size);


	return true;
/*
	int size;
	char* buffer = recibir_buffer(&size, socketCliente);
	log_info(logger, "Me llego el mensaje %s", buffer);

	free(buffer);




	int sizeContenido;

	*contenido = recibir_buffer(&sizeContenido, socketCliente);

	//memcpy(*contenido,buffer, sizeContenido);
	//free(buffer);
	return true;
*/

}

void* recibir_buffer2(int socketCliente, int* size){
	void * buffer;

	recv(socketCliente, size, sizeof(int), MSG_WAITALL);
	buffer = malloc(*size);
	recv(socketCliente, buffer,(size_t) *size, MSG_WAITALL);

	//printf("Me llego el size %d \n", *size);
	//printf("Me llego el mensaje %d \n", *b);


	return buffer;
}

//---------------------------------SEND------------------------------------------------

bool send_memalloc(int socketCliente, int size, uint32_t pid) {

    size_t sizeStream;
    void* stream = serializar_memalloc(&sizeStream, size, pid);
    if (send(socketCliente, stream, sizeStream, 0) == -1) {
        free(stream);
        return false;
    }
    free(stream);
    return true;
}

bool send_memfree(int socketCliente, uint32_t direcLogica, uint32_t pid) {

    size_t sizeStream;
    void* stream = serializar_memfree(&sizeStream, direcLogica, pid);
    if (send(socketCliente, stream, sizeStream, 0) == -1) {
        free(stream);
        return false;
    }
    free(stream);
    return true;
}

bool send_memread(int socketCliente, uint32_t direcLogica, int size, uint32_t pid) {

    size_t sizeStream;
    void* stream = serializar_memread(&sizeStream, direcLogica, size, pid);
    if (send(socketCliente, stream, sizeStream, 0) == -1) {
        free(stream);
        return false;
    }
    free(stream);
    return true;
}

bool send_memwrite(int socketCliente, uint32_t direcLogica, void* contenido,int size, uint32_t pid) {

    size_t sizeStream;
    void* stream = serializar_memwrite(&sizeStream, direcLogica,contenido, size, pid);
    if (send(socketCliente, stream, sizeStream, 0) == -1) {
        free(stream);
        return false;
    }
    free(stream);
    return true;
}

bool send_direcLogica(int socketCliente, uint32_t direcLogica) {

	 void* stream = malloc(sizeof(uint32_t));
	 memcpy(stream, &direcLogica, sizeof(uint32_t));
	 if (send(socketCliente, stream, sizeof(uint32_t), 0) == -1) {
	     free(stream);
	     return false;
	  }
	 free(stream);
	 return true;

}

bool send_respuesta_bool(int socketCliente, bool respuesta) {
    void* stream = malloc(sizeof(bool));
    memcpy(stream, &respuesta, sizeof(bool));
    if (send(socketCliente, stream, sizeof(bool), 0) == -1) {
        free(stream);
        return false;
    }
    free(stream);
    return true;
}

bool send_contenido_memread(int socketCliente, void* contenido, int size){
	 size_t sizeStream;
	void* stream = serializar_contenido_memread(&sizeStream, contenido,size);


	if (send(socketCliente, stream, sizeStream, 0) == -1) {
		free(stream);
		return false;
	}
	free(stream);
	return true;

}



