#include "../include/main.h"
#include <signal.h>

t_log *logger;
swamp_info info;
int socketMemoria;

void intHandler(int signum)
{
	log_info(logger, "SIGINT RECIBIDO");
	for (int i = 0; i < info.archivosSwamp->elements_count; i++)
	{
		swamp_file *file = list_get(info.archivosSwamp, i);
		free(file->path);
		free(file->pages);
		//list_destroy_and_destroy_elements(file->processes, free);
		for (int j = 0; j < file->processes->elements_count; j++)
		{
			free(list_get(file->processes, j));
		}
		list_destroy(file->processes);
	}

	list_destroy_and_destroy_elements(info.archivosSwamp, free);
	log_info(logger, "MEMORIA LIBERADA");
	log_destroy(logger);
	close(socketMemoria);
	exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[])
{
	signal(SIGINT, intHandler);

	info = get_info();
	logger = iniciar_logger("bin/swamp.log", "swamp", true, LOG_LEVEL_INFO);

	init_files(info.tamanioSwap, info.archivosSwamp);

	start_server_and_listen(logger, info);

	log_destroy(logger);
	return EXIT_SUCCESS;
}

swamp_info get_info()
{
	swamp_info output;
	t_config *config = iniciar_config("cfg/swamp.config");

	strcpy(output.puerto, obtener_de_config(config, "PUERTO"));
	strcpy(output.ip, obtener_de_config(config, "IP"));
	output.tamanioSwap = config_get_int_value(config, "TAMANIO_SWAP");
	output.tamanioPagina = config_get_int_value(config, "TAMANIO_PAGINA");
	output.paginasPorArchivo = output.tamanioSwap / output.tamanioPagina;
	output.marcosMaximos = config_get_int_value(config, "MARCOS_MAXIMOS");
	output.retardoSwap = config_get_int_value(config, "RETARDO_SWAP");

	output.archivosSwamp = list_create();
	char **aux = config_get_array_value(config, "ARCHIVOS_SWAP");
	for (int i = 0; aux[i] != 0; i++)
	{
		swamp_file *file = malloc(sizeof(swamp_file));
		file->pagesAvailable = output.paginasPorArchivo;
		file->path = aux[i];
		file->processes = list_create();
		file->pages = malloc(sizeof(swamp_map) * output.paginasPorArchivo);
		for (int j = 0; j < output.paginasPorArchivo; j++)
		{
			file->pages[j].pageNumber = -1;
			file->pages[j].pid = -1;
		}

		list_add(output.archivosSwamp, file);
	}
	free(aux);

	config_destroy(config);
	return output;
}

void start_server_and_listen(t_log *logger, swamp_info info)
{
	int socketSwamp = iniciar_servidor(logger, "swamp", info.ip, info.puerto);
	log_info(logger, "Swamp listo para recibir a la memoria");
	MEM_SWAP_MESSAGE cod_op;
	while ((socketMemoria = esperar_cliente(logger, "swamp", socketSwamp)))
	{
		bool alive = true;
		recieve_and_set_mode();
		while ((cod_op = recibir_operacion(socketMemoria)) && alive)
		{
			switch (cod_op)
			{
			case GET_PAGE_SIZE:
				process_get_page_size(socketMemoria);
				break;
			case DELETE_PAGE:
				process_delete_page(socketMemoria);
				break;
			case ADD_PAGE:
				process_add_page(socketMemoria);
				break;
			case ADD_PAGES:
				process_add_pages(socketMemoria);
				break;
			case READ_PAGE:
				process_read_page(socketMemoria);
				break;
			case RELEASE_PROCESS:
				process_release_process(socketMemoria);
				break;
			case IS_SPACE_SUFFICIENT:
				process_is_space_sufficient(socketMemoria);
				break;
			case CLOSE_CONNECTION:
				alive = false;
				break;
			default:
				log_warning(logger,
							"Operacion desconocida. Terminando el proceso");
				exit(EXIT_FAILURE);
			}

			usleep(info.retardoSwap);
		}
	}
}

void recieve_and_set_mode()
{
	MEM_SWAP_MESSAGE mode;
	recv(socketMemoria, &mode, sizeof(MEM_SWAP_MESSAGE), 0);
	if (mode == FIXED_ASIGN)
	{
		log_info(logger, "Asignacion fija.");
		set_fixed_asign();
	}
	else if (mode == GLOBAL_ASIGN)
	{
		log_info(logger, "Asignacion global.");
	}
	else
	{
		log_error(logger, "Modo de asignacion no recibido.");
	}
}

void process_delete_page(int socketClient)
{
	uint32_t pid;
	int pageNumber;
	recv(socketClient, &pid, sizeof(uint32_t), MSG_WAITALL);
	recv(socketClient, &pageNumber, sizeof(int), MSG_WAITALL);

	MEM_SWAP_MESSAGE result = delete_page(pid, pageNumber);
	send(socketClient, &result, sizeof(MEM_SWAP_MESSAGE), 0);
	if(result == OK) log_info(logger, "Se elimino la pagina %d del proceso %d", pageNumber, pid);

}

void process_get_page_size(int socketClient)
{
	send(socketClient, &info.tamanioPagina, sizeof(int), 0);
}

