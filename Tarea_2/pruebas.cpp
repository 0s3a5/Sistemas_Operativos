// juego.cpp
#include <iostream>
#include <pthread.h>
#include <semaphore.h>
#include <cmath>
#include <unistd.h>
using namespace std;

// =============================
// Constantes
// =============================
const int FILAS = 20;
const int COLUMNAS = 20;

// =============================
// Estructuras
// =============================
struct Pos {
    int x, y;
};

struct Heroe {
    int id;
    int hp;
    int attack_damage;
    int attack_range;
    Pos pos;
    Pos ruta[50];
    int path_len;
    int path_idx;
    bool alive;
    bool reached_goal;
};

struct Monstruo {
    int id;
    int hp;
    int attack_damage;
    int vision_range;
    int attack_range;
    Pos pos;
    bool active;
    bool alive;
};

// =============================
// Variables globales
// =============================
int mapa[FILAS][COLUMNAS];
Heroe heroe;
Monstruo monstruos[3];
pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mtx_heroe = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mtx_monstruos[3] = {
    PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER
};
sem_t sem_heroe;
sem_t sem_monstruos[3];
bool juego_activo = true;

// =============================
// Util: distancia Manhattan
// =============================
inline int distanciaManhattan(const Pos &a, const Pos &b) {
    return abs(a.x - b.x) + abs(a.y - b.y);
}

// =============================
// Función de visión
// =============================
void comprobarVision(Monstruo &monstruo, const Heroe &heroe) {
   lock_guard<mutex> lock(mtx);
    for(int i=(monstruo.pos.x-monstruo.vision_range); i<monstruo.pos.x+monstruo.vision_range; i++) {
        for(int j=(monstruo.pos.y-monstruo.vision_range); i<monstruo.pos.y+monstruo.vision_range;  j++) {
            if(mapa[i][j] == 2) {
                int distanciaX = abs(i - heroe.pos.x);
                int distanciaY = abs(j - heroe.pos.y);
                int distancia = distanciaX + distanciaY;

                if(distancia <= monstruo.vision_range) {
                    monstruo.active = true;
                    return;
                }
            }
        }
    }
}
}

// =============================
// Función para imprimir mapa
// =============================
void imprimirMapa() {
    pthread_mutex_lock(&mtx);
    for (int i = 0; i < FILAS; i++) {
        for (int j = 0; j < COLUMNAS; j++) {
            if (mapa[i][j] == 0) cout << ".";
            else if (mapa[i][j] == 1) cout << "H";
            else if (mapa[i][j] == 2) cout << "M";
        }
        cout << "\n";
    }
    cout << endl;
    cout << "Héroe HP=" << heroe.hp << " Pos=(" << heroe.pos.x << "," << heroe.pos.y << ")"
         << (heroe.alive ? "" : " DEAD") << (heroe.reached_goal ? " GOAL" : "") << "\n";
    for (int i = 0; i < 3; ++i) {
        cout << "Mon" << monstruos[i].id << " HP=" << monstruos[i].hp
             << " Pos=(" << monstruos[i].pos.x << "," << monstruos[i].pos.y << ")"
             << (monstruos[i].alive ? (monstruos[i].active ? " ACTIVE" : " PASSIVE") : " DEAD") << "\n";
    }
    cout << "-----------------------------\n";
    pthread_mutex_unlock(&mtx);
}

// =============================
// Movimiento del héroe
// =============================
void moverHeroe() {
    pthread_mutex_lock(&mtx);
    if (!heroe.alive || heroe.reached_goal) {
        pthread_mutex_unlock(&mtx);
        return;
    }
    if (heroe.path_idx < heroe.path_len) {
        if (heroe.pos.x >= 0 && heroe.pos.x < FILAS && heroe.pos.y >= 0 && heroe.pos.y < COLUMNAS)
            mapa[heroe.pos.x][heroe.pos.y] = 0;
        heroe.pos = heroe.ruta[heroe.path_idx];
        if (heroe.pos.x >= 0 && heroe.pos.x < FILAS && heroe.pos.y >= 0 && heroe.pos.y < COLUMNAS)
            mapa[heroe.pos.x][heroe.pos.y] = 1;
        heroe.path_idx++;
        if (heroe.path_idx >= heroe.path_len) heroe.reached_goal = true;
    }
    pthread_mutex_unlock(&mtx);
}

