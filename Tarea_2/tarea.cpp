// Compilar: g++ -pthread -std=c++17 doom_pthreads.cpp -o doom_pthreads
// Ejecutar: ./doom_pthreads
// Autor: Esteban versión Linux con pthreads y semáforos POSIX
// Simulación paso a paso sin sleep()

#include <iostream>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <cstdlib>

using namespace std;

// ===============================================
// CONFIGURACIÓN
// ===============================================

#define GRID_H 20
#define GRID_W 30
#define MAX_MONSTERS 3
#define MAX_PATH 10

// ===============================================
// ESTRUCTURAS BÁSICAS
// ===============================================

struct Pos { int x; int y; };

struct Hero {
    int hp;
    int attack;
    int range;
    Pos pos;
    Pos path[MAX_PATH];
    int path_len;
    int step;
    bool alive;
    bool reached_goal;
};

struct Monster {
    int id;
    int hp;
    int attack;
    int vision;
    int range;
    Pos pos;
    bool alive;
    bool active;
};

// ===============================================
// VARIABLES GLOBALES
// ===============================================

Hero hero;
Monster monsters[MAX_MONSTERS];
char grid[GRID_H][GRID_W];

sem_t step_sem;
pthread_mutex_t grid_mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t sync_mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t sync_cv = PTHREAD_COND_INITIALIZER;
atomic<int> remaining;

// ===============================================
// FUNCIONES AUXILIARES
// ===============================================

int manhattan(Pos a, Pos b) {
    return abs(a.x - b.x) + abs(a.y - b.y);
}

bool inside(Pos p) {
    return p.x >= 0 && p.x < GRID_W && p.y >= 0 && p.y < GRID_H;
}

void init_grid() {
    for (int y = 0; y < GRID_H; y++)
        for (int x = 0; x < GRID_W; x++)
            grid[y][x] = '.';
}

void update_grid() {
    pthread_mutex_lock(&grid_mtx);
    init_grid();
    if (hero.alive && !hero.reached_goal)
        grid[hero.pos.y][hero.pos.x] = 'H';
    for (int i = 0; i < MAX_MONSTERS; i++)
        if (monsters[i].alive)
            grid[monsters[i].pos.y][monsters[i].pos.x] = 'M';
    pthread_mutex_unlock(&grid_mtx);
}

void print_grid() {
    pthread_mutex_lock(&grid_mtx);
    for (int y = 0; y < GRID_H; y++) {
        for (int x = 0; x < GRID_W; x++)
            cout << grid[y][x] << ' ';
        cout << endl;
    }
    cout << "--------------------------\n";
    cout << "Hero HP=" << hero.hp
         << " Pos=(" << hero.pos.x << "," << hero.pos.y << ")"
         << (hero.alive ? "" : " DEAD")
         << (hero.reached_goal ? " GOAL" : "")
         << endl;
    for (int i = 0; i < MAX_MONSTERS; i++) {
        cout << "Mon" << monsters[i].id
             << " HP=" << monsters[i].hp
             << " Pos=(" << monsters[i].pos.x << "," << monsters[i].pos.y << ")"
             << (monsters[i].alive ? (monsters[i].active ? " ACTIVE" : " PASSIVE") : " DEAD")
             << endl;
    }
    cout << "==========================\n";
    pthread_mutex_unlock(&grid_mtx);
}

void notify_done() {
    if (remaining.fetch_sub(1) == 1) {
        pthread_mutex_lock(&sync_mtx);
        pthread_cond_signal(&sync_cv);
        pthread_mutex_unlock(&sync_mtx);
    }
}

// ===============================================
// LÓGICA DE ATAQUES Y MOVIMIENTOS
// ===============================================

void hero_attack() {
    for (int i = 0; i < MAX_MONSTERS; i++) {
        if (!monsters[i].alive) continue;
        int dist = manhattan(hero.pos, monsters[i].pos);
        if (dist <= hero.range) {
            monsters[i].hp -= hero.attack;
            cout << "[Hero] ataca Mon" << monsters[i].id
                 << " (-" << hero.attack << ") HP=" << monsters[i].hp << endl;
            if (monsters[i].hp <= 0) {
                monsters[i].alive = false;
                cout << "Mon" << monsters[i].id << " muere.\n";
            }
        }
    }
}

void monster_attack(Monster &m) {
    int dist = manhattan(m.pos, hero.pos);
    if (dist <= m.range) {
        hero.hp -= m.attack;
        cout << "[Mon" << m.id << "] ataca Hero (-" << m.attack << ") HP=" << hero.hp << endl;
        if (hero.hp <= 0) {
            hero.alive = false;
            cout << "Hero muere.\n";
        }
    }
}

