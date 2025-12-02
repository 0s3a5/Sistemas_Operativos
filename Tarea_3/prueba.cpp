#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <random>
#include <cmath>
#include <mutex>
#include <deque>   // Necesario para la política FIFO
#include <algorithm>

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
vector<Page*> RAM;  // Punteros a las páginas que están en RAM
vector<Page*> SWAP; // Punteros a las páginas que están en SWAP

// Cola para política FIFO: guarda los índices de los frames en orden de llegada
deque<int> fifo_queue; 

int page_size_kb;
int ram_size_mb;
int virtual_size_mb;
bool simulation_running = true; // Para detener los hilos limpiamente

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

// Busca un espacio libre en un vector (RAM o SWAP)
int find_free_index(const vector<Page*>& memory) {
    for (size_t i = 0; i < memory.size(); i++) {
        if (memory[i] == nullptr) return i;
    }
    return -1; // Lleno
}

// Función para mover una página de RAM a SWAP (Swap Out)
// Retorna el índice del frame liberado en RAM, o -1 si falla (Swap lleno)
int perform_swap_out() {
    if (fifo_queue.empty()) return -1; // No hay nada que sacar

    // Buscar espacio en Swap
    int swap_idx = find_free_index(SWAP);
    if (swap_idx == -1) {
        cout << "CRITICAL: SWAP LLENA. No se puede hacer swap out. Fin del programa.\n";
        exit(1); 
    }

    // 1. Elegir víctima (FIFO)
    int frame_victim = fifo_queue.front();
    fifo_queue.pop_front();

    Page* victim_page = RAM[frame_victim];

    // 2. Mover a Swap
    SWAP[swap_idx] = victim_page;
    victim_page->in_ram = false;
    victim_page->ram_frame = -1;
    victim_page->in_swap = true;
    victim_page->swap_index = swap_idx;

    // 3. Limpiar RAM
    RAM[frame_victim] = nullptr;

    cout << "   [SWAP OUT] Pag " << victim_page->page_number 
         << " (Proc " << victim_page->process_id << ") movida a SWAP slot " << swap_idx << endl;

    return frame_victim;
}

// Función para cargar página en RAM
bool load_page_into_ram(Page* pg) {
    int frame_idx = find_free_index(RAM);

    // Si no hay espacio, hacemos Swap Out
    if (frame_idx == -1) {
        frame_idx = perform_swap_out();
    }

    // Insertar página en RAM
    RAM[frame_idx] = pg;
    pg->in_ram = true;
    pg->ram_frame = frame_idx;
    
    // Si estaba en swap, actualizar estado
    if (pg->in_swap) {
        SWAP[pg->swap_index] = nullptr; // Liberar slot de swap
        pg->in_swap = false;
        pg->swap_index = -1;
    }

    // Agregar a la cola FIFO
    fifo_queue.push_back(frame_idx);
    return true;
}

// Crear un proceso cada 2 segundos
void create_process() {
    lock_guard<mutex> lock(mtx);

    // Calcular máximo tamaño permitido (ej. 30% de la RAM física para no saturar al instante)
    // Pero el enunciado dice input. Usaremos tu lógica random_int(1, 20)
    int size = random_int(1, max(1, ram_size_mb / 2)); 

    Process* p = new Process();
    p->pid = processes.empty() ? 1 : processes.back()->pid + 1;
    p->size_mb = size;
    p->num_pages = ceil((p->size_mb * 1024.0) / page_size_kb);

    cout << "\n[CREAR] Proceso PID " << p->pid 
         << " (" << p->size_mb << "MB) -> " 
         << p->num_pages << " paginas.\n";

    processes.push_back(p);

    // Crear páginas e intentar cargarlas en RAM
    for (int i = 0; i < p->num_pages; i++) {
        Page* pg = new Page();
        pg->process_id = p->pid;
        pg->page_number = i;
        p->pages.push_back(pg);

        // Intentar cargar inmediatamente
        load_page_into_ram(pg);
    }
}

