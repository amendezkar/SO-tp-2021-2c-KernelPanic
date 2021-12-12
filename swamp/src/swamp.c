#include "../include/swamp.h"

bool fixed_mode = false;

MEM_SWAP_MESSAGE add_page(uint32_t pid, int pageNumber, void *data, int size)
{
	int isNewProcess = 0;
	swamp_file *file = find_file_by_process(info.archivosSwamp, pid);

	if (file == NULL)
	{
		isNewProcess = 1;
		file = find_available_file(info.archivosSwamp);
		if (file == NULL)
		{
			log_error(logger, "No hay espacio disponible para la pagina.");
			return ERROR_NO_SPACE_IN_SWAMP;
		}
	}

	int frame = find_frame_by_page(file, pid, pageNumber, info.paginasPorArchivo);
	MEM_SWAP_MESSAGE result;
	if (frame == -1)
	{
		if (file->pagesAvailable < 1)
		{
			log_error(logger, "El archivo que contiene este proceso no tiene espacio.");
			return ERROR_NO_SPACE_IN_PROCESS_FILE;
		}
		result = write_first_available_frame(file, data, size, info.tamanioPagina, info.paginasPorArchivo, &frame);
		if(result == OK)
		{
			file->pagesAvailable -= 1;
			log_info(logger, "Se agrego la pagina %d del proceso %d", pageNumber, pid);
		}
	}
	else
	{
		log_info(logger, "Se actualizo la pagina %d del proceso %d", pageNumber, pid);
		result = write_file(file->path, frame, data, size, info.tamanioPagina);
	}

	if (result != OK)
		return result;

	file->pages[frame].pageNumber = pageNumber;
	file->pages[frame].pid = pid;

	if (isNewProcess)
	{
		uint32_t *processPointer = malloc(sizeof(uint32_t));
		*processPointer = pid;
		list_add(file->processes, processPointer);
	}

	return PAGE_ADDED;
}

void set_fixed_asign()
{
	fixed_mode = true;
}

swamp_file *find_file_by_process(t_list *files, uint32_t pid)
{
	for (int i = 0; i < files->elements_count; i++)
	{
		swamp_file *file = (swamp_file *)list_get(files, i);
		for (int i = 0; i < file->processes->elements_count; i++)
		{
			int *auxPid = list_get(file->processes, i);
			if (*auxPid == (int)pid)
				return file;
		}
	}

	return NULL;
}

bool is_there_space_available(uint32_t pid, int amountOfPages)
{
	swamp_file *processFile = find_file_by_process(info.archivosSwamp, pid);

	int usedPages = 0;
	if (fixed_mode && processFile != NULL)
		usedPages = number_of_pages_from_process(pid, processFile);
	bool belowMaxFrames = !fixed_mode ||
	(amountOfPages + usedPages <= info.marcosMaximos || info.marcosMaximos == 0);

	if (belowMaxFrames == false)
		return false;

	if (processFile != NULL)
	{
		if (processFile->pagesAvailable >= amountOfPages)
			return true;
		else
			return false;
	}
	else
	{
		for (int i = 0; i < info.archivosSwamp->elements_count; i++)
		{
			swamp_file *file = list_get(info.archivosSwamp, i);
			if (file->pagesAvailable >= amountOfPages)
				return true;
		}
		return false;
	}
}

int number_of_pages_from_process(uint32_t pid, swamp_file *file)
{
	int result = 0;
	for (int i = 0; i < info.paginasPorArchivo; i++)
	{
		if (file->pages[i].pid == pid)
			result++;
	}
	return result;
}

swamp_file *find_available_file(t_list *files)
{
	swamp_file *fileWithMostSpace = NULL;
	for (int i = 0; i < files->elements_count; i++)
	{
		swamp_file *file = (swamp_file *)list_get(files, i);
		if (fileWithMostSpace == NULL || file->pagesAvailable > fileWithMostSpace->pagesAvailable)
			fileWithMostSpace = file;
	}

	return fileWithMostSpace;
}

MEM_SWAP_MESSAGE write_first_available_frame(swamp_file *file, void *data, int size,
											 int pageSize, int pagesPerFile, int *output_frame)
{
	int firstFreeFrame = find_free_frame(file->pages, pagesPerFile);
	if (firstFreeFrame == -1)
	{
		log_error(logger, "No se encontro un frame libre en el swamp");
		return ERROR_NO_SPACE_IN_SWAMP;
	}

	write_file(file->path, firstFreeFrame, data, size, pageSize);

	*output_frame = firstFreeFrame;
	return OK;
}

int find_free_frame(swamp_map *pages, int pagesPerFile)
{
	for (int i = 0; i < pagesPerFile; i++)
	{
		if (pages[i].pageNumber == -1)
			return i;
	}

	return -1;
}

MEM_SWAP_MESSAGE write_file(char *path, int frameNumber, void *data, int size, int pageSize)
{
	FILE *file = fopen(path, "rb+");
	if (fseek(file, frameNumber * pageSize, SEEK_SET) == -1)
	{
		log_error(logger, "Error al moverse por el archivo de swap.");
		return ERROR_COULD_NOT_USE_FILE;
	}
	if (fwrite(data, size, 1, file) == -1)
	{
		log_error(logger, "Error al escribir archivo de swap.");
		return ERROR_COULD_NOT_USE_FILE;
	}
	fclose(file);
	return OK;
}

