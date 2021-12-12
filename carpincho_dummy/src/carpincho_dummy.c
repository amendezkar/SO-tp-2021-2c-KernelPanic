#include "../include/carpincho_dummy.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <commons/log.h>
#include <semaphore.h>
#include <string.h>

char *LOG_PATH = "./fibonacci.log";
char *PROGRAM_NAME = "fibonacci";
t_log *logger;

int main(int argc, char *argv[])
{
  uint32_t n_1 = 1;
  uint32_t n_2 = 0;
  uint32_t n = 0;
  char *label_n_1 = strdup("n-1");
  char *label_n_2 = strdup("n-2");

  logger = log_create(LOG_PATH, PROGRAM_NAME, true, 2);

  char *config_path = "/home/utnso/workspace/tp-2021-2c-KernelPanic/carpincho_dummy/cfg/carpincho_dummy.config";


  //Instancio la lib
  mate_instance mate_ref;
  mate_init(&mate_ref, config_path);

  //Pido memoria y asigno valores estáticos e iniciales de la sucesión de fibonacci
  mate_pointer key_2 = mate_memalloc(&mate_ref, 4);
  log_info(logger, "key_2: %d", key_2);
  mate_memwrite(&mate_ref, label_n_2, key_2, 4);

  mate_pointer value_2 = mate_memalloc(&mate_ref, sizeof(uint32_t));
  mate_memwrite(&mate_ref, &n_2, value_2, sizeof(uint32_t));
  log_info(logger, "value_2: %d", value_2);

  mate_pointer key_1 = mate_memalloc(&mate_ref, 4);
  mate_memwrite(&mate_ref, label_n_1, key_1, 4);
  log_info(logger, "key_1: %d", key_1);

  mate_pointer value_1 = mate_memalloc(&mate_ref, sizeof(uint32_t));
  mate_memwrite(&mate_ref, &n_1, value_1, sizeof(uint32_t));
  log_info(logger, "value_1: %d", value_1);

  while(1) {
    mate_memread(&mate_ref, value_1, &n_1, sizeof(uint32_t));
    mate_memread(&mate_ref, value_2, &n_2, sizeof(uint32_t));
    mate_memread(&mate_ref, key_2, label_n_2, 4);
    mate_memread(&mate_ref, key_1, label_n_1, 4);

    n = n_1 + n_2;

    log_info(logger, "valores de la sucesión:");
    log_info(logger, "%s: %d", label_n_2, n_2);
    log_info(logger, "%s: %d", label_n_1, n_1);
    log_info(logger, "n: %d", n);

    n_2 = n_1;
    n_1 = n;

    mate_memwrite(&mate_ref, &n_1, value_1, sizeof(uint32_t));
    mate_memwrite(&mate_ref, &n_2, value_2, sizeof(uint32_t));
  }

  return 0;
}
