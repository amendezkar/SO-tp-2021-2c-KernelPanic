/*
 * main.h
 *
 *  Created on: 8 sep. 2021
 *      Author: utnso
 */

#ifndef SWAMP_INCLUDE_MAIN_H_
#define SWAMP_INCLUDE_MAIN_H_

#include "swamp.h"
#include "utils/comunicacion.h"

swamp_info get_info(void);
void recieve_and_set_mode(void);
void start_server_and_listen();
void init_files(int swapSize, t_list *files);
void process_add_page(int socketClient);
void process_get_page_size(int socketClient);
void process_add_pages(int socketClient);
void process_read_page(int socketClient);
void process_release_process(int socketClient);
void process_is_space_sufficient(int socketClient);
void process_delete_page(int socketClient);

#endif /* SWAMP_INCLUDE_MAIN_H_ */
