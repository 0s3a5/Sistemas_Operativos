#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cstring>
#include <cstdlib>

using namespace std;

#define SERVER_FIFO "server_fifo"

int main() {
    int fd = open(SERVER_FIFO, O_WRONLY);
    if (fd == -1) {
        perror("No se pudo conectar al servidor");
        return 1;
    }

    pid_t pid = getpid();
    cout << "[Cliente " << pid << "] Conectado. Puedes escribir mensajes.\n";
    cout << "Comandos: duplicar | reportar <PID> | salir\n";

    string input;
    char buffer[256];

    while (true) {
        getline(cin, input);

        if (input == "salir") {
            cout << "[Cliente " << pid << "] Desconectando...\n";
            break;
        } else if (input == "duplicar") {
            pid_t child = fork();
            if (child == 0) {
                execl("./client", "./client", NULL);
                perror("Error al duplicar");
                exit(1);
            }
            continue;
        }

        snprintf(buffer, sizeof(buffer), "%d: %s", pid, input.c_str());
        write(fd, buffer, strlen(buffer));
    }

    close(fd);
    return 0;
}
