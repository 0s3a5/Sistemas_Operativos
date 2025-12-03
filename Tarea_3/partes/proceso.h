#pragma once
#include <vector>

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
