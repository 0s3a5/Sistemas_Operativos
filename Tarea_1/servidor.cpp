#include <iostream>
#include <fstream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <csignal>
#include <cstring>

using namespace std;

#define SERVER_FIFO "server_fifo"
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
    unlink(SERVER_FIFO);
    mkfifo(SERVER_FIFO);

    int fd = open(SERVER_FIFO, O_RDONLY);//se deja proceso como solo de lectura
    if (fd == -1) {
        perror("no se abruo el server");
        return 1;
    }

    char buffer[256];
    cout << "prendiendo"<<endl;

    while (true) {
        
        pid_t pid;
            char msg[200];
            sscanf(buffer, "%d: %[^\n]", &pid, msg);//se lee el pid y el mensaje

            cout << "pid " << pid << " mensaje " << msg << endl;

            if (strncmp(msg, "reportar", 8) == 0) {//si es un reporte
                pid_t pidReportado;
                sscanf(msg, "reportar %d", &pidReportado);//se lee el pid
                agregarReporte(pidReportado);//se envia reportado
            }
        }
    }

    close(fd);
    unlink(SERVER_FIFO);
    return 0;
}
