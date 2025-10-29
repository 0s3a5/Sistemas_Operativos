// juego.cpp
#include <iostream>
#include <pthread.h>
#include <semaphore.h>
#include <cmath>
#include <mutex>
#include <unistd.h>

using namespace std;

// =============================
// Constantes
// =============================
const int FILAS = 30;
const int FILAS = 20;
const int COLUMNAS = 20;

// =============================
@@ -18,29 +21,71 @@ struct Pos {
};

struct Heroe {
    int id, hp, attack_damage, attack_range;
    int id;
    int hp;
    int attack_damage;
    int attack_range;
    Pos pos;
    int path_len, path_idx;
    bool alive, reached_goal;
    Pos ruta[50];
    int path_len;
    int path_idx;
    bool alive;
    bool reached_goal;
};

struct Monstruo {
    int id, hp, attack_damage, vision_range, attack_range;
    int id;
    int hp;
    int attack_damage;
    int vision_range;
    int attack_range;
    Pos pos;
    bool active, alive;
    bool active;
    bool alive;
};

// =============================
// Variables globales
// =============================
int mapa[FILAS][COLUMNAS]; // mapa como arreglo bidimensional
mutex mtx; // para proteger la zona crítica
int mapa[FILAS][COLUMNAS];
Heroe heroe;
Monstruo monstruos[3];
pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
sem_t sem_heroe;
sem_t sem_monstruos[3];
bool juego_activo = true;

