#include <iostream>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstring>
using namespace std;

int main() {
    const char *fifo = "/tmp/myfifo";// misma direccion
    mkfifo(fifo);// con esto se crea la tuberia o pipe con nombre (fifo)

    int fd = open(fifo, O_WRONLY); //se le da nombre y que hace, (escribir)
    const char *msg = "prueba servido";// se da el mensaje
    write(fd, msg, strlen(msg)+1);// se envia el mensaje
  // estructura: proceso, mensaje, largo
    close(fd);// se cierra este proceso
    return 0;
}
