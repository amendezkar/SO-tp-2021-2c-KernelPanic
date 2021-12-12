#include "proceso.h"

int main() {

    char* config_path = "cfg/proceso.config";
    mate_instance* lib = malloc(sizeof(mate_instance));
    lib->group_info = malloc(sizeof(mate_posta));
    int status = mate_init(lib, config_path);
    if(status != 0) {
        perror("No se pudieron establecer las conexiones");
        abort();
    }

    mate_call_io(lib, "falopa", "Hola puto");
    mate_call_io(lib, "falopa", "Hola puto");
    mate_call_io(lib, "falopa", "Hola puto");
    mate_call_io(lib, "falopa", "Hola puto");

    mate_close(lib);

    return 0;
}
