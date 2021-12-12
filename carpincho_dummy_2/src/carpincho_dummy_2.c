#include "../include/carpincho_dummy_2.h"
#define SEMAFORO_SALUDO "SEM_HELLO"

int main(int argc, char *argv[]) {

	char *config = "/home/utnso/workspace/tp-2021-2c-KernelPanic/carpincho_dummy_2/cfg/carpincho_dummy_2.config";

	printf("MAIN - Utilizando el archivo de config: %s\n", config);

	mate_instance instance;

	mate_init(&instance, (char*)config);

    char saludo[] = "¡Hola mundo!\n";

    mate_pointer saludoRef = mate_memalloc(&instance, strlen(saludo));

    mate_memwrite(&instance, saludo, saludoRef, strlen(saludo));

    mate_memread(&instance, saludoRef, saludo, strlen(saludo));

    printf(saludo);

    mate_sem_post(&instance, SEMAFORO_SALUDO);

    mate_close(&instance);

	return EXIT_SUCCESS;
}
