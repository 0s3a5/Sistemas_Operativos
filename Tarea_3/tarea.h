#pragma once
#include <vector>
#include <deque>
#include <string>

// Usamos namespace std aquí para evitar escribir std:: en las clases
using namespace std;

// --- Estructuras de Proceso ---

struct PageInfo {
    int page_number;   // Numero de pagina dentro del proceso
    bool in_ram;       // true si esta cargada en un frame de RAM
    int frame_index;   // indice del frame en RAM (si in_ram == true)
    bool in_swap;      // true si esta almacenada en swap
    int swap_index;    // indice del frame en SWAP (si in_swap == true)
};

class Process {
public:
    int pid;                 // identificador del proceso
    int size_kb;             // tamano del proceso en KB
    int num_pages;           // numero de paginas
    vector<PageInfo> pages;  // vector sin std::

    Process(int pid, int size_kb, int page_size_kb);
};

// --- Estructuras de Memoria ---

struct Frame {
    bool used;        // true si el frame esta ocupado
    int pid;          // proceso que lo usa
    int page_number;  // numero de pagina del proceso
    bool is_swap;     // true si este frame pertenece al area de swap
};

class MemoryManager {
public:
    int page_size_kb;
    int frames_ram;
    int frames_swap;

    vector<Frame> ram;   // frames en memoria fisica
    vector<Frame> swap;  // frames en espacio de swap

    // Para politica FIFO de reemplazo
    deque<int> fifo_order;  // deque sin std::

    MemoryManager(int page_size_kb, int frames_ram, int frames_swap);

    bool allocate_process(Process &proc);
    void free_process(const Process &proc);

    bool access_page(vector<Process> &processes,
                     Process &proc,
                     int page_index,
                     bool verbose = true);

    int free_ram_frames() const;
    int free_swap_frames() const;

private:
    int find_free_ram_frame() const;
    int find_free_swap_frame() const;
};
