#include <iostream>
#include <random>
#include <chrono>
#include <thread>
#include "memoria.h"

using namespace std;

int main() {
    cout << "Simulador de Paginacion (ESP)\n\n";

    double ram_mb, pag_kb;

    cout << "Memoria Fisica (MB): ";
    if (!(cin >> ram_mb) || ram_mb <= 0) return 1;

    cout << "Tamano Pagina (KB): ";
    if (!(cin >> pag_kb) || pag_kb <= 0) return 1;
    int min_proc = 1024; 
    int max_proc = (int)((ram_mb * 1024.0) * 0.50);
    if (max_proc < min_proc) max_proc = min_proc;

    cout << "Rango procesos "<<endl;
    cout << "   Minimo: " << min_proc << " KB"<<endl;
    cout << "   Maximo: " << max_proc << " KB"<<endl;

    random_device rd;
    mt19937 gen(rd());
    uniform_real_distribution<double> dist_factor(1.5, 4.5);

    double factor = dist_factor(gen);
    double vir_mb = ram_mb * factor;

    cout << "memroia fisica: " << ram_mb << " MB"<<endl;
    cout << "memoria virtual: " << vir_mb << " MB (x" << factor << ")"<<endl;

    int marcos_ram = (ram_mb * 1024.0) / pag_kb;
    int marcos_vir = (vir_mb * 1024.0) / pag_kb;
    int marcos_swap = marcos_vir - marcos_ram;
    if (marcos_swap < 0) marcos_swap = 0;

    cout << "marcos ram " << marcos_ram << endl;
    cout << "marcos swap " << marcos_swap << endl;

    GestorMem mem(pag_kb, marcos_ram, marcos_swap);
    vector<Proceso> lista_procs;
    int contador_pid = 1;

    auto inicio = chrono::steady_clock::now();
    int t_creacion = 0;
    int t_eventos = 0;

    uniform_int_distribution<int> dist_tam(min_proc, max_proc);

    cout << "inicio de simualcion";

    while (true) {
        auto ahora = chrono::steady_clock::now();
        int seg = chrono::duration_cast<chrono::seconds>(ahora - inicio).count();

        if (seg - t_creacion >= 2) {
            t_creacion = seg;
            int tam = dist_tam(gen);
            Proceso nuevo(contador_pid++, tam, pag_kb);

            if (!mem.asignar(nuevo)) {
                cout << "[FIN] Memoria llena.\n";
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
                    cout << "termino por error en swap"<<endl;
                    break;
                }
                mem.estado(); 
            }
        }

        if (mem.libres_ram() == 0 && mem.libres_swap() == 0) {
            cout << "termino por memoria llena"<<endl;
            break;
        }

        this_thread::sleep_for(chrono::milliseconds(200));
    }

    return 0;
}
