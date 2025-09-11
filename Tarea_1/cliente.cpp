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
    char buffer[256];
    int opcion;
    while (true) {
        cout << "selecciona opcion (solo un numero)" << endl;
        cout << "1 escribir mensaje" << endl;
        cout << "2 duplicar cliente" << endl;
        cout << "3 reportar proceso" << endl;
        cout << "4 salir";
        cin >> opcion;

        if (opcion == 1) {
            string palabra;
            cout << "mensaje (por favor solo una palabra) ";
            cin >> palabra;

            snprintf(buffer, sizeof(buffer), "%d  %s", pid, palabra.c_str()); // muestra el pid y mensaje por pantalla
            write(fd, buffer, strlen(buffer));//mandarlo al servidor, 

        } else if (opcion == 2) {
            pid_t hijo = fork();
            if (hijo == 0) {
                execl("./cliente", "./cliente", NULL);
                perror("algo salio mal :(");
                exit(1);
            }

        } else if (opcion == 3) {
            pid_t numero;
            cout << "dar numero del pin "<< endl;
            cout << "de preferenia que se vea en el servidor " << endl;
            cin >> numero;

            snprintf(buffer, sizeof(buffer), "%d reporte %d", pid, numero);//muetsra por pantalla datos del pid y mensaje
            write(fd, buffer, strlen(buffer));

        } else if (opcion == 4) {
            cout << "desconectado ";
            break;

        } else {
            cout << "da un nuemero valido "<< endl;
            cout << "dice del 1 al 4 genio "<<endl;
        }
    }

    close(fd);
    return 0;
}

