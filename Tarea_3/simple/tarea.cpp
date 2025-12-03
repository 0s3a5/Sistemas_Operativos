#include <iostream>
#include <random>
#include <chrono>
#include <thread>
#include <algorithm>
#include <iomanip> // Para std::setprecision
#include "tarea.h"

using namespace std;

// ==========================================
// IMPLEMENTACIÓN DE PROCESS
// ==========================================

Process::Process(int pid_, int size_kb_, int page_size_kb)
    : pid(pid_), size_kb(size_kb_) {

    if (page_size_kb <= 0) page_size_kb = 1;
    if (size_kb <= 0) size_kb = page_size_kb;

    num_pages = (size_kb + page_size_kb - 1) / page_size_kb;
    if (num_pages <= 0) num_pages = 1;

    pages.clear();
    pages.reserve(num_pages);

    for (int i = 0; i < num_pages; ++i) {
        PageInfo p;
        p.page_number = i;
        p.in_ram = false;
        p.frame_index = -1;
        p.in_swap = false;
        p.swap_index = -1;
        pages.push_back(p);
    }
}

// ==========================================
// IMPLEMENTACIÓN DE MEMORY
// ==========================================

MemoryManager::MemoryManager(int page_size_kb_, int frames_ram_, int frames_swap_)
    : page_size_kb(page_size_kb_),
      frames_ram(frames_ram_),
      frames_swap(frames_swap_) {

    ram.resize(frames_ram);
    swap.resize(frames_swap);

    for (int i = 0; i < frames_ram; ++i) {
        ram[i].used = false;
        ram[i].pid = -1;
        ram[i].page_number = -1;
        ram[i].is_swap = false;
    }

    for (int i = 0; i < frames_swap; ++i) {
        swap[i].used = false;
        swap[i].pid = -1;
        swap[i].page_number = -1;
        swap[i].is_swap = true;
    }
}

int MemoryManager::find_free_ram_frame() const {
    for (int i = 0; i < frames_ram; ++i) {
        if (!ram[i].used) return i;
    }
    return -1;
}

int MemoryManager::find_free_swap_frame() const {
    for (int i = 0; i < frames_swap; ++i) {
        if (!swap[i].used) return i;
    }
    return -1;
}

int MemoryManager::free_ram_frames() const {
    int count = 0;
    for (const auto &f : ram) if (!f.used) ++count;
    return count;
}

int MemoryManager::free_swap_frames() const {
    int count = 0;
    for (const auto &f : swap) if (!f.used) ++count;
    return count;
}

// --- NUEVO: Función visual de estado (Estilo Trabajo A) ---
void MemoryManager::print_status() const {
    int ram_ocupada = frames_ram - free_ram_frames();
    int swap_ocupada = frames_swap - free_swap_frames();

    double ram_pct = (double)ram_ocupada / frames_ram * 100.0;
    double swap_pct = (frames_swap > 0) ? ((double)swap_ocupada / frames_swap * 100.0) : 0.0;

    cout << "   [ESTADO] RAM: " << ram_ocupada << "/" << frames_ram 
         << " (" << fixed << setprecision(1) << ram_pct << "%)"
         << " | SWAP: " << swap_ocupada << "/" << frames_swap 
         << " (" << fixed << setprecision(1) << swap_pct << "%)\n";
}

bool MemoryManager::allocate_process(Process &proc) {
    cout << "\n[CREAR] PID=" << proc.pid
         << " (" << proc.size_kb << " KB, " << proc.num_pages << " pgs)\n";

    for (int i = 0; i < proc.num_pages; ++i) {
        int frame = find_free_ram_frame();
        
        if (frame != -1) {
            ram[frame].used = true;
            ram[frame].pid = proc.pid;
            ram[frame].page_number = proc.pages[i].page_number;
            ram[frame].is_swap = false;

            proc.pages[i].in_ram = true;
            proc.pages[i].frame_index = frame;
            proc.pages[i].in_swap = false;
            proc.pages[i].swap_index = -1;

            fifo_order.erase(remove(fifo_order.begin(), fifo_order.end(), frame), fifo_order.end());
            fifo_order.push_back(frame);

        } else {
            int sframe = find_free_swap_frame();
            if (sframe == -1) {
                cout << "[ERROR] RAM y SWAP llenos. PID=" << proc.pid << " rechazado.\n";
                return false;
            }
            swap[sframe].used = true;
            swap[sframe].pid = proc.pid;
            swap[sframe].page_number = proc.pages[i].page_number;
            swap[sframe].is_swap = true;

            proc.pages[i].in_ram = false;
            proc.pages[i].frame_index = -1;
            proc.pages[i].in_swap = true;
            proc.pages[i].swap_index = sframe;
        }
    }
    return true;
}

