#include <iostream>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
using namespace std;

int main() {
    const char *fifo = "/tmp/myfifo";// se tiene que dar una direccion
    char buffer[100];
    int fd = open(fifo, O_RDONLY); //creacion de tuberia, recibe nombre y que hace
    read(fd, buffer, sizeof(buffer));// se les da que cosas lee
    cout << " mensaje " << buffer << endl;
    close(fd);// se cierra este proceso
    return 0;
}
