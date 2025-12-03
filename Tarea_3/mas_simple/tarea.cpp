#include <iostream>
#include <vector>
#include <deque>
#include <string>
#include <random>
#include <chrono>
#include <thread>
#include <algorithm>
#include <iomanip> 
using namespace std;

struct InfoPag {
    int id_pag;    
    bool en_ram;  
    int marco_ram;   
    bool en_swap;  
    int idx_swap;    
};
class Proceso {
public:
    int pid;                 
    int tam_kb;              
    int num_pags;           
    vector<InfoPag> pags;    

    Proceso(int id, int tam, int tam_pag);
};

struct Marco {
    bool uso;      
    int pid;        
    int id_pag;     
};

class GestorMem {
public:
    int tam_pag;
    int total_ram;    
    int total_swap;   

    vector<Marco> ram;
    vector<Marco> swap;
    deque<int> cola_fifo; 

    GestorMem(int tam_p, int t_ram, int t_swap);

    bool asignar(Proceso &p);
    void liberar(const Proceso &p);
    bool acceder(vector<Proceso> &lista_procs, Proceso &p, int n_pag);
    
    void estado() const;
    int libres_ram() const;
    int libres_swap() const;

private:
    int buscar_ram() const;
    int buscar_swap() const;
};



Proceso::Proceso(int id, int tam, int tam_pag)
    : pid(id), tam_kb(tam) {

    if (tam_pag <= 0) tam_pag = 1;
    if (tam_kb <= 0) tam_kb = tam_pag;

    num_pags = (tam_kb + tam_pag - 1) / tam_pag;
    if (num_pags <= 0) num_pags = 1;

    pags.reserve(num_pags);

    for (int i = 0; i < num_pags; ++i) {
        InfoPag p;
        p.id_pag = i;
        p.en_ram = false;
        p.marco_ram = -1;
        p.en_swap = false;
        p.idx_swap = -1;
        pags.push_back(p);
    }
}


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

     cout << "  ram " << oc_ram << "/" << total_ram 
         << " (" << fixed << setprecision(1) << p_ram << "%)"
         << " | swap  " << oc_swap << "/" << total_swap 
         << " (" << fixed << setprecision(1) << p_swap << "%)"<< endl;
}

bool GestorMem::asignar(Proceso &p) {
cout << "creacion de nuevo pid" << p.pid << " (" << p.tam_kb << " KB " << p.num_pags << " paginas)"<<endl;
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
                cout << "memoria lleba pid" << p.pid << " rechazado"<<endl;
                return false;
            }
            swap[m_swap] = {true, p.pid, p.pags[i].id_pag};
            p.pags[i] = {p.pags[i].id_pag, false, -1, true, m_swap};
        }
    }
    return true;
}

