#ifndef SWAMP_COMMONS_H_
#define SWAMP_COMMONS_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commons/log.h>
#include <commons/config.h>
#include <commons/collections/list.h>
#include "utils/log_config.h"
#include "utils/sockets.h"
#include "utils/mensajes_mem_swamp.h"

struct swamp_info
{
	char ip[30];
	char puerto[10];
	int tamanioSwap;
	int tamanioPagina;
	int paginasPorArchivo;
	int marcosMaximos;
	int retardoSwap;
	t_list *archivosSwamp;
} typedef swamp_info;

struct swamp_map
{
	uint32_t pid;
	int pageNumber;
} typedef swamp_map;

struct swamp_file
{
	char *path;
	int pagesAvailable;
	swamp_map *pages;
	t_list *processes;
} typedef swamp_file;

extern swamp_info info;
extern t_log *logger;

int number_of_pages_from_process(uint32_t pid, swamp_file *processFile);
void set_fixed_asign();
bool is_there_space_available(uint32_t pid, int amountOfPages);
MEM_SWAP_MESSAGE add_page(uint32_t pid, int pageNumber, void *data, int size);
MEM_SWAP_MESSAGE get_page(uint32_t pid, int pageNumber, void **output_buffer);
swamp_file *find_file_by_process(t_list *files, uint32_t pid);
swamp_file *find_available_file(t_list *files);
int find_free_frame(swamp_map *pages, int pagesPerFile);
int find_frame_by_page(swamp_file *file, uint32_t pid, int pageNumber, int pagesPerFile);
MEM_SWAP_MESSAGE write_first_available_frame(swamp_file *file, void *data, int size, int pageSize, int pagesPerFile, int *output_frame);
MEM_SWAP_MESSAGE write_file(char *path, int frameNumber, void *data, int size, int pageSize);
MEM_SWAP_MESSAGE read_frame(swamp_file *swampFile, int frameNumber, int pageSize, void **output_buffer);
MEM_SWAP_MESSAGE release_process(uint32_t pid);
MEM_SWAP_MESSAGE clear_frame_from_file(swamp_file *swampFile, int frameNumber, int pageSize);
MEM_SWAP_MESSAGE delete_page(uint32_t pid, int pageNumber);
void update_file_processes_if_needed(swamp_file *file, uint32_t pid);
bool should_remove_process_from_file(swamp_file *file, uint32_t pid);


#endif
