#include <iostream>
#include <unistd.h>
#include <cstring>

using namespace std;

int main() {
    int fds[2]; // se crea la pipe 
  // 2 espacios del arreglo porque son los dos procesos a comunicar
    if (pipe(fds) == -1) {//si no se crea se muestra el eror
        perror("pipe");
        return 1;
    }

    pid_t pid = fork();// se crea fork
  // con esto se tiene 2 procesos 
  //padre/hijo o cliente/servidor

    if (pid == 0) {
        // Hijo/Cliente
        close(fds[1]); // se cierra el porceso contrario para reescribir el proseso a usar
        char buffer[100];// texto para dar mensaje
        read(fds[0], buffer, sizeof(buffer));// se lee el proseso
        cout << "cliente tiene " << buffer << endl;
        close(fds[0]);// se cierra el proceso hijo o cliente
    } else {
        // Padre/Servidor
        close(fds[0]); // se cierra el proceso contrario
        const char *msg = "prueba servidor/padre";// se da mensaje para enviar
        write(fds[1], msg, strlen(msg)+1);// reescribe el proceso a usar
      //strlen es lo mimso que sizeof o size, unicamente para entregar el largo del mensaje
        close(fds[1]);// se cierra el proceso usado
    }
}