void GestorMem::liberar(const Proceso &p) {
    cout << "matar proceso (pid)" << p.pid << endl;

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

bool GestorMem::acceder(vector<Proceso> &lista_procs, Proceso &p, int n_pag) {
    if (n_pag < 0 || n_pag >= p.num_pags) return false;
    InfoPag &pag = p.pags[n_pag];

    if (pag.en_ram) {
        cout << "acceso a pid" << p.pid << " Pag=" << n_pag << " en Marco " << pag.marco_ram << endl;
        return true;
    }
    
    if (!pag.en_swap) return false;
    cout << "ocurrio un page fault" << p.pid << " pagina " << n_pag << " esta en swap"<<endl;

    int destino = buscar_ram();

    if (destino == -1) {
        if (cola_fifo.empty()) return false;

        int victima_idx = cola_fifo.front();
        cola_fifo.pop_front();
        cola_fifo.erase(remove(cola_fifo.begin(), cola_fifo.end(), victima_idx), cola_fifo.end());

        Marco &m_victima = ram[victima_idx];

        Proceso *proc_v = nullptr;
        for (auto &pr : lista_procs) {
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
            cout << "swap lleno, no se puede reemplazar"<<endl;
            return false;
        }

        swap[m_swap] = {true, m_victima.pid, m_victima.id_pag};
        proc_v->pags[idx_pag_v] = {m_victima.id_pag, false, -1, true, m_swap};

        cout << "   cambio mediante fifo se reemplaza" << m_victima.pid << " a Swap " << m_swap << endl;
        
        ram[victima_idx] = {false, -1, -1};
        destino = victima_idx;
    }

    swap[pag.idx_swap] = {false, -1, -1}; 
    
    ram[destino] = {true, p.pid, n_pag};  
    pag = {n_pag, true, destino, false, -1};
    cola_fifo.erase(remove(cola_fifo.begin(), cola_fifo.end(), destino), cola_fifo.end());
    cola_fifo.push_back(destino);

    cout << "  se logro cambiar a ram con el marco " << destino << endl;
    return true;
}

int main() {
    cout << "inicio simulacion"<<endl;

    double ram_mb, pag_kb;
    cout << "ingrese memoria fisica (MB)"<<endl;
  if (!(cin >> ram_mb) || ram_mb <= 0) return 1;

    cout << "ingrese tamanñ Pagina (KB) "<<endl;
    if (!(cin >> pag_kb) || pag_kb <= 0) return 1;

    int min_proc = 1024; 
    int max_proc = (int)((ram_mb * 1024.0) * 0.30); 
    
    if (max_proc < min_proc) max_proc = min_proc;
 cout << "rangos de procesos"<< endl;
    cout << "   minimo: " << min_proc << " KB"<<endl;
    cout << "   maximo: " << max_proc << " KB(ejecucion para que no sea tan rapido ) "<<endl;

    random_device rd;
    mt19937 gen(rd());
    uniform_real_distribution<double> dist_factor(1.5, 4.5);

    double factor = dist_factor(gen);
    double vir_mb = ram_mb * factor;

    cout << "memoria fisica: " << ram_mb << " MB"<<endl;
    cout << "memoria virtual: " << vir_mb << " MB (x" << factor << ")"<<endl;


    int marcos_ram = (ram_mb * 1024.0) / pag_kb;
    int marcos_vir = (vir_mb * 1024.0) / pag_kb;
    int marcos_swap = marcos_vir - marcos_ram;
    
    if (marcos_swap < 0) marcos_swap = 0;
cout << "marcos de ram " << marcos_ram << endl;
    cout << "maros de swap " << marcos_swap << endl;
    GestorMem mem(pag_kb, marcos_ram, marcos_swap);
    vector<Proceso> lista_procs;
    int contador_pid = 1;

    auto inicio = chrono::steady_clock::now();
    int t_creacion = 0;
    int t_eventos = 0;

    uniform_int_distribution<int> dist_tam(min_proc, max_proc);

    cout << "inicio bucler"<<endl;

    while (true) {
        auto ahora = chrono::steady_clock::now();
        int seg = chrono::duration_cast<chrono::seconds>(ahora - inicio).count();

        if (seg - t_creacion >= 2) {
            t_creacion = seg;
            int tam = dist_tam(gen);
            Proceso nuevo(contador_pid++, tam, pag_kb);

            if (!mem.asignar(nuevo)) {
                cout << "memoria llena no se puede hacer mas proceso"<<endl;
                break;
            }
            lista_procs.push_back(nuevo);
            mem.estado(); 
        }
        if (seg >= 30 && (seg - t_eventos >= 5)) {
            t_eventos = seg;

            if (!lista_procs.empty()) {
                uniform_int_distribution<int> r(0, lista_procs.size() - 1);
                int idx = r(gen);
                mem.liberar(lista_procs[idx]);
                lista_procs.erase(lista_procs.begin() + idx);
                mem.estado(); 
            }

            if (!lista_procs.empty()) {
                uniform_int_distribution<int> r_proc(0, lista_procs.size() - 1);
                Proceso &p = lista_procs[r_proc(gen)];
                
                uniform_int_distribution<int> r_pag(0, p.num_pags - 1);
                int pag = r_pag(gen);

                if (!mem.acceder(lista_procs, p, pag)) {
                    cout << "murio la swap"<< endl;
                    break;
                }
                mem.estado(); 
            }
        }

        if (mem.libres_ram() == 0 && mem.libres_swap() == 0) {
            cout << "memorias llenas"<< endl;
            break;
        }

        this_thread::sleep_for(chrono::milliseconds(200));
    }

    return 0;
}
