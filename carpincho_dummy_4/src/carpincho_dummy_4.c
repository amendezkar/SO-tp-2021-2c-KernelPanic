#include "../include/carpincho_dummy_4.h"
#include <stdio.h>
#include <stdlib.h>

void* carpincho1_func(void *config);
void* carpincho2_func(void *config);
void* carpincho3_func(void *config);
void* carpincho4_func(void *config);
void* carpincho5_func(void *config);
void* carpincho6_func(void *config);
void* carpincho7_func(void *config);
void* carpincho8_func(void *config);

int main(){

	char* argv[2];
	argv[1] = "cfg/carpincho_dummy_4.config";

	pthread_t carpincho1;
	pthread_t carpincho2;
	pthread_t carpincho3;
	pthread_t carpincho4;
	pthread_t carpincho5;
	pthread_t carpincho6;
	pthread_t carpincho7;
	pthread_t carpincho8;

	printf("MAIN - Utilizando el archivo de config: %s\n", argv[1]);

	pthread_create(&carpincho1, NULL, carpincho1_func, argv[1]);
	//sleep(1);
	pthread_create(&carpincho2, NULL, carpincho2_func, argv[1]);
	//sleep(1);
	pthread_create(&carpincho3, NULL, carpincho3_func, argv[1]);
	//sleep(1);
	pthread_create(&carpincho4, NULL, carpincho4_func, argv[1]);
	//sleep(1);
	pthread_create(&carpincho5, NULL, carpincho5_func, argv[1]);
	//sleep(1);
	pthread_create(&carpincho6, NULL, carpincho6_func, argv[1]);
	//sleep(1);
	pthread_create(&carpincho7, NULL, carpincho7_func, argv[1]);
	//sleep(1);
	pthread_create(&carpincho8, NULL, carpincho8_func, argv[1]);

	pthread_join(carpincho8, NULL);
	pthread_join(carpincho7, NULL);
	pthread_join(carpincho6, NULL);
	pthread_join(carpincho5, NULL);
	pthread_join(carpincho4, NULL);
	pthread_join(carpincho3, NULL);
	pthread_join(carpincho2, NULL);
	pthread_join(carpincho1, NULL);
	printf("MAIN - Retirados los carpinchos de la pelea, hora de analizar los hechos\n");

	return EXIT_SUCCESS;
}

void* carpincho1_func(void *config) {

	mate_instance instance;

	printf("C1 - Llamo a mate_init\n");
	mate_init(&instance, (char*) config);

	printf("C1 - Llamo a una operación de IO\n");
	mate_call_io(&instance, "PILETA", "Vamos a usar la pileta...");

	sleep(1);

	printf("C1 - Reservo un alloc de 200 bytes\n");
	mate_pointer alloc0 = mate_memalloc(&instance, 200);

	sleep(1);

	printf("C1 - Llamo a una operación de IO\n");
	mate_call_io(&instance, "PILETA", "Vamos a usar la pileta...");

	printf("C1 - Escribo en la página 0\n");
	mate_memwrite(&instance, "Que lindo dia para estar en la pileta", alloc0, 38);

	sleep(1);

	printf("C1 - Llamo a una operación de IO\n");
	mate_call_io(&instance, "PILETA", "Vamos a usar la pileta...");

	sleep(1);

	printf("C1 - Leo de la página 0\n");
	char *mensaje = malloc(38);
	mate_memread(&instance, alloc0, mensaje, 38);
	printf("C1 - Mensaje leido de la memoria: %s", mensaje);

	printf("C1 - Se retira a descansar\n");
	mate_close(&instance);

	return 0;
}

void* carpincho2_func(void *config) {

	mate_instance instance;

	printf("C2 - Llamo a mate_init\n");
	mate_init(&instance, (char*) config);

	printf("C2 - Abro una botella de smirnoff\n");
	mate_sem_init(&instance, "smirnoffc2", 1);
	mate_sem_init(&instance, "smirnoffc3", 0);
	mate_sem_post(&instance, "smirnoffc3");
	mate_sem_wait(&instance, "smirnoffc2");

	printf("C2 - Fondo blanco de smirnoff\n");
	mate_sem_wait(&instance, "smirnoffc2");

	printf("C2 - Me desperte re crudo, vamos a armar bardo!! \n");
	int a = 1;
	mate_pointer puntero = 0;
	while (a <= 20) {
		puntero = mate_memalloc(&instance, 100);
		printf("C2 valor malloc: %d \n", puntero);
		usleep(5000);
		mate_memfree(&instance, puntero);
		printf("C2 - %d Jardines destruidos!!\n", a);
		a++;
	}

	printf("C2 - Ya agotado, se retira a descansar\n");
	mate_close(&instance);

	return 0;
}