void init_files(int swapSize, t_list *files)
{
	int numberOfFiles = files->elements_count;
	for (int i = 0; i < numberOfFiles; i++)
	{
		swamp_file *file = (swamp_file *)list_get(files, i);
		if (fopen(file->path, "w") == NULL)
		{
			log_error(logger, "Error al crear archivos");
			exit(EXIT_FAILURE);
		}

		if (truncate(file->path, swapSize) == -1)
		{
			log_error(logger, "Error al inicializar archivos");
			exit(EXIT_FAILURE);
		}
	}
}

void process_add_page(int socketClient)
{
	uint32_t pid;
	int pageNumber, size;

	recv(socketClient, &pid, sizeof(uint32_t), MSG_WAITALL);
	recv(socketClient, &pageNumber, sizeof(int), MSG_WAITALL);
	recv(socketClient, &size, sizeof(int), MSG_WAITALL);

	void *buffer = malloc(size);
	recv(socketClient, buffer, size, MSG_WAITALL);

	if (size != info.tamanioPagina)
	{
		log_error(logger, "Tamaño de pagina incorrecto.");
		MEM_SWAP_MESSAGE result = ERROR_INCORRECT_PAGE_SIZE;
		send(socketClient, &result, sizeof(MEM_SWAP_MESSAGE), 0);
	}
	else
	{
		MEM_SWAP_MESSAGE result = add_page(pid, pageNumber, buffer, size);
		send(socketClient, &result, sizeof(MEM_SWAP_MESSAGE), 0);
	}

	free(buffer);
}

void process_is_space_sufficient(int socketClient)
{
	uint32_t pid;
	int amount;

	recv(socketClient, &pid, sizeof(uint32_t), MSG_WAITALL);
	recv(socketClient, &amount, sizeof(int), MSG_WAITALL);

	if (is_there_space_available(pid, amount) == false)
	{
		MEM_SWAP_MESSAGE result = ERROR_NO_SPACE_IN_PROCESS_FILE;
		send(socketClient, &result, sizeof(MEM_SWAP_MESSAGE), 0);
		//log_info(logger, "Hay espacio disponible para el proceso %d? No", pid);
	}
	else
	{
		MEM_SWAP_MESSAGE sufficientSpace = SUFFICIENT_SPACE;
		send(socketClient, &sufficientSpace, sizeof(MEM_SWAP_MESSAGE), 0);
		//log_info(logger, "Hay espacio disponible para el proceso %d? Si", pid);
	}
}

void process_add_pages(int socketClient)
{
	uint32_t pid;
	int amount, pageNumber, size;

	recv(socketClient, &pid, sizeof(uint32_t), MSG_WAITALL);
	recv(socketClient, &amount, sizeof(int), MSG_WAITALL);

	if (is_there_space_available(pid, amount) == false)
	{
		MEM_SWAP_MESSAGE result = ERROR_NO_SPACE_IN_PROCESS_FILE;
		send(socketClient, &result, sizeof(MEM_SWAP_MESSAGE), 0);
	}
	else
	{
		log_info(logger, "Reservando %d paginas para el proceso %d", amount, pid);
		MEM_SWAP_MESSAGE sufficientSpace = SUFFICIENT_SPACE;
		send(socketClient, &sufficientSpace, sizeof(MEM_SWAP_MESSAGE), 0);

		for (int i = 0; i < amount; i++)
		{
			recv(socketClient, &pageNumber, sizeof(int), MSG_WAITALL);
			recv(socketClient, &size, sizeof(int), MSG_WAITALL);

			void *buffer = malloc(size);
			recv(socketClient, buffer, size, MSG_WAITALL);

			if (size != info.tamanioPagina)
			{
				log_error(logger, "Tamaño de pagina incorrecto.");
				MEM_SWAP_MESSAGE result = ERROR_INCORRECT_PAGE_SIZE;
				send(socketClient, &result, sizeof(MEM_SWAP_MESSAGE), 0);
			}
			else
			{
				MEM_SWAP_MESSAGE result = add_page(pid, pageNumber, buffer, size);
				send(socketClient, &result, sizeof(MEM_SWAP_MESSAGE), 0);
			}

			free(buffer);
		}
	}
}

void process_read_page(int socketClient)
{
	uint32_t pid;
	int pageNumber;
	recv(socketClient, &pid, sizeof(uint32_t), 0);
	recv(socketClient, &pageNumber, sizeof(int), 0);

	void *buffer;
	MEM_SWAP_MESSAGE result = get_page(pid, pageNumber, &buffer);

	send(socketClient, &result, sizeof(MEM_SWAP_MESSAGE), 0);
	if (result == PAGE_READ)
	{
		log_info(logger, "Se leyo la pagina %d del proceso %d", pageNumber, pid);
		send(socketClient, &info.tamanioPagina, sizeof(int), 0);
		send(socketClient, buffer, info.tamanioPagina, 0);
	}

	free(buffer);
}

void process_release_process(int socketClient)
{
	int pid;
	recv(socketClient, &pid, sizeof(int), MSG_WAITALL);

	int result = release_process(pid);
	send(socketClient, &result, sizeof(MEM_SWAP_MESSAGE), 0);
}