void MemoryManager::free_process(const Process &proc) {
    cout << "\n[KILL] Finalizando PID=" << proc.pid << " (Liberando memoria...)\n";

    for (int i = 0; i < frames_ram; ++i) {
        if (ram[i].used && ram[i].pid == proc.pid) {
            ram[i].used = false;
            ram[i].pid = -1;
            ram[i].page_number = -1;
            fifo_order.erase(remove(fifo_order.begin(), fifo_order.end(), i), fifo_order.end());
        }
    }

    for (int i = 0; i < frames_swap; ++i) {
        if (swap[i].used && swap[i].pid == proc.pid) {
            swap[i].used = false;
            swap[i].pid = -1;
            swap[i].page_number = -1;
        }
    }
}

bool MemoryManager::access_page(vector<Process> &processes, Process &proc, int page_index, bool verbose) {
    if (page_index < 0 || page_index >= proc.num_pages) return false;

    PageInfo &page = proc.pages[page_index];

    if (page.in_ram) {
        if (verbose) {
            cout << "[ACCESO] HIT! PID=" << proc.pid << " Pag=" << page.page_number
                 << " en RAM Frame " << page.frame_index << "\n";
        }
        return true;
    }

    if (!page.in_swap) return false;

    if (verbose) {
        cout << "[ACCESO] PAGE FAULT! PID=" << proc.pid << " Pag=" << page.page_number << " esta en SWAP.\n";
    }

    int free_frame = find_free_ram_frame();

    if (free_frame == -1) {
        if (fifo_order.empty()) return false;

        int victim_frame = fifo_order.front();
        fifo_order.pop_front();
        fifo_order.erase(remove(fifo_order.begin(), fifo_order.end(), victim_frame), fifo_order.end());

        Frame &vict = ram[victim_frame];

        Process *vict_proc = nullptr;
        for (auto &p : processes) {
            if (p.pid == vict.pid) {
                vict_proc = &p;
                break;
            }
        }

        if (!vict_proc) return false;

        int vict_idx = -1;
        for (int i = 0; i < vict_proc->num_pages; i++) {
            if (vict_proc->pages[i].page_number == vict.page_number) {
                vict_idx = i;
                break;
            }
        }
        if (vict_idx == -1) return false;

        int swap_slot = find_free_swap_frame();
        if (swap_slot == -1) {
            cout << "[ERROR] Swap lleno. No se puede reemplazar.\n";
            return false;
        }

        swap[swap_slot].used = true;
        swap[swap_slot].pid = vict.pid;
        swap[swap_slot].page_number = vict.page_number;

        vict_proc->pages[vict_idx].in_ram = false;
        vict_proc->pages[vict_idx].frame_index = -1;
        vict_proc->pages[vict_idx].in_swap = true;
        vict_proc->pages[vict_idx].swap_index = swap_slot;

        ram[victim_frame].used = false;
        ram[victim_frame].pid = -1;
        ram[victim_frame].page_number = -1;

        free_frame = victim_frame;

        if (verbose) {
            cout << "   -> [FIFO SWAP OUT] Victima PID=" << vict.pid 
                 << " movida a SWAP Slot " << swap_slot << "\n";
        }
    }

    Frame &sw_slot = swap[page.swap_index];
    sw_slot.used = false;
    sw_slot.pid = -1;
    sw_slot.page_number = -1;

    Frame &rf = ram[free_frame];
    rf.used = true;
    rf.pid = proc.pid;
    rf.page_number = page.page_number;

    page.in_ram = true;
    page.frame_index = free_frame;
    page.in_swap = false;
    page.swap_index = -1;

    fifo_order.erase(remove(fifo_order.begin(), fifo_order.end(), free_frame), fifo_order.end());
    fifo_order.push_back(free_frame);

    if (verbose) {
        cout << "   -> [SWAP IN] Pagina cargada a RAM Frame " << free_frame << "\n";
    }

    return true;
}

// ==========================================
// PROGRAMA PRINCIPAL
// ==========================================

