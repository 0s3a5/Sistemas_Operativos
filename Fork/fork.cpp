#include <iostream>
#include <unistd.h> // sirve para que se conecte al so el programa 
#include <sys/types.h> // libreria de los pids

using namespace std;

int main() {
    cout << "PID" << getpid() << endl; // se muetsra el ID del programa en c++

    pid_t pid = fork(); // crea un proceso hijo
    // el hijo empieza desde 0
    //el padre toma un valor distinto 
    // casi siempre char
    if (pid < 0) {
        perror("fork"); // elimina los erroes
        // si ocurre esto el proceso se ejecuto mal
        return 1;
    }

    if (pid == 0) {
        // Este es el hijo
        cout << "PID del hijo " << getpid() << " PPID (id del padre)" << getppid() << endl;
    } else {
        // Este es el padre
        cout << " PID" << getpid() <<  endl;
    }

    return 0;
}
