#include <iostream>
#include <cmath>
#include <mutex>
using namespace std;

// =============================
// Constantes
// =============================
const int FILAS = 30;
const int COLUMNAS = 20;

// =============================
// Estructuras
// =============================
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
// Variables globales
// =============================
int mapa[FILAS][COLUMNAS];
mutex mtx;

// =============================
// Funciones auxiliares
// =============================
void imprimirMapa() {
    lock_guard<mutex> lock(mtx);
    for (int i = 0; i < FILAS; i++) {
        for (int j = 0; j < COLUMNAS; j++) {
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
void moverHeroe(Heroe &heroe, Pos ruta[]) {
    lock_guard<mutex> lock(mtx);
    if (heroe.path_idx < heroe.path_len) {
        mapa[heroe.pos.x][heroe.pos.y] = 0;
        heroe.pos = ruta[heroe.path_idx];
        mapa[heroe.pos.x][heroe.pos.y] = 1;
        heroe.path_idx++;
        if (heroe.path_idx >= heroe.path_len)
            heroe.reached_goal = true;
    }
}

// =============================
// Función: comprobar visión
// =============================
void comprobarVision(Monstruo &monstruo, const Heroe& heroe) {
    int d = abs(monstruo.pos.x - heroe.pos.x) + abs(monstruo.pos.y - heroe.pos.y);
    if (d <= monstruo.vision_range && !monstruo.active) {
        monstruo.active = true;
        cout << "⚠️  Monstruo " << monstruo.id << " ha visto al héroe!" << endl;
    }
}

// =============================
// Función: mover monstruo
// =============================
void moverMonstruo(Monstruo &monstruo, const Heroe& heroe) {
    lock_guard<mutex> lock(mtx);

    int distanciaX = abs(monstruo.pos.x - heroe.pos.x);
    int distanciaY = abs(monstruo.pos.y - heroe.pos.y);
    int distancia = distanciaX + distanciaY;

    if (distancia > monstruo.vision_range) {
        monstruo.active = false;
        return;
    }

    monstruo.active = true;
    mapa[monstruo.pos.x][monstruo.pos.y] = 0;

    int dx = heroe.pos.x - monstruo.pos.x;
    int dy = heroe.pos.y - monstruo.pos.y;

    if (abs(dx) > abs(dy))
        monstruo.pos.x += (dx > 0 ? 1 : -1);
    else if (dy != 0)
        monstruo.pos.y += (dy > 0 ? 1 : -1);

    mapa[monstruo.pos.x][monstruo.pos.y] = 2;
}

// =============================
// NUEVA FUNCIÓN: ataque del héroe y monstruos
// =============================
void ejecutarAtaques(Heroe &heroe, Monstruo monstruos[], int numMonstruos) {
    lock_guard<mutex> lock(mtx);

    // ---- Héroe ataca primero ----
    if (!heroe.alive) return;

    bool atacó = false;
    for (int i = 0; i < numMonstruos; i++) {
        if (!monstruos[i].alive) continue;

        int dist = abs(heroe.pos.x - monstruos[i].pos.x) + abs(heroe.pos.y - monstruos[i].pos.y);
        if (dist <= heroe.attack_range) {
            monstruos[i].hp -= heroe.attack_damage;
            cout << "🗡️  Héroe ataca al Monstruo " << monstruos[i].id
                 << " (-" << heroe.attack_damage << " HP)" << endl;
            cout << "    Monstruo " << monstruos[i].id << " queda con " << monstruos[i].hp << " HP." << endl;
            atacó = true;

            if (monstruos[i].hp <= 0) {
                monstruos[i].alive = false;
                cout << "💀 Monstruo " << monstruos[i].id << " ha muerto." << endl;
            }
            break; // héroe solo ataca una vez
        }
    }

    if (!atacó)
        cout << "⚔️  Héroe no tiene enemigos en rango." << endl;

    // ---- Monstruos atacan luego ----
    for (int i = 0; i < numMonstruos; i++) {
        if (!monstruos[i].alive) continue;

        int dist = abs(monstruos[i].pos.x - heroe.pos.x) + abs(monstruos[i].pos.y - heroe.pos.y);
        if (dist <= monstruos[i].attack_range) {
            heroe.hp -= monstruos[i].attack_damage;
            cout << "🔥 Monstruo " << monstruos[i].id << " ataca al héroe (-"
                 << monstruos[i].attack_damage << " HP)" << endl;
            cout << "    Héroe queda con " << heroe.hp << " HP." << endl;

            if (heroe.hp <= 0) {
                heroe.alive = false;
                cout << "💀 ¡El héroe ha muerto!" << endl;
                return;
            }
        }
    }
}

// =============================
// Función principal
// =============================
int main() {
    // Inicializar mapa
    for (int i = 0; i < FILAS; i++)
        for (int j = 0; j < COLUMNAS; j++)
            mapa[i][j] = 0;

    // Crear héroe y monstruos
    Heroe heroe = {1, 100, 20, 1, {0, 0}, 0, 0, true, false};
    Monstruo monstruos[2] = {
        {1, 60, 10, 5, 1, {10, 10}, false, true},
        {2, 80, 15, 4, 1, {5, 8}, false, true}
    };
    const int numMonstruos = 2;

    // Ruta del héroe
    Pos ruta_heroe[] = {
        {0,0}, {1,0}, {2,0}, {3,0}, {4,0}, {5,0}, {6,0}, {7,0}, {8,0}, {9,0},
        {10,0}, {10,1}, {10,2}, {10,3}, {10,4}, {10,5}, {10,6}, {10,7}, {10,8}, {10,9}, {10,10}
    };
    heroe.path_len = sizeof(ruta_heroe)/sizeof(ruta_heroe[0]);

    mapa[heroe.pos.x][heroe.pos.y] = 1;
    for (int i = 0; i < numMonstruos; i++)
        mapa[monstruos[i].pos.x][monstruos[i].pos.y] = 2;

    cout << "=== JUEGO INICIADO ===\n";
    cout << "Presiona 'n' + Enter para avanzar turno.\n\n";

    char tecla;

    while (heroe.alive && !heroe.reached_goal) {
        cout << "Presiona 'n' para siguiente turno: ";
        cin >> tecla;
        if (tecla != 'n') continue;

        // --- Turno del héroe ---
        moverHeroe(heroe, ruta_heroe);

        // --- Monstruos reaccionan ---
        for (int i = 0; i < numMonstruos; i++) {
            comprobarVision(monstruos[i], heroe);
            moverMonstruo(monstruos[i], heroe);
        }

        // --- Ataques (héroe -> monstruos -> héroe) ---
        ejecutarAtaques(heroe, monstruos, numMonstruos);

        // --- Mostrar mapa ---
        imprimirMapa();

        // --- Condiciones de fin ---
        bool todosMuertos = true;
        for (int i = 0; i < numMonstruos; i++)
            if (monstruos[i].alive) todosMuertos = false;

        if (heroe.reached_goal) {
            cout << "🏆 ¡El héroe ha llegado a su objetivo!" << endl;
            break;
        }
        if (!heroe.alive) {
            cout << "💀 El héroe ha sido derrotado." << endl;
            break;
        }
        if (todosMuertos) {
            cout << "🔥 ¡Todos los monstruos han sido eliminados!" << endl;
            break;
        }
    }

    cout << "\n=== JUEGO TERMINADO ===" << endl;
    return 0;
}