int main() {
    cout << "trabajo final final ";

    double mem_fisica_mb, tam_pagina_kb;

    cout << "Ingrese tamano de memoria fisica (MB): ";
    if (!(cin >> mem_fisica_mb) || mem_fisica_mb <= 0) return 1;

    cout << "Ingrese tamano de pagina (KB): ";
    if (!(cin >> tam_pagina_kb) || tam_pagina_kb <= 0) return 1;

    // --- LOGICA TRABAJO A: Rango calculado automáticamente ---
    // Minimo: 1 MB (1024 KB)
    // Maximo: 50% de la Memoria Fisica
    int min_proc_kb = 1024; 
    int max_proc_kb = (int)((mem_fisica_mb * 1024.0) * 0.50);
    
    // Seguridad por si la memoria es muy chica
    if (max_proc_kb < min_proc_kb) max_proc_kb = min_proc_kb;

   cout <<"rangos"<<endl;
    cout << "   Minimo: " << min_proc_kb << " KB\n";
    cout << "   Maximo: " << max_proc_kb << " KB (50% de RAM)\n";

    random_device rd;
    mt19937 gen(rd());
    uniform_real_distribution<double> dist_factor(1.5, 4.5);

    double factor = dist_factor(gen);
    double mem_virtual_mb = mem_fisica_mb * factor;

    cout << "\n[INFO] Memoria fisica: " << mem_fisica_mb << " MB\n";
    cout << "[INFO] Memoria virtual: " << mem_virtual_mb << " MB (factor " << factor << "x)\n";

    double mem_fisica_kb = mem_fisica_mb * 1024.0;
    double mem_virtual_kb = mem_virtual_mb * 1024.0;

    int frames_ram = mem_fisica_kb / tam_pagina_kb;
    int frames_virtual = mem_virtual_kb / tam_pagina_kb;
    int frames_swap = frames_virtual - frames_ram;

    if (frames_swap < 0) frames_swap = 0;

    cout << "[INFO] Frames RAM: " << frames_ram << "\n";
    cout << "[INFO] Frames SWAP: " << frames_swap << "\n\n";

    MemoryManager mem(tam_pagina_kb, frames_ram, frames_swap);
    vector<Process> procesos;
    int next_pid = 1;

    auto inicio = chrono::steady_clock::now();
    int ultimo_creado = 0;
    int ultimo_evento30 = 0;

    uniform_int_distribution<int> dist_size(min_proc_kb, max_proc_kb);

    cout << "[INFO] Iniciando simulacion...\n";

    while (true) {
        auto ahora = chrono::steady_clock::now();
        int seg = chrono::duration_cast<chrono::seconds>(ahora - inicio).count();

        // 1. Crear proceso (cada 2s)
        if (seg - ultimo_creado >= 2) {
            ultimo_creado = seg;
            int size_kb = dist_size(gen);
            Process nuevo(next_pid++, size_kb, tam_pagina_kb);

            if (!mem.allocate_process(nuevo)) {
                cout << "[FIN] Sin espacio critico en memoria.\n";
                break;
            }
            procesos.push_back(nuevo);
            mem.print_status(); // Mostrar estado visual
        }

        // 2. Eventos periódicos (cada 5s, después del segundo 30)
        if (seg >= 30 && (seg - ultimo_evento30 >= 5)) {
            ultimo_evento30 = seg;

            if (!procesos.empty()) {
                // Matar proceso
                uniform_int_distribution<int> distP(0, procesos.size() - 1);
                int idx = distP(gen);
                Process morir = procesos[idx];
                mem.free_process(morir);
                procesos.erase(procesos.begin() + idx);
                mem.print_status(); // Actualizar estado tras liberar
            }

            if (!procesos.empty()) {
                // Acceder memoria
                uniform_int_distribution<int> distP2(0, procesos.size() - 1);
                Process &p = procesos[distP2(gen)];
                uniform_int_distribution<int> distPage(0, p.num_pages - 1);
                int pagina = distPage(gen);

                if (!mem.access_page(procesos, p, pagina, true)) {
                    cout << "[FIN] Error critico en swap.\n";
                    break;
                }
                mem.print_status(); // Mostrar si hubo cambios en RAM/Swap
            }
        }

        // Fin si todo lleno
        if (mem.free_ram_frames() == 0 && mem.free_swap_frames() == 0) {
            cout << "\n[FIN] Memoria completamente llena.\n";
            break;
        }

        this_thread::sleep_for(chrono::milliseconds(200));
    }

    cout << "\nSimulacion terminada.\n";
    return 0;
}
