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
        if (reportes[i].pid == pid) return i;
    }
    return -1;
}

void agregarReporte(pid_t pid) {
    int idx = buscarReporte(pid);
    if (idx == -1 && totalClientes < MAX_CLIENTS) {
        reportes[totalClientes].pid = pid;
        reportes[totalClientes].count = 0;
        idx = totalClientes;
        totalClientes++;
    }
    reportes[idx].count++;

    cout << "[Servidor] Proceso " << pid << " tiene " << reportes[idx].count << " reportes.\n";

    if (reportes[idx].count >= 10) {
        cout << "[Servidor] Proceso " << pid << " será terminado por acumulación de reportes.\n";
        kill(pid, SIGTERM);
    }
}

int main() {
    unlink(SERVER_FIFO);
    mkfifo(SERVER_FIFO, 0666);

    int fd = open(SERVER_FIFO, O_RDONLY);
    if (fd == -1) {
        perror("Error abriendo server_fifo");
        return 1;
    }

    char buffer[256];
    ofstream log("chat.log", ios::app);

    cout << "[Servidor] Esperando mensajes...\n";

    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int n = read(fd, buffer, sizeof(buffer));
        if (n > 0) {
            buffer[n] = '\0';

            // Mensajes con formato: "PID: mensaje"
            pid_t pid;
            char msg[200];
            sscanf(buffer, "%d: %[^\n]", &pid, msg);

            // Guardar en log
            log << "[" << pid << "] " << msg << endl;
            log.flush();

            cout << "[Servidor] " << pid << ": " << msg << endl;

            // Verificar si es reporte
            if (strncmp(msg, "reportar", 8) == 0) {
                pid_t pidReportado;
                sscanf(msg, "reportar %d", &pidReportado);
                agregarReporte(pidReportado);
            }
        }
    }

    close(fd);
    unlink(SERVER_FIFO);
    return 0;
}
