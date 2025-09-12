#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cstring>
#include <cstdlib>

using namespace std;

#define SERVIDOR_FIFO "servidor_fifo"

int main() {
    int fd = open(SERVIDOR_FIFO, O_WRONLY); // solo se escribre en esta pipe
    if (fd == -1) {
        perror("No se pudo conectar al servidor");
    }
    pid_t pid = getpid();
    cout << "cliente " << pid << " conectado, puedes escribir mensajes."<<endl;
    cout<<"si escribes ahora se va a enviar como mensaje"<<endl;
    cout << "si esribes reportar y al lado un numero de pid se reporta ese pid "<<endl;
    cout << "Comandos: duplicar | reportar <PID> | salir"<<endl;

    string input;
    char buffer[256];
    while (true) {
        getline(cin, input);

        if (input == "salir") {
            cout << "cliente " << pid << "] desconectando"<<endl;
            break;
        } else if (input == "duplicar") {
            pid_t child = fork();
            if (child == 0) {
                execl("./cliente2", "./cliente2", NULL);
                perror("Error al duplicar");
                exit(1);
            }
            continue;
        }
        //funcionamiento de snprintf necesita
        
        // archivo de guardado, tamaño del archivo, impresion del pid, impresion del mensaje, pid y mensaje
        snprintf(buffer, sizeof(buffer), "%d: %s", pid, input.c_str());
        //funcionamiento del write, se necesita
        //el proceso para la salida, el mensaje y el tamaño del mensaje
        write(fd, buffer, strlen(buffer));
    }
    close(fd);
    return 0;
}
