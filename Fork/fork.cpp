#include <iostream>
#include <unistd.h> // sirve para que se conecte al so el programa 
#include <sys/types.h> // libreria de los pids

using namespace std;

int main() {
    cout << "Inicio del programa, PID=" << getpid() << endl;

    pid_t pid = fork(); // crea un proceso hijo

    if (pid < 0) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        // Este es el hijo
        cout << "[Hijo] PID=" << getpid() << " PPID=" << getppid() << endl;
    } else {
        // Este es el padre
        cout << "[Padre] PID=" << getpid() << " creó hijo con PID=" << pid << endl;
    }

    return 0;
}