// =============================
// Movimiento del monstruo
// =============================
void moverMonstruo(Monstruo &m) {
    if (!m.alive || !heroe.alive) return;

    pthread_mutex_lock(&mtx_monstruos[m.id - 1]);
    comprobarVision(m, heroe);
    if (!m.active) {
        pthread_mutex_unlock(&mtx_monstruos[m.id - 1]);
        return;
    }

    int dist = distanciaManhattan(m.pos, heroe.pos);
    if (dist <= m.attack_range) {
        pthread_mutex_unlock(&mtx_monstruos[m.id - 1]);
        return;
    }

    if (m.pos.x >= 0 && m.pos.x < FILAS && m.pos.y >= 0 && m.pos.y < COLUMNAS)
        mapa[m.pos.x][m.pos.y] = 0;

    int dx = heroe.pos.x - m.pos.x;
    int dy = heroe.pos.y - m.pos.y;
    if (abs(dx) > abs(dy)) m.pos.x += (dx > 0 ? 1 : -1);
    else if (dy != 0) m.pos.y += (dy > 0 ? 1 : -1);

    if (m.pos.x >= 0 && m.pos.x < FILAS && m.pos.y >= 0 && m.pos.y < COLUMNAS)
        mapa[m.pos.x][m.pos.y] = 2;

    cout << "[Mon" << m.id << "] se mueve a (" << m.pos.x << "," << m.pos.y << ")\n";
    pthread_mutex_unlock(&mtx_monstruos[m.id - 1]);
}

// =============================
// Ataque del héroe
// =============================
void atacarHeroe() {
    pthread_mutex_lock(&mtx_heroe);
    if (!heroe.alive) { pthread_mutex_unlock(&mtx_heroe); return; }

    bool ataco = false;
    for (int i = 0; i < 3; ++i) {
        pthread_mutex_lock(&mtx_monstruos[i]);
        if (!monstruos[i].alive) {
            pthread_mutex_unlock(&mtx_monstruos[i]);
            continue;
        }

        int dist = distanciaManhattan(heroe.pos, monstruos[i].pos);
        if (dist <= heroe.attack_range) {
            monstruos[i].hp -= heroe.attack_damage;
            cout << "🗡️  Héroe ataca a Mon" << monstruos[i].id
                 << " (-" << heroe.attack_damage << " HP). Queda HP=" << max(0, monstruos[i].hp) << "\n";
            if (monstruos[i].hp <= 0) {
                monstruos[i].alive = false;
                if (monstruos[i].pos.x >= 0 && monstruos[i].pos.x < FILAS && monstruos[i].pos.y >= 0 && monstruos[i].pos.y < COLUMNAS)
                    mapa[monstruos[i].pos.x][monstruos[i].pos.y] = 0;
                cout << "💀 Mon" << monstruos[i].id << " ha muerto.\n";
            }
            ataco = true;
            pthread_mutex_unlock(&mtx_monstruos[i]);
            break;
        }
        pthread_mutex_unlock(&mtx_monstruos[i]);
    }

    if (!ataco)
        cout << "⚔️  Héroe no encontró monstruos en su rango de ataque.\n";

    pthread_mutex_unlock(&mtx_heroe);
}

// =============================
// Ataque de monstruo
// =============================
void atacarMonstruo(Monstruo &m) {
    pthread_mutex_lock(&mtx_monstruos[m.id - 1]);
    pthread_mutex_lock(&mtx_heroe);

    if (!m.alive || !heroe.alive) {
        pthread_mutex_unlock(&mtx_heroe);
        pthread_mutex_unlock(&mtx_monstruos[m.id - 1]);
        return;
    }

    int dist = distanciaManhattan(m.pos, heroe.pos);
    if (dist <= m.attack_range) {
        heroe.hp -= m.attack_damage;
        cout << "🔥 Mon" << m.id << " ataca al Héroe (-" << m.attack_damage
             << " HP). Héroe HP=" << max(0, heroe.hp) << "\n";

        if (heroe.hp <= 0) {
            heroe.alive = false;
            juego_activo = false;
            cout << "💀 ¡El héroe ha muerto!\n";
            if (heroe.pos.x >= 0 && heroe.pos.x < FILAS && heroe.pos.y >= 0 && heroe.pos.y < COLUMNAS)
                mapa[heroe.pos.x][heroe.pos.y] = 0;
        }
    }

    pthread_mutex_unlock(&mtx_heroe);
    pthread_mutex_unlock(&mtx_monstruos[m.id - 1]);
}