// =============================
// Funciones auxiliares
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
    lock_guard<mutex> lock(mtx); // zona crítica protegida
    pthread_mutex_lock(&mtx);
    for (int i = 0; i < FILAS; i++) {
        for (int j = 0; j < COLUMNAS; j++) {
            if (mapa[i][j] == 0) cout << ".";
@@ -50,126 +95,239 @@ void imprimirMapa() {
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
// Función: mover héroe
// Movimiento del héroe
// =============================
void moverHeroe(Heroe &heroe, Pos ruta[]) {
    lock_guard<mutex> lock(mtx); // bloquear mientras se mueve el héroe
void moverHeroe() {
    pthread_mutex_lock(&mtx);
    if (!heroe.alive || heroe.reached_goal) {
        pthread_mutex_unlock(&mtx);
        return;
    }
    if (heroe.path_idx < heroe.path_len) {
        mapa[heroe.pos.x][heroe.pos.y] = 0; // limpia posición anterior
        heroe.pos = ruta[heroe.path_idx];
        mapa[heroe.pos.x][heroe.pos.y] = 1; // nueva posición
        // limpiar posición anterior (si está dentro del mapa)
        if (heroe.pos.x >= 0 && heroe.pos.x < FILAS && heroe.pos.y >= 0 && heroe.pos.y < COLUMNAS)
            mapa[heroe.pos.x][heroe.pos.y] = 0;
        // mover
        heroe.pos = heroe.ruta[heroe.path_idx];
        if (heroe.pos.x >= 0 && heroe.pos.x < FILAS && heroe.pos.y >= 0 && heroe.pos.y < COLUMNAS)
            mapa[heroe.pos.x][heroe.pos.y] = 1;
        heroe.path_idx++;

        if (heroe.path_idx >= heroe.path_len)
            heroe.reached_goal = true;
        if (heroe.path_idx >= heroe.path_len) heroe.reached_goal = true;
    }
    pthread_mutex_unlock(&mtx);
}

// =============================
// Función: comprobar visión
// Ataque del héroe (1 ataque total por turno)
// Héroe ataca el primer monstruo vivo en su rango (si lo hay).
// =============================
void comprobarVision(Monstruo &monstruo, const Heroe& heroe) {
   lock_guard<mutex> lock(mtx); // zona crítica protegida (mapa)
    int d = abs(monstruo.pos.x - heroe.pos.x) + abs(monstruo.pos.y - heroe.pos.y);
    if (d <= monstruo.vision_range && !monstruo.active) {
        monstruo.active = true;
        cout << "⚠️  Monstruo " << monstruo.id << " ha visto al héroe!" << endl;
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
// NUEVA Función: mover monstruo (la tuya)
// Movimiento del monstruo (usa Manhattan greedy) y ataque si está en rango
// =============================
void moverMonstruo(Monstruo &monstruo, const Heroe& heroe) {
    lock_guard<mutex> lock(mtx); // zona crítica protegida (mapa)
void moverYAtacarMonstruo(Monstruo &m) {
    // Primero comprobar visión (sin lock)
    comprobarVision(m, heroe);

    pthread_mutex_lock(&mtx);
    if (!m.alive) { pthread_mutex_unlock(&mtx); return; }

    int distanciaX = abs(monstruo.pos.x - heroe.pos.x);
    int distanciaY = abs(monstruo.pos.y - heroe.pos.y);
    int distancia = distanciaX + distanciaY;
    int dist = distanciaManhattan(m.pos, heroe.pos);
    if (dist > m.vision_range) {
        m.active = false;
        pthread_mutex_unlock(&mtx);
        return;
    }

    // Si el héroe está fuera del rango de visión
    if (distancia > monstruo.vision_range) {
        monstruo.active = false;
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

    monstruo.active = true; // activo mientras lo ve
    // si no puede atacar, moverse una casilla que reduzca la distancia
    // limpiar posición anterior
    if (m.pos.x >=0 && m.pos.x < FILAS && m.pos.y >=0 && m.pos.y < COLUMNAS)
        mapa[m.pos.x][m.pos.y] = 0;

    int dx = heroe.pos.x - m.pos.x;
    int dy = heroe.pos.y - m.pos.y;
    if (abs(dx) > abs(dy)) m.pos.x += (dx > 0 ? 1 : -1);
    else if (dy != 0) m.pos.y += (dy > 0 ? 1 : -1);

    // Limpiar posición anterior
    mapa[monstruo.pos.x][monstruo.pos.y] = 0;
    // actualizar en mapa (si dentro)
    if (m.pos.x >=0 && m.pos.x < FILAS && m.pos.y >=0 && m.pos.y < COLUMNAS)
        mapa[m.pos.x][m.pos.y] = 2;

    // Calcular diferencias de posición
    int dx = heroe.pos.x - monstruo.pos.x;
    int dy = heroe.pos.y - monstruo.pos.y;
    cout << "[Mon" << m.id << "] se mueve a (" << m.pos.x << "," << m.pos.y << ")\n";
    pthread_mutex_unlock(&mtx);
}

    // Movimiento Manhattan: se mueve en la dirección que reduce la distancia
    if (abs(dx) > abs(dy)) {
        monstruo.pos.x += (dx > 0 ? 1 : -1);
    } else if (dy != 0) {
        monstruo.pos.y += (dy > 0 ? 1 : -1);
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
    while (juego_activo && monstruos[idx].alive && heroe.alive) {
        sem_wait(&sem_monstruos[idx]);

        if (!juego_activo) break;
        if (!monstruos[idx].alive) {
            // si murió mientras esperaba, el último devuelve turno al héroe
            if (idx == 2) sem_post(&sem_heroe);
            continue;
        }

    // Actualizar posición en el mapa
    mapa[monstruo.pos.x][monstruo.pos.y] = 2;
        // comprobar visión, moverse y atacar si procede
        moverYAtacarMonstruo(monstruos[idx]);

        // si es el último monstruo, devolver turno al héroe (ciclo)
        if (idx == 2) sem_post(&sem_heroe);
    }
    return nullptr;
}

// =============================
// Función principal
// MAIN
// =============================
int main() {
    // Inicializar mapa
    for (int i = 0; i < FILAS; i++)
        for (int j = 0; j < COLUMNAS; j++)
    // Inicializar mapa vacío
    for (int i = 0; i < FILAS; ++i)
        for (int j = 0; j < COLUMNAS; ++j)
            mapa[i][j] = 0;

    // Crear héroe y monstruo
    Heroe heroe = {1, 100, 20, 1, {0, 0}, 0, 0, true, false};
    Monstruo monstruo = {1, 100, 15, 5, 1, {10, 10}, false, true};

    // Definir ruta del héroe (arreglo)
    Pos ruta_heroe[] = {
        {0,0}, {1,0}, {2,0}, {3,0}, {4,0}, {5,0}, {6,0}, {7,0}, {8,0}, {9,0},
        {10,0}, {10,1}, {10,2}, {10,3}, {10,4}, {10,5}, {10,6}, {10,7}, {10,8}, {10,9}, {10,10}
    // Inicializar héroe
    heroe.id = 1;
    heroe.hp = 150;
    heroe.attack_damage = 20;
    heroe.attack_range = 3;
    heroe.pos = {2,2};
    Pos ruta[] = {
        {3,2}, {4,2}, {5,2}, {5,3}, {5,4}, {6,4}
    };
    heroe.path_len = sizeof(ruta_heroe)/sizeof(ruta_heroe[0]);
    heroe.path_len = sizeof(ruta)/sizeof(ruta[0]);
    heroe.path_idx = 0;
    for (int i = 0; i < heroe.path_len; ++i) heroe.ruta[i] = ruta[i];
    heroe.alive = true;
    heroe.reached_goal = false;

    // Colocar héroe y monstruo en el mapa
    // Inicializar monstruos (ejemplo PDF; puedes cambiar)
    monstruos[0].id = 1; monstruos[0].hp = 50; monstruos[0].attack_damage = 10; monstruos[0].vision_range = 5; monstruos[0].attack_range = 1; monstruos[0].pos = {8,4}; monstruos[0].active = false; monstruos[0].alive = true;
    monstruos[1].id = 2; monstruos[1].hp = 50; monstruos[1].attack_damage = 10; monstruos[1].vision_range = 5; monstruos[1].attack_range = 1; monstruos[1].pos = {15,10}; monstruos[1].active = false; monstruos[1].alive = true;
    monstruos[2].id = 3; monstruos[2].hp = 80; monstruos[2].attack_damage = 15; monstruos[2].vision_range = 4; monstruos[2].attack_range = 2; monstruos[2].pos = {5,8}; monstruos[2].active = false; monstruos[2].alive = true;

    // Colocar en mapa
    mapa[heroe.pos.x][heroe.pos.y] = 1;
    mapa[monstruo.pos.x][monstruo.pos.y] = 2;
    for (int i = 0; i < 3; ++i) mapa[monstruos[i].pos.x][monstruos[i].pos.y] = 2;

    cout << "=== JUEGO INICIADO ===\n";
    cout << "Presiona 'n' + Enter para avanzar turno.\n\n";
    // Inicializar semáforos (héroe empieza)
    sem_init(&sem_heroe, 0, 1);
    for (int i = 0; i < 3; ++i) sem_init(&sem_monstruos[i], 0, 0);

    char tecla;
    // Crear hilos
    pthread_t th_heroe;
    pthread_t th_monstruos[3];
    int ids[3] = {0,1,2};

    while (heroe.alive && monstruo.alive && !heroe.reached_goal) {
        cout << "Presiona 'n' para siguiente turno: ";
        cin >> tecla;
        if (tecla != 'n') continue;
    pthread_create(&th_heroe, nullptr, hiloHeroe, nullptr);
    for (int i = 0; i < 3; ++i) pthread_create(&th_monstruos[i], nullptr, hiloMonstruo, &ids[i]);

        // --- Turno del héroe ---
        moverHeroe(heroe, ruta_heroe);
    // Esperar condición de final (hilo héroe terminará cuando juego_activo false o llegó meta)
    pthread_join(th_heroe, nullptr);
    for (int i = 0; i < 3; ++i) pthread_join(th_monstruos[i], nullptr);

        // --- Turno del monstruo ---
        comprobarVision(monstruo, heroe);
        moverMonstruo(monstruo, heroe);
    // Mostrar estado final
    imprimirMapa();
    if (heroe.reached_goal) cout << "🏆 El héroe alcanzó la meta.\n";
    else if (!heroe.alive) cout << "💀 El héroe fue derrotado.\n";
    else cout << "🛑 Simulación terminada.\n";

        // --- Verificar colisión ---
        if (monstruo.pos.x == heroe.pos.x && monstruo.pos.y == heroe.pos.y) {
            heroe.alive = false;
            cout << "💀 El monstruo atrapó al héroe!" << endl;
        }
    // Limpiar semáforos y mutex
    sem_destroy(&sem_heroe);
    for (int i = 0; i < 3; ++i) sem_destroy(&sem_monstruos[i]);
    pthread_mutex_destroy(&mtx);

        // --- Mostrar mapa ---
        imprimirMapa();

        if (heroe.reached_goal)
            cout << "🏆 ¡El héroe ha llegado a su objetivo!" << endl;
    }

    cout << "\n=== JUEGO TERMINADO ===" << endl;
    return 0;
}