void move_monster(Monster &m) {
    if (!m.active || !hero.alive) return;
    int dist = manhattan(m.pos, hero.pos);
    if (dist <= m.range) return; // ya puede atacar
    Pos best = m.pos;
    int bestDist = dist;
    Pos dirs[4] = {
        {m.pos.x + 1, m.pos.y},
        {m.pos.x - 1, m.pos.y},
        {m.pos.x, m.pos.y + 1},
        {m.pos.x, m.pos.y - 1}
    };
    for (int i = 0; i < 4; i++) {
        if (!inside(dirs[i])) continue;
        int d = manhattan(dirs[i], hero.pos);
        if (d < bestDist) {
            bestDist = d;
            best = dirs[i];
        }
    }
    if (best.x != m.pos.x || best.y != m.pos.y) {
        cout << "[Mon" << m.id << "] se mueve ("
             << m.pos.x << "," << m.pos.y << ") -> ("
             << best.x << "," << best.y << ")\n";
        m.pos = best;
    }
}

// ===============================================
// HILOS
// ===============================================

void* hero_thread(void*) {
    while (hero.alive && !hero.reached_goal) {
        sem_wait(&step_sem);

        if (!hero.alive) { notify_done(); break; }

        hero_attack();

        if (hero.path_len > 0 && hero.step < hero.path_len) {
            Pos next = hero.path[hero.step];
            if (inside(next)) {
                cout << "[Hero] se mueve (" << hero.pos.x << "," << hero.pos.y
                     << ") -> (" << next.x << "," << next.y << ")\n";
                hero.pos = next;
            }
            hero.step++;
            if (hero.step >= hero.path_len) {
                hero.reached_goal = true;
                cout << "Hero alcanzó la meta.\n";
            }
        }

        notify_done();
    }
    return nullptr;
}

void* monster_thread(void* arg) {
    int idx = *((int*)arg);
    Monster &m = monsters[idx];
    while (m.alive) {
        sem_wait(&step_sem);
        if (!m.alive) { notify_done(); break; }

        int dist = manhattan(m.pos, hero.pos);
        if (dist <= m.vision) {
            m.active = true;
        }

        if (m.active && hero.alive) {
            if (dist <= m.range) monster_attack(m);
            else move_monster(m);
        }

        notify_done();
    }
    return nullptr;
}

// ===============================================
// MAIN
// ===============================================

int main() {
    // Parámetros por defecto (según PDF)
    hero.hp = 150;
    hero.attack = 20;
    hero.range = 3;
    hero.pos = {2, 2};
    hero.path_len = 6;
    hero.step = 0;
    hero.path[0] = {3,2};
    hero.path[1] = {4,2};
    hero.path[2] = {5,2};
    hero.path[3] = {5,3};
    hero.path[4] = {5,4};
    hero.path[5] = {6,4};
    hero.alive = true;
    hero.reached_goal = false;

    // Monstruos
    monsters[0] = {1, 50, 10, 5, 1, {8,4}, true, false};
    monsters[1] = {2, 50, 10, 5, 1, {15,10}, true, false};
    monsters[2] = {3, 80, 15, 4, 2, {5,8}, true, false};

    sem_init(&step_sem, 0, 0);
    init_grid();
    update_grid();
    print_grid();

    pthread_t hero_t;
    pthread_t mons_t[MAX_MONSTERS];
    int idx[MAX_MONSTERS] = {0,1,2};

    pthread_create(&hero_t, NULL, hero_thread, NULL);
    for (int i = 0; i < MAX_MONSTERS; i++)
        pthread_create(&mons_t[i], NULL, monster_thread, &idx[i]);

    // Control paso a paso
    while (true) {
        int alive_count = (hero.alive && !hero.reached_goal) ? 1 : 0;
        for (int i = 0; i < MAX_MONSTERS; i++)
            if (monsters[i].alive) alive_count++;

        if (alive_count == 0) break;

        cout << "Presiona ENTER para avanzar (" << alive_count << " actores activos)...\n";
        cin.get();

        remaining.store(alive_count);
        for (int i = 0; i < alive_count; i++) sem_post(&step_sem);

        pthread_mutex_lock(&sync_mtx);
        while (remaining.load() > 0)
            pthread_cond_wait(&sync_cv, &sync_mtx);
        pthread_mutex_unlock(&sync_mtx);

        update_grid();
        print_grid();

        bool any_mon_alive = false;
        for (int i = 0; i < MAX_MONSTERS; i++)
            if (monsters[i].alive) any_mon_alive = true;

        if (!hero.alive) {
            cout << "Heroe muerto. Fin.\n";
            break;
        }
        if (hero.reached_goal) {
            cout << "Heroe alcanzó la meta. Fin.\n";
            break;
        }
        if (!any_mon_alive) {
            cout << "Todos los monstruos murieron. Fin.\n";
            break;
        }
    }

    // Desbloquear hilos en espera
    for (int i = 0; i < (MAX_MONSTERS + 2); i++) sem_post(&step_sem);

    pthread_join(hero_t, NULL);
    for (int i = 0; i < MAX_MONSTERS; i++)
        pthread_join(mons_t[i], NULL);

    sem_destroy(&step_sem);

    cout << "Simulación finalizada.\n";
    return 0;
}