MEM_SWAP_MESSAGE get_page(uint32_t pid, int pageNumber, void **output_buffer)
{
	swamp_file *file = find_file_by_process(info.archivosSwamp, pid);
	if (file == NULL)
	{
		log_error(logger, "No se encontro archivo con el proceso.");
		return ERROR_PAGE_NOT_FOUND;
	}
	int frame = find_frame_by_page(file, pid, pageNumber, info.paginasPorArchivo);
	if (frame == -1)
	{
		log_error(logger, "No se encontro la pagina %d del proceso %d en swap.", pageNumber, pid);
		return ERROR_PAGE_NOT_FOUND;
	}
	MEM_SWAP_MESSAGE result = read_frame(file, frame, info.tamanioPagina, output_buffer);
	if (result != OK)
		return result;
	return PAGE_READ;
}

int find_frame_by_page(swamp_file *file, uint32_t pid, int pageNumber, int pagesPerFile)
{
	for (int i = 0; i < pagesPerFile; i++)
	{
		bool correctPageNumber = file->pages[i].pageNumber == pageNumber;
		bool correctProcess = file->pages[i].pid == pid;
		if (correctPageNumber && correctProcess)
			return i;
	}

	return -1;
}

MEM_SWAP_MESSAGE read_frame(swamp_file *swampFile, int frameNumber, int pageSize, void **output_buffer)
{
	FILE *file = fopen(swampFile->path, "rb+");
	int offset = frameNumber * pageSize;
	if (fseek(file, offset, SEEK_SET) == -1)
	{
		log_error(logger, "Error al moverse por el archivo de swap.");
		fclose(file);
		return ERROR_COULD_NOT_USE_FILE;
	}

	*output_buffer = malloc(pageSize);
	if (fread(*output_buffer, pageSize, 1, file) == -1)
	{
		log_error(logger, "Error al leer el archivo de swap.");
		fclose(file);
		free(*output_buffer);
		return ERROR_COULD_NOT_USE_FILE;
	}

	fclose(file);
	return OK;
}

MEM_SWAP_MESSAGE release_process(uint32_t pid)
{
	swamp_file *swampFile = find_file_by_process(info.archivosSwamp, pid);
	if (swampFile == NULL)
	{
		log_error(logger, "No se encontro archivo con el proceso en swap");
		return ERROR_PROCESS_NOT_FOUND;
	}

	for (int i = 0; i < info.paginasPorArchivo; i++)
	{
		if (swampFile->pages[i].pid == pid)
		{
			MEM_SWAP_MESSAGE result = clear_frame_from_file(swampFile, i, info.tamanioPagina);
			if (result == OK)
			{
				swampFile->pages[i].pid = -1;
				swampFile->pages[i].pageNumber = -1;
				swampFile->pagesAvailable++;
			}
			else
				return result;
		}
	}

	return PROCESS_RELEASED;
}

MEM_SWAP_MESSAGE clear_frame_from_file(swamp_file *swampFile, int frameNumber, int pageSize)
{
	FILE *file = fopen(swampFile->path, "rb+");
	if (file == NULL)
		return ERROR_COULD_NOT_OPEN_FILE;

	int offset = frameNumber * pageSize;
	if (fseek(file, offset, SEEK_SET) == -1)
	{
		fclose(file);
		log_error(logger, "Error al moverse por el archivo de swap.");
		return ERROR_COULD_NOT_USE_FILE;
	}

	char emptyBuffer[pageSize];
	for (int i = 0; i < pageSize; i++)
		emptyBuffer[i] = '\0';

	if (fwrite(emptyBuffer, pageSize, 1, file) == -1)
	{
		fclose(file);
		log_error(logger, "Error al escribir archivo de swap.");
		return ERROR_COULD_NOT_USE_FILE;
	}

	fclose(file);
	return OK;
}

MEM_SWAP_MESSAGE delete_page(uint32_t pid, int pageNumber)
{
	swamp_file *swampFile = find_file_by_process(info.archivosSwamp, pid);

	if (swampFile == NULL)
		return ERROR_PROCESS_NOT_FOUND;

	int frame = find_frame_by_page(swampFile, pid, pageNumber, info.paginasPorArchivo);
	if (frame == -1)
		return ERROR_PAGE_NOT_FOUND;

	MEM_SWAP_MESSAGE result = clear_frame_from_file(swampFile, frame, info.tamanioPagina);
	if (result == OK)
	{
		update_file_processes_if_needed(swampFile, pid);
		result = PAGE_DELETED;
		log_info(logger, "Se borro la pagina %d del proceso %d", pageNumber, pid);
		swampFile->pages[frame].pageNumber = -1;
		swampFile->pages[frame].pid = -1;
		swampFile->pagesAvailable++;


	}
	return result;
}

void update_file_processes_if_needed(swamp_file *file, uint32_t pid)
{
	if(should_remove_process_from_file(file, pid) == true)
	{
		for (int i = 0; i < file->processes->elements_count; i++)
		{
			int *auxPid = list_get(file->processes, i);
			if(*auxPid == pid) list_remove_and_destroy_element(file->processes, i, free);
		}
	}
}

bool should_remove_process_from_file(swamp_file *file, uint32_t pid)
{
	for (int j = 0; j < info.paginasPorArchivo; j++)
	{
		if(file->pages[j].pid == pid) return false;
	}

	return true;
}
