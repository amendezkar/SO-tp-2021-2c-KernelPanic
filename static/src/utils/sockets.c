/*
 * sockets.c
 *
 *  Created on: 6 sep. 2021
 *      Author: utnso
 */
#include "utils/sockets.h"

//CLIENTE
int crear_conexion(t_log* logger, const char* server_name, char* ip, char* puerto) {
    struct addrinfo hints, *servinfo;

    // Init de hints
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    // Recibe addrinfo
    getaddrinfo(ip, puerto, &hints, &servinfo);

    // Crea un socket con la informacion recibida (del primero, suficiente)
    int socket_cliente = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);

    // Fallo en crear el socket
    if(socket_cliente == -1) {
        log_error(logger, "Error creando el socket para %s:%s", ip, puerto);
        return 0;
    }

    // Error conectando
    if(connect(socket_cliente, servinfo->ai_addr, servinfo->ai_addrlen) == -1) {
        log_error(logger, "Error al conectar (a %s)\n", server_name);
        freeaddrinfo(servinfo);
        return 0;
    } else{
        log_info(logger, "Cliente conectado en %s:%s (a %s)\n", ip, puerto, server_name);
    }

    freeaddrinfo(servinfo); //free

    return socket_cliente;
}

void liberar_conexion(int socket_cliente) {
    close(socket_cliente);
}


//SERVIDOR
int iniciar_servidor(t_log* logger, const char* name, char* ip, char* puerto) {
	int socket_servidor;

	    struct addrinfo hints, *servinfo;

	    memset(&hints, 0, sizeof(hints));
	    hints.ai_family = AF_UNSPEC;
	    hints.ai_socktype = SOCK_STREAM;
	    hints.ai_flags = AI_PASSIVE;

	    getaddrinfo(ip, puerto, &hints, &servinfo);


	    socket_servidor = socket(servinfo->ai_family,
	                                 servinfo->ai_socktype,
	                                 servinfo->ai_protocol);

	    bind(socket_servidor, servinfo->ai_addr, servinfo->ai_addrlen);

		listen(socket_servidor, SOMAXCONN);

	    freeaddrinfo(servinfo);

	    log_info(logger, "Listo para escuchar a mi cliente");

	    return socket_servidor;
}

int esperar_cliente(t_log* logger, const char* name, int socket_servidor) {
    struct sockaddr_in dir_cliente;
    socklen_t tam_direccion = sizeof(struct sockaddr_in);

    int socket_cliente = accept(socket_servidor, (void*) &dir_cliente, &tam_direccion);

    log_info(logger, "Cliente conectado a %s\n", name);

    return socket_cliente;
}