void* carpincho3_func(void *config) {

	mate_instance instance;

	printf("C3 - Llamo a mate_init\n");
	mate_init(&instance, (char*) config);

	sleep(5);

	printf("C3 - Vamos a tomar un smirnoff\n");
	mate_sem_init(&instance, "smirnoffc2", 1);
	mate_sem_post(&instance, "smirnoffc2");

	printf("C3 - Vamos a protestar por nuestro lugar\n");
	int a = 1;
	mate_pointer puntero = 0;
	while (a <= 20) {
		puntero = mate_memalloc(&instance, 100);
		printf("C3 valor malloc: %d \n", puntero);
		mate_call_io(&instance, "HUMEDAL", "No nos vamos nada, que nos saquen a patadas");
		mate_memfree(&instance, puntero);
		a++;
	}

	printf("C3 - Se retira a descansar\n");
	mate_close(&instance);

	return 0;
}

void* carpincho4_func(void *config) {

	mate_instance instance;

	printf("C4 - Llamo a mate_init\n");
	mate_init(&instance, (char*) config);

	printf("C4 - Reservo memoria\n");
	mate_pointer puntero1 = mate_memalloc(&instance, 200);
	sleep(1);
	mate_pointer puntero2 = mate_memalloc(&instance, 200);
	sleep(1);
	mate_pointer puntero3 = mate_memalloc(&instance, 200);
	sleep(1);
	mate_pointer puntero4 = mate_memalloc(&instance, 200);
	sleep(1);
	mate_pointer puntero5 = mate_memalloc(&instance, 200);
	sleep(1);
	mate_pointer puntero6 = mate_memalloc(&instance, 200);
	sleep(1);
	mate_pointer puntero7 = mate_memalloc(&instance, 200);
	sleep(1);
	mate_pointer puntero8 = mate_memalloc(&instance, 200);

	printf("C4 - Un par de vueltas en la pileta... \n");
	int a = 1;
	mate_pointer puntero = 0;
	while (a <= 20) {
		puntero = mate_memalloc(&instance, 100);
		mate_call_io(&instance, "PILETA", "Vamos a usar la pileta...");
		mate_memfree(&instance, puntero);
		a++;
	}

	printf("C4 - Se retira a descansar\n");
	mate_close(&instance);

	return 0;
}

void* carpincho5_func(void *config) {

	mate_instance instance;

	printf("C5 - Llamo a mate_init\n");
	mate_init(&instance, (char*) config);

	printf("C5 - Vamos a hacer algo de quilombo con los semáforos\n");
	mate_sem_init(&instance, "SEMAFORO", 1);

	printf("C5 - Se viene el post\n");
	mate_sem_post(&instance, "SEMAFORO");
	mate_sem_post(&instance, "SEMAFORO");
	mate_sem_post(&instance, "SEMAFORO");
	mate_sem_post(&instance, "SEMAFORO");
	mate_sem_post(&instance, "SEMAFORO");
	mate_sem_post(&instance, "SEMAFORO");
	mate_sem_post(&instance, "SEMAFORO");
	mate_sem_post(&instance, "SEMAFORO");

	printf("C5 - Se viene el wait\n");
	mate_sem_wait(&instance, "SEMAFORO");
	mate_sem_wait(&instance, "SEMAFORO");
	mate_sem_wait(&instance, "SEMAFORO");
	mate_sem_wait(&instance, "SEMAFORO");
	mate_sem_wait(&instance, "SEMAFORO");
	mate_sem_wait(&instance, "SEMAFORO");
	mate_sem_wait(&instance, "SEMAFORO");
	mate_sem_wait(&instance, "SEMAFORO");

	printf("C5 - Se retira a descansar\n");
	mate_close(&instance);

	return 0;
}

void* carpincho6_func(void *config) {

	mate_instance instance;

	printf("C6 - Llamo a mate_init\n");
	mate_init(&instance, (char*) config);

	printf("C6 - Reservo un alloc de 4 bytes\n");
	mate_pointer alloc = mate_memalloc(&instance, 4);

	printf("C6 - Un par de vueltas con la memoria... \n");
	int a = 1;
	int b = 0;
	while (a <= 256) {
		usleep(5000);
		mate_memwrite(&instance, &a, alloc, 4);
		mate_memread(&instance, alloc, &b, 4);
		printf("C6 - Ya destrui %d macetas... \n", b);
		a++;
	}

	printf("C6 - Se retira a descansar\n");
	mate_close(&instance);

	return 0;
}

void* carpincho7_func(void *config) {

	mate_instance instance;

	printf("C7 - Llamo a mate_init\n");
	mate_init(&instance, (char*) config);

	printf("C7 - Creo un nuevo semáforo\n");
	mate_sem_init(&instance, "SEMAFORO_NUEVO", 1);

	printf("C7 - Un par de vueltas con semáforos... \n");
	int a = 1;
	while (a <= 256) {
		mate_sem_post(&instance, "SEMAFORO_NUEVO");
		usleep(5000);
		mate_sem_wait(&instance, "SEMAFORO_NUEVO");
		printf("C7 - Vuelta número %d... \n", a);
		a++;
	}

	printf("C7 - Se retira a descansar\n");
	mate_close(&instance);

	return 0;
}

void* carpincho8_func(void *config) {

	mate_instance instance;

	printf("C8 - Llamo a mate_init\n");
	mate_init(&instance, (char*) config);

	printf("C8 - Soy el carpincho vago, no pienso hacer nada\n");
	sleep(60);

	printf("C8 - Se retira a descansar de nuevo...\n");
	mate_close(&instance);

	return 0;
}
