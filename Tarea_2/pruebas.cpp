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
// Función de visión (perímetro)
// Si el héroe está dentro de vision_range, activa al monstruo y notifica.
// =============================
void comprobarVision(Monstruo &m, const Heroe &h) {
    // No bloqueamos el mutex para la comprobación de distancia
    // (usamos mutex solo para prints y accesos a mapa/estados mutables si es necesario)
    int dist = distanciaManhattan(m.pos, h.pos);
    if (dist <= m.vision_range) {
        if (!m.active) {
            // marcar activo y notificar (protegido al imprimir/actualizar estado)
            pthread_mutex_lock(&mtx);
            m.active = true;
            cout << "👁️  Monstruo " << m.id << " ha detectado al héroe (dist=" << dist << ", vision=" << m.vision_range << ").\n";
            pthread_mutex_unlock(&mtx);
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
    // Stats rápidos
    cout << "Héroe HP=" << heroe.hp << " Pos=(" << heroe.pos.x << "," << heroe.pos.y << ")"
         << (heroe.alive ? "" : " DEAD") << (heroe.reached_goal ? " GOAL" : "") << "\n";
    for (int i = 0; i < 3; ++i) {
        cout << "Mon" << monstruos[i].id << " HP=" << monstruos[i].hp
             << " Pos=(" << monstruos[i].pos.x << "," << monstruos[i].pos.y << ")"
             << (monstruos[i].alive ? (monstruos[i].active ? " ACTIVE":" PASSIVE") : " DEAD") << "\n";
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
        // limpiar posición anterior (si está dentro del mapa)
        if (heroe.pos.x >= 0 && heroe.pos.x < FILAS && heroe.pos.y >= 0 && heroe.pos.y < COLUMNAS)
            mapa[heroe.pos.x][heroe.pos.y] = 0;
        // mover
        heroe.pos = heroe.ruta[heroe.path_idx];
        if (heroe.pos.x >= 0 && heroe.pos.x < FILAS && heroe.pos.y >= 0 && heroe.pos.y < COLUMNAS)
            mapa[heroe.pos.x][heroe.pos.y] = 1;
        heroe.path_idx++;
        if (heroe.path_idx >= heroe.path_len) heroe.reached_goal = true;
    }
    pthread_mutex_unlock(&mtx);
}

// =============================
// Ataque del héroe (1 ataque total por turno)
// Héroe ataca el primer monstruo vivo en su rango (si lo hay).
// =============================
void ataqueHeroe() {
    pthread_mutex_lock(&mtx);
    if (!heroe.alive) { pthread_mutex_unlock(&mtx); return; }

    bool ataco = false;
    for (int i = 0; i < 3; ++i) {
        if (!monstruos[i].alive) continue;
        int dist = distanciaManhattan(heroe.pos, monstruos[i].pos);
        if (dist <= heroe.attack_range) {
            monstruos[i].hp -= heroe.attack_damage;
            cout << "🗡️  Héroe ataca a Mon" << monstruos[i].id
                 << " (-" << heroe.attack_damage << " HP). Queda HP=" << max(0, monstruos[i].hp) << "\n";
            if (monstruos[i].hp <= 0) {
                monstruos[i].alive = false;
                // limpiar mapa
                if (monstruos[i].pos.x >=0 && monstruos[i].pos.x < FILAS && monstruos[i].pos.y >=0 && monstruos[i].pos.y < COLUMNAS)
                    mapa[monstruos[i].pos.x][monstruos[i].pos.y] = 0;
                cout << "💀 Mon" << monstruos[i].id << " ha muerto.\n";
            }
            ataco = true;
            break; // solo 1 ataque por turno del héroe
        }
    }
    if (!ataco) {
        cout << "⚔️  Héroe no encontró monstruos en su rango de ataque.\n";
    }
    pthread_mutex_unlock(&mtx);
}

// =============================
// Movimiento del monstruo (usa Manhattan greedy) y ataque si está en rango
// =============================
void moverYAtacarMonstruo(Monstruo &m) {
    // Primero comprobar visión (sin lock)
    comprobarVision(m, heroe);

    pthread_mutex_lock(&mtx);
    if (!m.alive || !heroe.alive || heroe.reached_goal) {
        pthread_mutex_unlock(&mtx);
        return;
    }

    int dist = distanciaManhattan(m.pos, heroe.pos);
    if (dist > m.vision_range) {
        m.active = false;
        pthread_mutex_unlock(&mtx);
        return;
    }

    // si activo, intentar atacar si en rango
    if (dist <= m.attack_range) {
        // atacar
        heroe.hp -= m.attack_damage;
        cout << "🔥 Mon" << m.id << " ataca al Héroe (-" << m.attack_damage << " HP). Héroe HP=" << max(0, heroe.hp) << "\n";
        if (heroe.hp <= 0) {
            heroe.alive = false;
            juego_activo = false;
            cout << "💀 ¡El héroe ha muerto!\n";
            // opcional: limpiar la casilla del héroe
            if (heroe.pos.x >=0 && heroe.pos.x < FILAS && heroe.pos.y >=0 && heroe.pos.y < COLUMNAS)
                mapa[heroe.pos.x][heroe.pos.y] = 0;
        }
        pthread_mutex_unlock(&mtx);
        return;
    }

    // si no puede atacar, moverse una casilla que reduzca la distancia
    // limpiar posición anterior
    if (m.pos.x >=0 && m.pos.x < FILAS && m.pos.y >=0 && m.pos.y < COLUMNAS)
        mapa[m.pos.x][m.pos.y] = 0;

    int dx = heroe.pos.x - m.pos.x;
    int dy = heroe.pos.y - m.pos.y;
    if (abs(dx) > abs(dy)) m.pos.x += (dx > 0 ? 1 : -1);
    else if (dy != 0) m.pos.y += (dy > 0 ? 1 : -1);

    // actualizar en mapa (si dentro)
    if (m.pos.x >=0 && m.pos.x < FILAS && m.pos.y >=0 && m.pos.y < COLUMNAS)
        mapa[m.pos.x][m.pos.y] = 2;

    cout << "[Mon" << m.id << "] se mueve a (" << m.pos.x << "," << m.pos.y << ")\n";
    pthread_mutex_unlock(&mtx);
}

// =============================
// Hilos
// =============================
void* hiloHeroe(void*) {
    while (juego_activo && heroe.alive && !heroe.reached_goal) {
        // hero tiene su semáforo; main dará permiso inicial
        sem_wait(&sem_heroe);

        // esperar tecla 'n' (se lee con bloqueo fuera del mutex)
        char tecla;
        do {
            cout << "Presiona 'n' + Enter para siguiente turno: ";
            cin >> tecla;
        } while (tecla != 'n');

        // mover + ataque del héroe
        moverHeroe();
        ataqueHeroe();

        // mostrar mapa parcial antes de los monstruos (opcional)
        imprimirMapa();

        // dar turno a cada monstruo uno por uno
        for (int i = 0; i < 3; ++i) {
            sem_post(&sem_monstruos[i]);
        }
    }
    // Si sale, desbloquear monstruos para que terminen si quedan esperando
    for (int i = 0; i < 3; ++i) sem_post(&sem_monstruos[i]);
    return nullptr;
}

void* hiloMonstruo(void* arg) {
    int idx = *((int*)arg);
  while (juego_activo && monstruos[idx].alive && heroe.alive && !heroe.reached_goal) {
        sem_wait(&sem_monstruos[idx]);

        if (!juego_activo) break;
        if (!monstruos[idx].alive) {
            // si murió mientras esperaba, el último devuelve turno al héroe
            if (idx == 2) sem_post(&sem_heroe);
            continue;
        }

        // comprobar visión, moverse y atacar si procede
        moverYAtacarMonstruo(monstruos[idx]);

        // si es el último monstruo, devolver turno al héroe (ciclo)
        if (idx == 2) sem_post(&sem_heroe);
    }
    return nullptr;
}

// =============================
// MAIN
// =============================
int main() {
    // Inicializar mapa vacío
    for (int i = 0; i < FILAS; ++i)
        for (int j = 0; j < COLUMNAS; ++j)
            mapa[i][j] = 0;

    // Inicializar héroe
    heroe.id = 1;
    heroe.hp = 150;
    heroe.attack_damage = 20;
    heroe.attack_range = 3;
    heroe.pos = {2,2};
    Pos ruta[] = {
        {3,2}, {4,2}, {5,2}, {5,3}, {5,4}, {6,4}
    };
    heroe.path_len = sizeof(ruta)/sizeof(ruta[0]);
    heroe.path_idx = 0;
    for (int i = 0; i < heroe.path_len; ++i) heroe.ruta[i] = ruta[i];
    heroe.alive = true;
    heroe.reached_goal = false;

    // Inicializar monstruos (ejemplo PDF; puedes cambiar)
    monstruos[0].id = 1; monstruos[0].hp = 50; monstruos[0].attack_damage = 10; monstruos[0].vision_range = 5; monstruos[0].attack_range = 1; monstruos[0].pos = {8,4}; monstruos[0].active = false; monstruos[0].alive = true;
    monstruos[1].id = 2; monstruos[1].hp = 50; monstruos[1].attack_damage = 10; monstruos[1].vision_range = 5; monstruos[1].attack_range = 1; monstruos[1].pos = {15,10}; monstruos[1].active = false; monstruos[1].alive = true;
    monstruos[2].id = 3; monstruos[2].hp = 80; monstruos[2].attack_damage = 15; monstruos[2].vision_range = 4; monstruos[2].attack_range = 2; monstruos[2].pos = {5,8}; monstruos[2].active = false; monstruos[2].alive = true;

    // Colocar en mapa
    mapa[heroe.pos.x][heroe.pos.y] = 1;
    for (int i = 0; i < 3; ++i) mapa[monstruos[i].pos.x][monstruos[i].pos.y] = 2;

    // Inicializar semáforos (héroe empieza)
    sem_init(&sem_heroe, 0, 1);
    for (int i = 0; i < 3; ++i) sem_init(&sem_monstruos[i], 0, 0);

    // Crear hilos
    pthread_t th_heroe;
    pthread_t th_monstruos[3];
    int ids[3] = {0,1,2};

    pthread_create(&th_heroe, nullptr, hiloHeroe, nullptr);
    for (int i = 0; i < 3; ++i) pthread_create(&th_monstruos[i], nullptr, hiloMonstruo, &ids[i]);

    // Esperar condición de final (hilo héroe terminará cuando juego_activo false o llegó meta)
    pthread_join(th_heroe, nullptr);
    for (int i = 0; i < 3; ++i) pthread_join(th_monstruos[i], nullptr);

    // Mostrar estado final
    imprimirMapa();
    if (heroe.reached_goal) cout << "🏆 El héroe alcanzó la meta.\n";
    else if (!heroe.alive) cout << "💀 El héroe fue derrotado.\n";
    else cout << "🛑 Simulación terminada.\n";

    // Limpiar semáforos y mutex
    sem_destroy(&sem_heroe);
    for (int i = 0; i < 3; ++i) sem_destroy(&sem_monstruos[i]);
    pthread_mutex_destroy(&mtx);

    return 0;
}
