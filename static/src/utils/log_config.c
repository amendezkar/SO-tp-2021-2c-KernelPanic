/*
 * log_config.c
 *
 *  Created on: 6 sep. 2021
 *      Author: utnso
 */
#include "utils/log_config.h"

t_log* iniciar_logger(char* archivoLog, char* nombrePrograma, int flagConsola, t_log_level nivelLoggeo) {

	t_log* logger = log_create(archivoLog, nombrePrograma, flagConsola, nivelLoggeo);
	//hay q hacer un chequeo de q el programa q se paso es correcto?
	if(logger == NULL){
		//printf("No se pudo iniciar el logger del archivo %s perteneciente al programa %s \n"
		//		, archivoLog, nombrePrograma);
		exit(1);
	}
	else
		return logger;
}

t_config* iniciar_config(char* path) {
	t_config* nuevo_config;

	if((nuevo_config = config_create(path)) == NULL){
		perror("No se pudo crear la config\n");
		abort();
	}

	return nuevo_config;
}

char* obtener_de_config(t_config* config, char* key) {

	char* valor;

	if((valor = config_get_string_value(config, key)) == NULL){

		perror("No se pudo obtener el valor del archivo de configuraciones!\n");
		abort();
	}

	return valor;
}

int obtener_int_de_config(t_config* config, char* key){

	int valor;

	if((valor = config_get_int_value(config, key)) < 0){

		perror("No se pudo obtener el valor del archivo de configuraciones!\n");
		abort();
	}

	return valor;
}

float obtener_float_de_config(t_config* config, char* key){

	double valor;

	if((valor = config_get_double_value(config, key)) < 0){

		perror("No se pudo obtener el valor del archivo de configuraciones!\n");
		abort();
	}

	return (float) valor;
}

bool config_tiene_todas_las_propiedades(t_config* cfg, char** propiedades) {
    for(uint8_t i = 0; propiedades[i] != NULL; i++) {
        if(!config_has_property(cfg, propiedades[i]))
            return false;
    }

    return true;
}

void terminar_programa(int conexion, t_log* logger, t_config* config) {
	log_destroy(logger);
	config_destroy(config);
	liberar_conexion(conexion);
}
