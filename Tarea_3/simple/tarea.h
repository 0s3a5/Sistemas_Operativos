#pragma once
#include <vector>
#include <deque>
#include <string>
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

// --- Estructuras de Memoria ---

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
