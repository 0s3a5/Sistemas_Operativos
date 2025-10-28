#include <iostream>
#include <vector>
#include <cmath>
#include <mutex>

using namespace std;

struct Pos {
    int x, y;
};

struct Heroe {
    int id, hp, attack_damage, attack_range;
    Pos pos;
    int path_len, path_idx;
    bool alive, reached_goal;
};

struct Monstruo {
    int id, hp, attack_damage, vision_range, attack_range;
    Pos pos;
    bool active, alive;
};

// =============================
// Función: imprimir mapa
// =============================
void imprimirMapa(const vector<vector<int>>& mapa) {
    for (int i = 0; i < (int)mapa.size(); i++) {
        for (int j = 0; j < (int)mapa[0].size(); j++) {
            if (mapa[i][j] == 0) cout << ".";
            else if (mapa[i][j] == 1) cout << "H";
            else if (mapa[i][j] == 2) cout << "M";
        }
        cout << "\n";
    }
    cout << endl;
}

// =============================
// Función: mover héroe
// =============================
void moverHeroe(Heroe &heroe, vector<vector<int>>& mapa, const vector<Pos>& ruta) {
    if (heroe.path_idx < heroe.path_len) {
        mapa[heroe.pos.x][heroe.pos.y] = 0; // limpia la posición anterior
        heroe.pos = ruta[heroe.path_idx];
        mapa[heroe.pos.x][heroe.pos.y] = 1; // marca nueva posición
        heroe.path_idx++;

        if (heroe.path_idx >= heroe.path_len)
            heroe.reached_goal = true;
    }
}

// =============================
// Función: mover monstruo
// =============================
void moverMonstruo(Monstruo &monstruo, const Heroe& heroe, vector<vector<int>>& mapa) {
    if (!monstruo.active) return;

    mapa[monstruo.pos.x][monstruo.pos.y] = 0;

    int movx = (heroe.pos.x > monstruo.pos.x) ? 1 : (heroe.pos.x < monstruo.pos.x ? -1 : 0);
    int movy = (heroe.pos.y > monstruo.pos.y) ? 1 : (heroe.pos.y < monstruo.pos.y ? -1 : 0);

    monstruo.pos.x += movx;
    monstruo.pos.y += movy;

    mapa[monstruo.pos.x][monstruo.pos.y] = 2;
}

// =============================
// Función: comprobar visión
// =============================
void comprobarVision(Monstruo &monstruo, const Heroe& heroe) {
    int dx = abs(monstruo.pos.x - heroe.pos.x);
    int dy = abs(monstruo.pos.y - heroe.pos.y);
    bool en_rango = (dx <= monstruo.vision_range && dy <= monstruo.vision_range);

    if (en_rango && !monstruo.active) {
        monstruo.active = true;
        cout << "⚠️  Monstruo " << monstruo.id << " ha visto al héroe!" << endl;
    }
}

// =============================
// Función principal
// =============================
int main() {
    const int largo = 30;
    const int ancho = 20;
    vector<vector<int>> mapa(largo, vector<int>(ancho, 0));
    mutex mutex_mapa;

    Heroe heroe = {1, 100, 20, 1, {0, 0}, 0, 0, true, false};
    Monstruo monstruo = {1, 100, 15, 5, 1, {10, 10}, false, true};

    vector<Pos> ruta_heroe = {
        {0,0}, {1,0}, {2,0}, {3,0}, {4,0}, {5,0}, {6,0}, {7,0}, {8,0}, {9,0},
        {10,0}, {10,1}, {10,2}, {10,3}, {10,4}, {10,5}, {10,6}, {10,7}, {10,8}, {10,9}, {10,10}
    };
    heroe.path_len = (int)ruta_heroe.size();

    mapa[heroe.pos.x][heroe.pos.y] = 1;
    mapa[monstruo.pos.x][monstruo.pos.y] = 2;

    cout << "=== JUEGO INICIADO ===\n";
    cout << "Presiona 'n' + Enter para avanzar turno.\n\n";

    char tecla;

    while (heroe.alive && monstruo.alive && !heroe.reached_goal) {
        cout << "Presiona 'n' para siguiente turno: ";
        cin >> tecla;
        if (tecla != 'n') continue;

        moverHeroe(heroe, mapa, ruta_heroe);
        comprobarVision(monstruo, heroe);
        moverMonstruo(monstruo, heroe, mapa);

        if (monstruo.pos.x == heroe.pos.x && monstruo.pos.y == heroe.pos.y) {
            heroe.alive = false;
            cout << "💀 El monstruo atrapó al héroe!" << endl;
        }

        imprimirMapa(mapa);

        if (heroe.reached_goal)
            cout << "🏆 ¡El héroe ha llegado a su objetivo!" << endl;
    }

    cout << "\n=== JUEGO TERMINADO ===" << endl;
    return 0;
}
