#include <iostream>
#include <algorithm>
#include <iomanip>
#include "memoria.h"

using namespace std;

GestorMem::GestorMem(int tam_p, int t_ram, int t_swap)
    : tam_pag(tam_p), total_ram(t_ram), total_swap(t_swap) {

    ram.resize(total_ram, {false, -1, -1});
    swap.resize(total_swap, {false, -1, -1});
}

int GestorMem::buscar_ram() const {
    for (int i = 0; i < total_ram; ++i) if (!ram[i].uso) return i;
    return -1;
}

int GestorMem::buscar_swap() const {
    for (int i = 0; i < total_swap; ++i) if (!swap[i].uso) return i;
    return -1;
}

int GestorMem::libres_ram() const {
    int c = 0;
    for (const auto &m : ram) if (!m.uso) c++;
    return c;
}

int GestorMem::libres_swap() const {
    int c = 0;
    for (const auto &m : swap) if (!m.uso) c++;
    return c;
}

void GestorMem::estado() const {
    int oc_ram = total_ram - libres_ram();
    int oc_swap = total_swap - libres_swap();
    
    double p_ram = (double)oc_ram / total_ram * 100.0;
    double p_swap = (total_swap > 0) ? ((double)oc_swap / total_swap * 100.0) : 0.0;

    cout << "   [ESTADO] RAM: " << oc_ram << "/" << total_ram 
         << " (" << fixed << setprecision(1) << p_ram << "%)"
         << " | SWAP: " << oc_swap << "/" << total_swap 
         << " (" << fixed << setprecision(1) << p_swap << "%)\n";
}

bool GestorMem::asignar(Proceso &p) {
    cout << "\n[NUEVO] PID=" << p.pid << " (" << p.tam_kb << " KB, " << p.num_pags << " pags)\n";

    for (int i = 0; i < p.num_pags; ++i) {
        int m_ram = buscar_ram();
        
        if (m_ram != -1) {
            ram[m_ram] = {true, p.pid, p.pags[i].id_pag};
            p.pags[i] = {p.pags[i].id_pag, true, m_ram, false, -1};
            
            cola_fifo.erase(remove(cola_fifo.begin(), cola_fifo.end(), m_ram), cola_fifo.end());
            cola_fifo.push_back(m_ram);
        } else {
            int m_swap = buscar_swap();
            if (m_swap == -1) {
                cout << "[ERROR] Memoria LLENA. PID=" << p.pid << " rechazado.\n";
                return false;
            }
            swap[m_swap] = {true, p.pid, p.pags[i].id_pag};
            p.pags[i] = {p.pags[i].id_pag, false, -1, true, m_swap};
        }
    }
    return true;
}

void GestorMem::liberar(const Proceso &p) {
    cout << "\n[MATAR] PID=" << p.pid << " liberando memoria...\n";

    for (int i = 0; i < total_ram; ++i) {
        if (ram[i].uso && ram[i].pid == p.pid) {
            ram[i] = {false, -1, -1};
            cola_fifo.erase(remove(cola_fifo.begin(), cola_fifo.end(), i), cola_fifo.end());
        }
    }
    for (int i = 0; i < total_swap; ++i) {
        if (swap[i].uso && swap[i].pid == p.pid) {
            swap[i] = {false, -1, -1};
        }
    }
}

bool GestorMem::acceder(vector<Proceso> &procs, Proceso &p, int n_pag) {
    if (n_pag < 0 || n_pag >= p.num_pags) return false;
    InfoPag &pag = p.pags[n_pag];

    if (pag.en_ram) {
        cout << "[ACCESO] HIT! PID=" << p.pid << " Pag=" << n_pag << " en Marco " << pag.marco_ram << "\n";
        return true;
    }
    
    if (!pag.en_swap) return false;
    cout << "[ACCESO] PAGE FAULT! PID=" << p.pid << " Pag=" << n_pag << " esta en SWAP.\n";

    int destino = buscar_ram();

    if (destino == -1) {
        if (cola_fifo.empty()) return false;

        int victima_idx = cola_fifo.front();
        cola_fifo.pop_front();
        cola_fifo.erase(remove(cola_fifo.begin(), cola_fifo.end(), victima_idx), cola_fifo.end());

        Marco &m_victima = ram[victima_idx];

        Proceso *proc_v = nullptr;
        for (auto &pr : procs) {
            if (pr.pid == m_victima.pid) { proc_v = &pr; break; }
        }
        if (!proc_v) return false;

        int idx_pag_v = -1;
        for (int i = 0; i < proc_v->num_pags; i++) {
            if (proc_v->pags[i].id_pag == m_victima.id_pag) { idx_pag_v = i; break; }
        }
        if (idx_pag_v == -1) return false;

        int m_swap = buscar_swap();
        if (m_swap == -1) {
            cout << "[ERROR] Swap lleno, no se puede reemplazar.\n";
            return false;
        }

        swap[m_swap] = {true, m_victima.pid, m_victima.id_pag};
        proc_v->pags[idx_pag_v] = {m_victima.id_pag, false, -1, true, m_swap};

        cout << "   -> [FIFO OUT] Victima PID=" << m_victima.pid << " a Swap " << m_swap << "\n";
        
        ram[victima_idx] = {false, -1, -1};
        destino = victima_idx;
    }

    swap[pag.idx_swap] = {false, -1, -1}; 
    
    ram[destino] = {true, p.pid, n_pag};  
    pag = {n_pag, true, destino, false, -1}; 

    cola_fifo.erase(remove(cola_fifo.begin(), cola_fifo.end(), destino), cola_fifo.end());
    cola_fifo.push_back(destino);

    cout << "   -> [SWAP IN] Pagina cargada a RAM Marco " << destino << "\n";
    return true;
}
