#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <random>
#include <cmath>
#include <mutex>

using namespace std;

// =======================================================
// Estructuras base
// =======================================================

struct Page {
    int process_id;
    int page_number;

    bool in_ram = false;
    int ram_frame = -1;

    bool in_swap = false;
    int swap_index = -1;
};

struct Process {
    int pid;
    int size_mb;
    int num_pages;

    vector<Page*> pages;
};

// =======================================================
// Variables globales
// =======================================================

vector<Process*> processes;
vector<Page*> RAM;
vector<Page*> SWAP;

int page_size_kb;
int ram_size_mb;
int virtual_size_mb;

mutex mtx;   // para proteger estructuras en hilos

// =======================================================
// Funciones auxiliares
// =======================================================

int random_int(int min, int max) {
    static random_device rd;
    static mt19937 gen(rd());
    uniform_int_distribution<> dist(min, max);
    return dist(gen);
}

// Crear un proceso cada 2 segundos
void create_process() {
    lock_guard<mutex> lock(mtx);

    Process* p = new Process();
    p->pid = processes.size() + 1;
    p->size_mb = random_int(1, 20);  // ejemplo
    p->num_pages = ceil((p->size_mb * 1024.0) / page_size_kb);

    cout << "Creando proceso PID " << p->pid 
         << " tamaño " << p->size_mb << "MB, " 
         << p->num_pages << " páginas.\n";

    // Crear páginas
    for (int i = 0; i < p->num_pages; i++) {
        Page* pg = new Page();
        pg->process_id = p->pid;
        pg->page_number = i;
        p->pages.push_back(pg);
    }

    processes.push_back(p);

    // TODO: intentar meter las páginas en RAM, o swap
}

// Finalizar proceso aleatorio
void kill_random_process() {
    lock_guard<mutex> lock(mtx);

    if (processes.empty()) return;

    int idx = random_int(0, processes.size() - 1);
    Process* p = processes[idx];

    cout << "Terminando proceso PID " << p->pid << endl;

    // Liberar RAM y SWAP
    for (auto* pg : p->pages) {
        if (pg->in_ram) {
            RAM[pg->ram_frame] = nullptr;
        }
        if (pg->in_swap) {
            SWAP[pg->swap_index] = nullptr;
        }
    }

    processes.erase(processes.begin() + idx);
    delete p;
}

// Simular acceso a memoria
void simulate_memory_access() {
    lock_guard<mutex> lock(mtx);

    if (processes.empty()) return;

    // Elegir proceso aleatorio
    Process* p = processes[random_int(0, processes.size() - 1)];

    // Dirección virtual
    int max_addr = p->size_mb * 1024 * 1024;
    int vaddr = random_int(0, max_addr);

    int page = vaddr / (page_size_kb * 1024);

    cout << "Acceso virtual: PID " << p->pid
         << " addr=" << vaddr
         << " -> page " << page << endl;

    Page* pg = p->pages[page];

    if (pg->in_ram) {
        cout << " → Página en RAM (frame " << pg->ram_frame << ")\n";
    } else {
        cout << " → PAGE FAULT!\n";
        // TODO: implementar swap-in y reemplazo
    }
}

// =======================================================
// Hilos de ejecución
// =======================================================

void process_creator_loop() {
    while (true) {
        create_process();
        this_thread::sleep_for(chrono::seconds(2));
    }
}

void periodic_events_loop() {
    // Esperar 30 segundos
    this_thread::sleep_for(chrono::seconds(30));

    while (true) {
        kill_random_process();
        simulate_memory_access();
        this_thread::sleep_for(chrono::seconds(5));
    }
}

// =======================================================
// Programa principal
// =======================================================

int main() {
    cout << "Tamaño RAM (MB): ";
    cin >> ram_size_mb;

    cout << "Tamaño página (KB): ";
    cin >> page_size_kb;

    int ram_kb = ram_size_mb * 1024;
    int num_frames = ram_kb / page_size_kb;

    RAM.resize(num_frames, nullptr);

    // Elegir memoria virtual 1.5x a 4.5x
    virtual_size_mb = random_int(ram_size_mb * 1.5, ram_size_mb * 4.5);

    int virtual_kb = virtual_size_mb * 1024;
    int virtual_pages = virtual_kb / page_size_kb;

    int swap_pages = virtual_pages - num_frames;

    SWAP.resize(swap_pages, nullptr);

    cout << "Frames en RAM: " << num_frames << endl;
    cout << "Páginas en Swap: " << swap_pages << endl;
    cout << "Memoria Virtual: " << virtual_size_mb << " MB\n\n";

    // Lanzar hilos
    thread t1(process_creator_loop);
    thread t2(periodic_events_loop);

    t1.join();
    t2.join();

    return 0;
}
