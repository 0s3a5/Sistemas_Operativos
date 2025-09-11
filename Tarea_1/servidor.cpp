#include <iostream>
#include <fstream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <csignal>
#include <cstring>

using namespace std;
#define MAX_CLIENTS 50

struct Reporte {
    pid_t pid;
    int count;
};

Reporte reportes[MAX_CLIENTS];
int totalClientes = 0;

int buscarReporte(pid_t pid) {
    for (int i = 0; i < totalClientes; i++) {
        if (reportes[i].pid == pid) return i;//busacamos cuantos reportes tiene el pid
    }
    return -1; //si no tiene se devuelve -1
}

void agregarReporte(pid_t pid) {
    int idx = buscarReporte(pid);//si no tiene reportes se da el -1
    if (idx == -1 && totalClientes < MAX_CLIENTS) {//al no estar creado se ve si es posible crear el cliente
        reportes[totalClientes].pid = pid; //se le da el pid al nuevo proceso
        reportes[totalClientes].count = 0; //su cuenta de reportes inicia en 0
        idx = totalClientes;
        totalClientes++;// como se reporto por primera vez se le suma un reporte
    }
    reportes[idx].count++;// se suma automaticamente si esta creado

    cout << "proceso " << pid << " tiene " << reportes[idx].count << " reportes"<< endl;

    if (reportes[idx].count >= 10) {
        cout << "proceso " << pid << " tiene limite de reportes "<< endl;
        kill(pid, SIGTERM);
    }
}

int main() {
    unlink(servidor_fifo);
    mkfifo(servidor_fifo);
    int fd = open(servidor_fifo, O_RDONLY); // se abre proceso de solo lectura
    if (fd == -1) {
        perror("Error abriendo server_fifo"); // si no se creo se tira error
        return 1;
    }

    char buffer[256];
    cout << "abierto"<< endl ;

    while (true) {
            pid_t pid;
            char msg[200];
            sscanf(buffer, "%d  %[^]", &pid, msg);//muestra el pid y mensaje

            cout << " proceso" << pid << "mensaje " << msg << endl;

            if (strncmp(msg, "reportar", 8) == 0) { //von esto llega lamsoliitud de reporte
                pid_t pidReportado;
                sscanf(msg, "reportar %d", &pidReportado); //se busca cual es el pid
                agregarReporte(pidReportado); // se reporta con la funcion de arriba
            }
        }
    }

    close(fd); //se cierra el procesp
    unlink(servidor_fifo);
    return 0;
}
