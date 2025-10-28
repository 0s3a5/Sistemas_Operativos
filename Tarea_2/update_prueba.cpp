#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>
#include <vector>

using namespace std;

struct Pos {
    int x;
    int y;
};

struct Monstruo {
    int id;
    int hp;
    int attack_damage;
    int vision_range;
    int attack_range;
    Pos pos;
    bool active; // alertado / en persecución
    bool alive;
};

struct Heroe {
    int id;
    int hp;
    int attack_damage;
    int attack_range;
    Pos pos;
    int path_len;
    int path_idx;
    bool alive;
    bool reached_goal;
};

int largo = 30;
int ancho = 20;
int mapa[30][20] = {0};

std::mutex mutex_mapa; // mutex C++ moderno

void imprimir_mapa() {
    std::lock_guard<std::mutex> lock(mutex_mapa);
    // (Opcional) limpiar pantalla (funciona en muchos terminales)
    // cout << "\x1B[2J\x1B[H";
    for (int i = 0; i < largo; i++) {
        for (int j = 0; j < ancho; j++) {
            if (mapa[i][j] == 0) cout << ".";
            else if (mapa[i][j] == 1) cout << "H";
            else if (mapa[i][j] == 2) cout << "M";
        }
        cout << "\n";
    }
}

void activar_monstruo(Monstruo &monstruo) {
    std::lock_guard<std::mutex> lock(mutex_mapa);
    if (!monstruo.active) {
        monstruo.active = true;
        cout << "Monstruo " << monstruo.id << " activado!" << endl;
    }
}

void vigilar(Monstruo &monstruo) {
    while (monstruo.alive) {
        std::this_thread::sleep_for(std::chrono::milliseconds(400)); // pausa entre chequeos

        bool hero_detected = false;
        {
            std::lock_guard<std::mutex> lock(mutex_mapa);
            for (int i = -monstruo.vision_range; i <= monstruo.vision_range && !hero_detected; i++) {
                for (int j = -monstruo.vision_range; j <= monstruo.vision_range; j++) {
                    int x = monstruo.pos.x + i;
                    int y = monstruo.pos.y + j;
                    if (x >= 0 && x < largo && y >= 0 && y < ancho) {
                        if (mapa[x][y] == 1) { // héroe detectado
                            hero_detected = true;
                            break;
                        }
                    }
                }
            }
        }

        if (hero_detected) activar_monstruo(monstruo);
    }
}

void moverHeroe(Heroe &heroe, const vector<Pos> &ruta) {
    // mover un paso (si queda ruta)
    if (heroe.path_idx >= heroe.path_len) return;

    {
        std::lock_guard<std::mutex> lock(mutex_mapa);
        // limpiar posición anterior
        if (heroe.pos.x >= 0 && heroe.pos.x < largo && heroe.pos.y >= 0 && heroe.pos.y < ancho) {
            mapa[heroe.pos.x][heroe.pos.y] = 0;
        }

        // actualizar posición desde la ruta
        heroe.pos = ruta[heroe.path_idx];
        if (heroe.pos.x >= 0 && heroe.pos.x < largo && heroe.pos.y >= 0 && heroe.pos.y < ancho) {
            mapa[heroe.pos.x][heroe.pos.y] = 1;
        }

        imprimir_mapa();
        cout << "---------------------------------\n";
    }

    // incrementar índice de ruta y chequear objetivo
    heroe.path_idx++;
    if (heroe.path_idx >= heroe.path_len) {
        heroe.reached_goal = true;
    }
}

int main() {
    // Inicializar entidades
    Heroe heroe1 = {1, 100, 20, 1, {0, 0}, 0, 0, true, false};
    Monstruo monstruo1 = {1, 100, 15, 5, 1, {10, 10}, false, true};

    // definir ruta como vector de Pos
    vector<Pos> ruta_heroe = {
        {0,0}, {1,0}, {2,0}, {3,0}, {4,0}, {5,0}, {6,0}, {7,0}, {8,0}, {9,0},
        {10,0}, {10,1}, {10,2}, {10,3}, {10,4}, {10,5}, {10,6}, {10,7}, {10,8}, {10,9}, {10,10}
    };
    heroe1.path_len = static_cast<int>(ruta_heroe.size());

    // colocar en el mapa posiciones iniciales
    {
        std::lock_guard<std::mutex> lock(mutex_mapa);
        if (heroe1.pos.x >= 0 && heroe1.pos.x < largo && heroe1.pos.y >= 0 && heroe1.pos.y < ancho)
            mapa[heroe1.pos.x][heroe1.pos.y] = 1;
        if (monstruo1.pos.x >= 0 && monstruo1.pos.x < largo && monstruo1.pos.y >= 0 && monstruo1.pos.y < ancho)
            mapa[monstruo1.pos.x][monstruo1.pos.y] = 2;
    }

    // lanzar hilo vigilante del monstruo
    std::thread vigilante(vigilar, std::ref(monstruo1));

    // loop de movimiento del héroe
    while (heroe1.alive && !heroe1.reached_goal) {
        moverHeroe(heroe1, ruta_heroe);
        std::this_thread::sleep_for(std::chrono::milliseconds(300)); // velocidad del héroe
    }

    if (heroe1.reached_goal) {
        cout << "El héroe ha llegado a su objetivo!" << endl;
    }

    // terminar el hilo del monstruo limpiamente
    monstruo1.alive = false;
    if (vigilante.joinable()) vigilante.join();

    return 0;
}
