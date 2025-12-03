#pragma once
#include <vector>
#include <deque>
#include <string>

using namespace std;

// --- Estructuras de Proceso ---
struct PageInfo {
    int page_number;
    bool in_ram;
    int frame_index;
    bool in_swap;
    int swap_index;
};

class Process {
public:
    int pid;
    int size_kb;
    int num_pages;
    vector<PageInfo> pages;

    Process(int pid, int size_kb, int page_size_kb);
};

// --- Estructuras de Memoria ---
struct Frame {
    bool used;
    int pid;
    int page_number;
    bool is_swap;
};

class MemoryManager {
public:
    int page_size_kb;
    int frames_ram;
    int frames_swap;

    vector<Frame> ram;
    vector<Frame> swap;
    deque<int> fifo_order; // Para política FIFO

    MemoryManager(int page_size_kb, int frames_ram, int frames_swap);

    bool allocate_process(Process &proc);
    void free_process(const Process &proc);

    bool access_page(vector<Process> &processes,
                     Process &proc,
                     int page_index,
                     bool verbose = true);

    int free_ram_frames() const;
    int free_swap_frames() const;
    
    // --- NUEVO: Interfaz de estado del Trabajo A ---
    void print_status() const; 

private:
    int find_free_ram_frame() const;
    int find_free_swap_frame() const;
};
