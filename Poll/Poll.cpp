#include <iostream>
#include <unistd.h>
#include <poll.h>
using namespace std;

int main() {
    struct pollfd fds[1]; //se crea un proceso
    fds[0].fd = STDIN_FILENO; // esto es para reconocer teclado
    fds[0].events = POLLIN;   // esto es para escribir
  //es como un cin

    cout << "aqui hay tiempo " << endl;

    int ret = poll(fds, 1, 10000); // espera hasta 10 segundos se cuenta en milisegundos
    if (ret == -1) {
        perror("poll");// si hay error se muestra 
      //pasa cuando no se crea bien
        return 1;
    } else if (ret == 0) {
        cout << "esta vacio el proceso";
    } else {
        if (fds[0].revents & POLLIN) {// ve si hay procesos en ejecucion
            char buf[100];
            read(STDIN_FILENO, buf, sizeof(buf)); // lee el mensaje y lo ancla al proceso
            cout << "mensaje" << buf << endl;
        }
    }
    return 0;
}