// =============================
// Hilo del héroe
// =============================
void* hiloHeroe(void*) {
    while (juego_activo && heroe.alive && !heroe.reached_goal) {
        sem_wait(&sem_heroe);

        char tecla;
        do {
            cout << "Presiona 'n' + Enter para siguiente turno: ";
            cin >> tecla;
        } while (tecla != 'n');

        moverHeroe();
        atacarHeroe();
        imprimirMapa();

        for (int i = 0; i < 3; ++i)
            sem_post(&sem_monstruos[i]);
    }

    for (int i = 0; i < 3; ++i) sem_post(&sem_monstruos[i]);
    return nullptr;
}

// =============================
// Hilo de monstruo
// =============================
void* hiloMonstruo(void* arg) {
    int idx = *((int*)arg);
    while (juego_activo && heroe.alive) {
        sem_wait(&sem_monstruos[idx]);
        if (!juego_activo) break;

        if (monstruos[idx].alive) {
            moverMonstruo(monstruos[idx]);
            atacarMonstruo(monstruos[idx]);
        }

        // Verifica si todos los monstruos ya terminaron su turno
        bool todos_listos = true;
        for (int i = 0; i < 3; ++i) {
            int val;
            sem_getvalue(&sem_monstruos[i], &val);
            if (val == 0 && (monstruos[i].alive || !heroe.alive)) {
                todos_listos = false;
                break;
            }
        }

        // Si todos actuaron, el héroe puede jugar nuevamente
        if (todos_listos) {
            sem_post(&sem_heroe);
        }
    }
    return nullptr;
}


// =============================
// MAIN
// =============================
int main() {
    for (int i = 0; i < FILAS; ++i)
        for (int j = 0; j < COLUMNAS; ++j)
            mapa[i][j] = 0;

    heroe.id = 1;
    heroe.hp = 150;
    heroe.attack_damage = 20;
    heroe.attack_range = 3;
    heroe.pos = {2, 2};
    Pos ruta[] = { {3,2}, {4,2}, {5,2}, {5,3}, {5,4}, {6,4} };
    heroe.path_len = sizeof(ruta) / sizeof(ruta[0]);
    heroe.path_idx = 0;
    for (int i = 0; i < heroe.path_len; ++i) heroe.ruta[i] = ruta[i];
    heroe.alive = true;
    heroe.reached_goal = false;

    monstruos[0] = {1, 50, 10, 5, 1, {8,4}, false, true};
    monstruos[1] = {2, 50, 10, 5, 1, {15,10}, false, true};
    monstruos[2] = {3, 80, 15, 4, 2, {5,8}, false, true};

    mapa[heroe.pos.x][heroe.pos.y] = 1;
    for (int i = 0; i < 3; ++i)
        mapa[monstruos[i].pos.x][monstruos[i].pos.y] = 2;

    sem_init(&sem_heroe, 0, 1);
    for (int i = 0; i < 3; ++i)
        sem_init(&sem_monstruos[i], 0, 0);

    pthread_t th_heroe;
    pthread_t th_monstruos[3];
    int ids[3] = {0, 1, 2};

    pthread_create(&th_heroe, nullptr, hiloHeroe, nullptr);
    for (int i = 0; i < 3; ++i)
        pthread_create(&th_monstruos[i], nullptr, hiloMonstruo, &ids[i]);

    pthread_join(th_heroe, nullptr);
    for (int i = 0; i < 3; ++i)
        pthread_join(th_monstruos[i], nullptr);

    imprimirMapa();
    if (heroe.reached_goal) cout << "🏆 El héroe alcanzó la meta.\n";
    else if (!heroe.alive) cout << "💀 El héroe fue derrotado.\n";
    else cout << "🛑 Simulación terminada.\n";

    sem_destroy(&sem_heroe);
    for (int i = 0; i < 3; ++i)
        sem_destroy(&sem_monstruos[i]);
    pthread_mutex_destroy(&mtx);
    pthread_mutex_destroy(&mtx_heroe);
    for (int i = 0; i < 3; ++i)
        pthread_mutex_destroy(&mtx_monstruos[i]);

    return 0;
}
