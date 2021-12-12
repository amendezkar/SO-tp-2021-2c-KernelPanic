#include "../include/main.h"
#include "utils/mensajes_mem_swamp.h"

int socket_cliente;
t_log* logger;

void intHandler(int signum) {
	MEM_SWAP_MESSAGE msg = CLOSE_CONNECTION;
	send(socket_cliente, &msg, sizeof(MEM_SWAP_MESSAGE), 0);
    close(socket_cliente);
	exit(EXIT_SUCCESS);
}


int main(int argc, char* argv[]) {

	signal(SIGINT, intHandler);
	 logger = log_create("swamp-test/bin/swamp-test.log", "Servidor", 1, LOG_LEVEL_DEBUG);


	socket_cliente = crear_conexion(logger, "swamp-server", "127.0.0.1", "5555");
	if(!socket_cliente) exit(EXIT_FAILURE);

	MEM_SWAP_MESSAGE mode = FIXED_ASIGN;
	send(socket_cliente, &mode, sizeof(MEM_SWAP_MESSAGE), 0);

	int pagesize;
	MEM_SWAP_MESSAGE men = GET_PAGE_SIZE;
	send(socket_cliente, &men, sizeof(MEM_SWAP_MESSAGE), 0);
	recv(socket_cliente, &pagesize, sizeof(int), MSG_WAITALL);

	char* str = malloc(100);
	sprintf(str, "Tamanio de pagina de swamp: %d", pagesize);
	log_info(logger, str);
	char mensaje[pagesize];
	memset(mensaje, 0, pagesize);
	strcpy(mensaje, "MAMA MIA");
	send_add_page(69, 1, mensaje, pagesize, socket_cliente);
	send_add_page(69, 2, mensaje, pagesize, socket_cliente);
	send_add_page(62, 1, mensaje, pagesize, socket_cliente);
	send_add_pages(63, 10, "QUE MALA LECHE!", strlen("QUE MALA LECHE!") + 1, socket_cliente);

	MEM_SWAP_MESSAGE cod_op;
	while((cod_op = recibir_operacion(socket_cliente))){
		switch (cod_op)
		{
		case PAGE_ADDED:
			log_debug(logger, "PAGINA AÑADIDA");
			send_release_process(socket_cliente, 0);
			break;
		case PAGE_READ:
			log_debug(logger, "PAGINA LEIDA");
			int size;
			recv(socket_cliente, &size, sizeof(int), 0);
			if(size != 16) {
				log_error(logger, "Tamano de pagina incorrecto");
			}else{
				void* buffer = NULL;
				recv(socket_cliente, buffer, 16, 0);
				log_debug(logger, buffer);
			}
			break;
		case PROCESS_RELEASED:
			log_debug(logger, "PROCESO LIBERADO");
			break;
		case ERROR_COULD_NOT_OPEN_FILE:
		case ERROR_COULD_NOT_USE_FILE:
		case ERROR_NO_SPACE_IN_PROCESS_FILE:
		case ERROR_NO_SPACE_IN_SWAMP:
		default:
			log_error(logger, "Upsis?");
			close(socket_cliente);
			exit(EXIT_FAILURE);
			break;
		}
	}
	liberar_conexion(socket_cliente);
	return EXIT_SUCCESS;
}


void send_add_pages(int pid, int amountOfPages, void* data, int size, int socket_cliente) {
	MEM_SWAP_MESSAGE addpages = ADD_PAGES;
	log_info(logger, "Anadir pagians enviado.");
	send(socket_cliente, &addpages, sizeof(MEM_SWAP_MESSAGE), 0);
	send(socket_cliente, &pid, sizeof(int), 0);
	send(socket_cliente, &amountOfPages, sizeof(int), 0);

	MEM_SWAP_MESSAGE space;
	recv(socket_cliente, &space, sizeof(MEM_SWAP_MESSAGE), MSG_WAITALL);
	if(space == ERROR_NO_SPACE_IN_PROCESS_FILE)
	{
		log_error(logger, "No hay espacio en el archivo de proceso para esta cantidad de paginas.");
	}
	else if(space == SUFFICIENT_SPACE)
	{
		for (int i = 0; i < amountOfPages; i++)
		{
			send(socket_cliente, &i, sizeof(int), 0);
			send(socket_cliente, &size, sizeof(int), 0);
			send(socket_cliente, data, size, 0);

			MEM_SWAP_MESSAGE result;
			recv(socket_cliente, &result, sizeof(MEM_SWAP_MESSAGE), MSG_WAITALL);
			if(result == PAGE_ADDED)
				log_info(logger, "Pagina anadida");
			else
				log_error(logger, "Error al anadir pagina");
		}
	}

}

void send_add_page(int pid, int pageNumber, void* data, int size, int socket_cliente) {

	MEM_SWAP_MESSAGE cod_op = ADD_PAGE;

	send(socket_cliente, &cod_op, sizeof(MEM_SWAP_MESSAGE), 0);
	send(socket_cliente, &pid, sizeof(int), 0);
	send(socket_cliente, &pageNumber, sizeof(int), 0);
	send(socket_cliente, &size, sizeof(int), 0);
	send(socket_cliente, data, size, 0);

	MEM_SWAP_MESSAGE response;
	recv(socket_cliente, &response, sizeof(MEM_SWAP_MESSAGE), MSG_WAITALL);
}

void send_get_page(int pid, int pageNumber, int socket_cliente) {
	MEM_SWAP_MESSAGE cod_op = READ_PAGE;
	send(socket_cliente, &cod_op, sizeof(MEM_SWAP_MESSAGE), 0);
	send(socket_cliente, &pid, sizeof(int), 0);
	send(socket_cliente, &pageNumber, sizeof(int), 0);
}

void send_release_process(int socketClient, int pid){
	MEM_SWAP_MESSAGE message = RELEASE_PROCESS;
	send(socketClient, &message, sizeof(MEM_SWAP_MESSAGE), 0);
	send(socketClient, &pid, sizeof(int), 0);
}

void enviar_buffer(MEM_SWAP_MESSAGE cod_op, void* data, int size, int socket_cliente) {
	t_paquete* paquete = malloc(sizeof(t_paquete));

	paquete->codigo_operacion = cod_op;
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = size;
	paquete->buffer->stream = malloc(paquete->buffer->size);
	memcpy(paquete->buffer->stream, data, size);

	int bytes = paquete->buffer->size + 2*sizeof(int);

	void* a_enviar = serializar_paquete(paquete, bytes);

	send(socket_cliente, a_enviar, bytes, 0);

	free(a_enviar);
	eliminar_paquete(paquete);
}