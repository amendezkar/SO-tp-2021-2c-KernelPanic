#include "main.h"

void sighandler(int x) {
    switch (x) {
        case SIGINT:
        	imprimir_estado_frames();
        	imprimir_metricas();
            terminar_programa(socketSwamp, logger, config);
            free(memoriaPrincipal);
            exit(EXIT_SUCCESS);
        case SIGUSR1:
        	dump_tlb(cfg->pathDump,listaTLB);
        	break;
        case SIGUSR2:
        	limpiar_TLB();
        	break;
    }
}


int main(int argc, char* argv[]) {

	signal(SIGINT , sighandler);
	signal(SIGUSR1 , sighandler);
	signal(SIGUSR2 , sighandler);

    logger = iniciar_logger("bin/memoria.log","memoria",true,LOG_LEVEL_INFO);

    if(!init_config() || !generar_conexiones() || !init_memoria()){

        terminar_programa(socketSwamp, logger, config);
        return EXIT_FAILURE;
    }

   while (server_escuchar("memoria", socketMemoria));

   terminar_programa(socketSwamp, logger, config);
   finalizar_memoria();

   return EXIT_SUCCESS;
}
