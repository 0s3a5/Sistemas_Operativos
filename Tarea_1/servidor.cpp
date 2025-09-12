#include <iostream>
#include <fstream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <csignal>
#include <cstring>

using namespace std;

#define SERVIDOR_FIFO "servidor_fifo"
#define MAX_CLIENTS 50

struct Reporte {
    pid_t pid;
    int count;
};

Reporte reportes[MAX_CLIENTS];
int totalClientes = 0;

int buscarReporte(pid_t pid) {
    for (int i = 0; i < totalClientes; i++) {
        if (reportes[i].pid == pid) return i; // se busca si el proceso tiene algun reporte 
    }
    return -1;//si no tiene se devuelve el -1
}

void agregarReporte(pid_t pid) {
    int idx = buscarReporte(pid);
    if (idx == -1 && totalClientes < MAX_CLIENTS) {
        //si no tiene ningun reporte se crea uno
        reportes[totalClientes].pid = pid; //se toma la posicion como pid
        reportes[totalClientes].count = 0;// su contador de reportes empiez en 0
        idx = totalClientes;
        totalClientes++;
    }
    reportes[idx].count++;//como ya se reporto se le suma el reporte al proceso

    cout << " proceso " << pid << " tiene " << reportes[idx].count << " reportes"<<endl;

    if (reportes[idx].count >= 10) {
        cout << "proceso " << pid << " tiene muchos reportes ban"<<endl;
        kill(pid, SIGTERM);
    }
}

int main() {
    unlink(SERVIDOR_FIFO);//se cierra todo proceso que estuviera usando el servidor
    mkfifo(SERVIDOR_FIFO, 0666);

    int fd = open(SERVIDOR_FIFO, O_RDONLY);//se deja proceso como solo de lectura
    if (fd == -1) {
        perror("no se abruo el server");
        return 1;
    }

    char buffer[256];
    cout << "prendiendo"<<endl;

    while (true) {
    //verificamos que el area a ocupar este vacio
    //en memoria, antes no daba bien las cosas
    //le damos un lugar en memoria, de que va a estar llenos en 
    //este caso de 0, y el tamaño del espacio a ocupar
        memset(buffer, 0, sizeof(buffer));
        int n = read(fd, buffer, sizeof(buffer));
        if (n > 0) { buffer[n] = '\0';
        pid_t pid;
            char msg[200];
            //se ocupa el escaner para la linea donde se da
            //direccion de memoria, el pid y el mensaje
            sscanf(buffer, "%d: %[^\n]", &pid, msg);//se lee el pid y el mensaje

            cout << "pid " << pid << " mensaje " << msg << endl;
            //se compara el mensaje para ver que sea un reporte
            if (strncmp(msg, "reportar", 8) == 0) {//si es un reporte
                pid_t pidReportado;
                sscanf(msg, "reportar %d", &pidReportado);//se lee el pid
                agregarReporte(pidReportado);//se envia el pid a la funcion reporte
                //con esto se agrega un reporte al contador
            }
        }
    }

    close(fd);
    unlink(SERVIDOR_FIFO);//se vueve a cerrar el cervidor pero ahora con todos los cambios ya hechos
    return 0;
}