// Finalizar proceso aleatorio
void kill_random_process() {
    lock_guard<mutex> lock(mtx);

    if (processes.empty()) return;

    int idx = random_int(0, processes.size() - 1);
    Process* p = processes[idx];

    cout << "\n[KILL] Terminando proceso PID " << p->pid << endl;

    // Liberar RAM y SWAP
    for (auto* pg : p->pages) {
        if (pg->in_ram) {
            RAM[pg->ram_frame] = nullptr;
            // IMPORTANTE: Remover de fifo_queue es complejo en deque estándar.
            // Para simplificar esta tarea, dejaremos el índice en la cola.
            // Cuando la cola saque este índice, verificaremos si es nulo o de otro proceso.
            // (En una implementación real se usaría una lista enlazada).
        }
        if (pg->in_swap) {
            SWAP[pg->swap_index] = nullptr;
        }
        delete pg; // Liberar memoria del objeto Page
    }

    processes.erase(processes.begin() + idx);
    delete p; // Liberar memoria del objeto Process
    
    // Limpieza de cola FIFO sucia (opcional pero recomendada para evitar bugs visuales)
    // Reconstruimos la cola solo con frames ocupados
    deque<int> clean_queue;
    for(int frame : fifo_queue) {
        if(RAM[frame] != nullptr) clean_queue.push_back(frame);
    }
    fifo_queue = clean_queue;
}

// Simular acceso a memoria
void simulate_memory_access() {
    lock_guard<mutex> lock(mtx);

    if (processes.empty()) return;

    // Elegir proceso aleatorio
    Process* p = processes[random_int(0, processes.size() - 1)];

    // Dirección virtual
    int max_addr = p->size_mb * 1024 * 1024; // MB a Bytes
    if (max_addr == 0) return;
    
    int vaddr = random_int(0, max_addr - 1);
    int page_idx = vaddr / (page_size_kb * 1024);

    if (page_idx >= p->pages.size()) return; // Seguridad

    cout << "[ACCESO] PID " << p->pid << " Pag " << page_idx;

    Page* pg = p->pages[page_idx];

    if (pg->in_ram) {
        cout << " -> HIT en RAM (frame " << pg->ram_frame << ")\n";
    } else if (pg->in_swap) {
        cout << " -> PAGE FAULT! Recuperando de SWAP...\n";
        load_page_into_ram(pg);
    } else {
        // Caso raro: página creada pero que no cupo ni en swap al inicio (si falla la lógica de creación)
        cout << " -> ERROR: Página no asignada (Segmentation Fault simulado)\n";
    }
}

void print_status() {
    // Calcular ocupación sin bloquear (aproximado) o bloqueando
    lock_guard<mutex> lock(mtx);
    int ram_used = 0;
    for(auto p : RAM) if(p) ram_used++;
    
    int swap_used = 0;
    for(auto p : SWAP) if(p) swap_used++;

    cout << "       >>> ESTADO: RAM " << ram_used << "/" << RAM.size() 
         << " | SWAP " << swap_used << "/" << SWAP.size() << " <<<\n";
}

// =======================================================
// Hilos de ejecución
// =======================================================

void process_creator_loop() {
    while (simulation_running) {
        create_process();
        print_status();
        this_thread::sleep_for(chrono::seconds(2));
    }
}

void periodic_events_loop() {
    // Esperar 30 segundos iniciales
    cout << "Esperando 30 segundos para iniciar eventos de finalización/acceso...\n";
    for(int i=0; i<30; i++) {
        if(!simulation_running) return;
        this_thread::sleep_for(chrono::seconds(1));
    }

    while (simulation_running) {
        kill_random_process();
        simulate_memory_access();
        print_status();
        this_thread::sleep_for(chrono::seconds(5));
    }
}

// =======================================================
// Programa principal
// =======================================================

int main() {
    cout << "Tamaño RAM (MB): ";
    if (!(cin >> ram_size_mb)) return 0;

    cout << "Tamaño página (KB): ";
    if (!(cin >> page_size_kb)) return 0;

    // Conversiones
    long long ram_total_kb = (long long)ram_size_mb * 1024;
    int num_frames = ram_total_kb / page_size_kb;

    RAM.resize(num_frames, nullptr);

    // Elegir memoria virtual 1.5x a 4.5x
    virtual_size_mb = random_int(ram_size_mb * 1.5, ram_size_mb * 4.5);

    long long virtual_total_kb = (long long)virtual_size_mb * 1024;
    int total_virtual_pages = virtual_total_kb / page_size_kb;
    int swap_pages = total_virtual_pages - num_frames;

    if (swap_pages <= 0) swap_pages = num_frames; // Mínimo de seguridad

    SWAP.resize(swap_pages, nullptr);

    cout << "--------------------------------\n";
    cout << "Frames en RAM: " << num_frames << endl;
    cout << "Slots en Swap: " << swap_pages << endl;
    cout << "--------------------------------\n";

    // Lanzar hilos
    thread t1(process_creator_loop);
    thread t2(periodic_events_loop);

    // Unimos los hilos (el programa correrá indefinidamente hasta Ctrl+C)
    t1.join();
    t2.join();

    return 0;
}
