#include "proceso.h"

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
