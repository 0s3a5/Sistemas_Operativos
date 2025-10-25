// main.cpp
// Compilar: g++ -pthread -std=c++17 main.cpp -o doom_sim
// Ejecutar: ./doom_sim [config.txt]
// Usa POSIX semáforos (sem_init / sem_wait / sem_post) y no usa sleep()

#include <bits/stdc++.h>
#include <thread>
#include <atomic>
#include <semaphore.h>
#include <mutex>
#include <condition_variable>
#include <fstream>
#include <sstream>
#include <cstring>

using namespace std;

// ========== CONFIGURACIÓN FIJA ==========
const int GRID_W_DEF = 30;
const int GRID_H_DEF = 20;
const int MAX_MONSTERS = 3;
const int MAX_PATH = 100;

// ========== TIPOS SIMPLES (arreglos en vez de vectores) ==========
struct Pos { int x; int y; };

struct Monster {
    int id;
    int hp;
    int attack_damage;
    int vision_range;
    int attack_range;
    Pos pos;
    bool active; // alertado / en persecución
    bool alive;
};

struct Hero {
    int id;
    int hp;
    int attack_damage;
    int attack_range;
    Pos pos;
    Pos path[MAX_PATH];
    int path_len;
    int path_idx;
    bool alive;
    bool reached_goal;
};

// ========== GLOBALES (arreglos fijos) ==========
int GRID_W = GRID_W_DEF;
int GRID_H = GRID_H_DEF;

char grid_map[GRID_H_DEF][GRID_W_DEF]; // uso máximo fijo, pero solo indices < GRID_H/GRID_W serán usados
Hero hero;
Monster monsters[MAX_MONSTERS];
int monster_count = 0;

// sincronización
sem_t step_sem;                  // semáforo que libera 1 paso por cada actor vivo (se postea count veces)
atomic<int> step_remaining(0);   // cuenta de actores que aún no terminaron el paso actual
condition_variable_any step_cv;
mutex step_cv_mtx;
mutex grid_mtx;                  // protege impresión y accesos críticos

// util
int manhattan(const Pos &a, const Pos &b) {
    return abs(a.x - b.x) + abs(a.y - b.y);
}
bool inside_grid(const Pos &p) {
    return p.x >= 0 && p.x < GRID_W && p.y >= 0 && p.y < GRID_H;
}

// inicializa grid con '.'
void init_grid() {
    for (int y = 0; y < GRID_H; ++y)
        for (int x = 0; x < GRID_W; ++x)
            grid_map[y][x] = '.';
}

void place_entities_on_grid() {
    lock_guard<mutex> lk(grid_mtx);
    init_grid();
    if (hero.alive && !hero.reached_goal && inside_grid(hero.pos))
        grid_map[hero.pos.y][hero.pos.x] = 'H';
    for (int i = 0; i < monster_count; ++i) {
        if (monsters[i].alive && inside_grid(monsters[i].pos))
            grid_map[monsters[i].pos.y][monsters[i].pos.x] = 'M';
    }
}

void print_grid_and_stats() {
    lock_guard<mutex> lk(grid_mtx);
    for (int y = 0; y < GRID_H; ++y) {
        for (int x = 0; x < GRID_W; ++x) {
            cout << grid_map[y][x] << ' ';
        }
        cout << '\n';
    }
    cout << "-----------------------------\n";
    cout << "Hero HP: " << hero.hp << " pos(" << hero.pos.x << "," << hero.pos.y << ")"
         << (hero.alive ? "" : " DEAD") << (hero.reached_goal ? " GOAL" : "") << '\n';
    for (int i = 0; i < monster_count; ++i) {
        cout << "Mon" << monsters[i].id << " HP:" << monsters[i].hp << " pos(" << monsters[i].pos.x << "," << monsters[i].pos.y << ")"
             << (monsters[i].alive ? (monsters[i].active ? " ACTIVE" : " PASSIVE") : " DEAD") << '\n';
    }
    cout << "=============================\n";
}

// ========== PARSER SENCILLO (llaves del PDF) ==========
string trim(const string &s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b-a+1);
}

void parse_path_to_array(const string &line, Pos out_path[], int &out_len) {
    out_len = 0;
    size_t i = 0;
    while (i < line.size()) {
        size_t p1 = line.find('(', i);
        if (p1 == string::npos) break;
        size_t p2 = line.find(')', p1);
        if (p2 == string::npos) break;
        string inside = line.substr(p1+1, p2-p1-1);
        size_t comma = inside.find(',');
        if (comma != string::npos && out_len < MAX_PATH) {
            int x = stoi(trim(inside.substr(0, comma)));
            int y = stoi(trim(inside.substr(comma+1)));
            out_path[out_len].x = x;
            out_path[out_len].y = y;
            out_len++;
        }
        i = p2 + 1;
    }
}

void parse_config(const char *fname) {
    ifstream in(fname);
    if (!in) {
        cerr << "No se pudo abrir config (se usarán valores por defecto del PDF)\n";
        return;
    }
    string line;
    while (getline(in, line)) {
        string s = trim(line);
        if (s.empty()) continue;
        if (s.rfind("GRID_SIZE", 0) == 0) {
            istringstream ss(s.substr(strlen("GRID_SIZE")));
            ss >> GRID_W >> GRID_H;
        } else if (s.rfind("HERO_HP",0) == 0) {
            istringstream ss(s.substr(strlen("HERO_HP"))); ss >> hero.hp;
        } else if (s.rfind("HERO_ATTACK_DAMAGE",0) == 0) {
            istringstream ss(s.substr(strlen("HERO_ATTACK_DAMAGE"))); ss >> hero.attack_damage;
        } else if (s.rfind("HERO_ATTACK_RANGE",0) == 0) {
            istringstream ss(s.substr(strlen("HERO_ATTACK_RANGE"))); ss >> hero.attack_range;
        } else if (s.rfind("HERO_START",0) == 0) {
            istringstream ss(s.substr(strlen("HERO_START"))); ss >> hero.pos.x >> hero.pos.y;
        } else if (s.rfind("HERO_PATH",0) == 0) {
            parse_path_to_array(s, hero.path, hero.path_len);
            hero.path_idx = 0;
        } else if (s.rfind("MONSTER_COUNT",0) == 0) {
            istringstream ss(s.substr(strlen("MONSTER_COUNT"))); ss >> monster_count;
            if (monster_count > MAX_MONSTERS) monster_count = MAX_MONSTERS;
        } else if (s.rfind("MONSTER_",0) == 0) {
            // formato: MONSTER_i_FIELD value  o MONSTER_i_COORDS x y
            // extraer id y field
            // ejemplo: MONSTER_1_HP 50
            string key;
            istringstream ss(s);
            ss >> key;
            string rest;
            getline(ss, rest);
            rest = trim(rest);
            // split key por '_'
            vector<string> parts;
            size_t start = 0;
            for (;;) {
                size_t k = key.find('_', start);
                if (k == string::npos) { parts.push_back(key.substr(start)); break; }
                parts.push_back(key.substr(start, k-start));
                start = k+1;
            }
            if (parts.size() >= 3) {
                int id = stoi(parts[1]);
                string field = parts[2];
                if (id < 1 || id > MAX_MONSTERS) continue;
                Monster &m = monsters[id-1];
                m.id = id;
                if (field == "HP") m.hp = stoi(rest);
                else if (field == "ATTACK") {
                    if (key.find("ATTACK_DAMAGE") != string::npos) m.attack_damage = stoi(rest);
                } else if (field == "ATTACK_DAMAGE") m.attack_damage = stoi(rest);
                else if (field == "VISION") {
                    if (key.find("VISION_RANGE") != string::npos) m.vision_range = stoi(rest);
                } else if (field == "VISION_RANGE") m.vision_range = stoi(rest);
                else if (field == "ATTACK_RANGE") m.attack_range = stoi(rest);
                else if (field == "COORDS") {
                    istringstream ss2(rest);
                    ss2 >> m.pos.x >> m.pos.y;
                }
            }
        }
    }
}

// ========== COMPORTAMIENTO HEROE Y MONSTRUOS (un paso por ENTER) ==========
void notify_step_done() {
    if (step_remaining.fetch_sub(1) == 1) {
        unique_lock<mutex> lk(step_cv_mtx);
        step_cv.notify_all();
    }
}

// función hilo del héroe
void hero_thread_func() {
    while (hero.alive && !hero.reached_goal) {
        sem_wait(&step_sem); // espera que main libere 1 paso
        if (!hero.alive || hero.reached_goal) { notify_step_done(); break; }

        // primero verificar monstruos en rango de ataque
        int target_idx = -1;
        for (int i = 0; i < monster_count; ++i) {
            if (!monsters[i].alive) continue;
            if (manhattan(hero.pos, monsters[i].pos) <= hero.attack_range) {
                target_idx = i;
                break;
            }
        }

        if (target_idx != -1) {
            // atacar ese monstruo (un ataque por paso)
            Monster &m = monsters[target_idx];
            if (m.alive) {
                m.hp -= hero.attack_damage;
                cout << "[Hero] ataca Mon" << m.id << " (-" << hero.attack_damage << ") HPm=" << m.hp << "\n";
                if (m.hp <= 0) {
                    m.alive = false;
                    cout << "Mon" << m.id << " muere.\n";
                }
            }
        } else {
            // si no hay monstruo en rango, avanzar por la ruta (si existe)
            if (hero.path_idx < hero.path_len) {
                Pos next = hero.path[hero.path_idx];
                if (inside_grid(next)) {
                    cout << "[Hero] se mueve (" << hero.pos.x << "," << hero.pos.y << ") -> (" << next.x << "," << next.y << ")\n";
                    hero.pos = next;
                    hero.path_idx++;
                    if (hero.path_idx >= hero.path_len) {
                        hero.reached_goal = true;
                        cout << "[Hero] alcanzó la meta.\n";
                    }
                } else {
                    // fuera de grid: marcar goal para no quedar haciendo nada
                    hero.reached_goal = true;
                }
            } else {
                hero.reached_goal = true;
            }
        }
        notify_step_done();
    }
}

// activa (alerta) monstruos cercanos dentro de vision del monstruo idx
void alert_nearby(int idx) {
    Monster &m = monsters[idx];
    for (int j = 0; j < monster_count; ++j) {
        if (j == idx) continue;
        if (!monsters[j].alive) continue;
        if (manhattan(m.pos, monsters[j].pos) <= m.vision_range) {
            if (!monsters[j].active) {
                monsters[j].active = true;
                cout << "Mon" << m.id << " alerta a Mon" << monsters[j].id << "\n";
            }
        }
    }
}

// hilo de cada monstruo
void monster_thread_func(int idx) {
    Monster &m = monsters[idx];
    while (m.alive) {
        sem_wait(&step_sem);
        if (!m.alive) { notify_step_done(); break; }

        // revisar si ve al héroe
        bool sees = false;
        if (hero.alive && inside_grid(hero.pos) && inside_grid(m.pos)) {
            if (manhattan(m.pos, hero.pos) <= m.vision_range) {
                sees = true;
            }
        }
        if (sees) {
            if (!m.active) {
                m.active = true;
                cout << "Mon" << m.id << " vio al héroe y se activa.\n";
            }
            alert_nearby(idx);
        }

        if (m.active && m.alive && hero.alive) {
            int dist = manhattan(m.pos, hero.pos);
            if (dist <= m.attack_range) {
                // atacar HEROE
                hero.hp -= m.attack_damage;
                cout << "[Mon" << m.id << "] ataca Hero (-" << m.attack_damage << ") HPh=" << hero.hp << "\n";
                if (hero.hp <= 0) {
                    hero.alive = false;
                    cout << "Hero muere.\n";
                }
            } else {
                // mover una celda hacia el héroe (greedy Manhattan)
                Pos best = m.pos;
                int bestDist = dist;
                Pos cand[4] = {{m.pos.x+1,m.pos.y},{m.pos.x-1,m.pos.y},{m.pos.x,m.pos.y+1},{m.pos.x,m.pos.y-1}};
                for (int k=0;k<4;k++) {
                    Pos c = cand[k];
                    if (!inside_grid(c)) continue;
                    int d = manhattan(c, hero.pos);
                    if (d < bestDist) {
                        bestDist = d;
                        best = c;
                    }
                }
                if (best.x != m.pos.x || best.y != m.pos.y) {
                    cout << "[Mon" << m.id << "] se mueve ("<<m.pos.x<<","<<m.pos.y<<")->("<<best.x<<","<<best.y<<")\n";
                    m.pos = best;
                }
            }
        }
        notify_step_done();
    }
}

// ========== MAIN ==========
int main(int argc, char** argv) {
    // valores por defecto siguiendo el PDF ejemplo
    GRID_W = GRID_W_DEF; GRID_H = GRID_H_DEF;
    // héroe default (ejemplo PDF)
    hero.id = 1;
    hero.hp = 150;
    hero.attack_damage = 20;
    hero.attack_range = 3;
    hero.pos = {2,2};
    hero.path_len = 6;
    hero.path_idx = 0;
    hero.path[0] = {3,2};
    hero.path[1] = {4,2};
    hero.path[2] = {5,2};
    hero.path[3] = {5,3};
    hero.path[4] = {5,4};
    hero.path[5] = {6,4};
    hero.alive = true;
    hero.reached_goal = false;

    // monstruos default (ejemplo del PDF)
    monster_count = 3;
    for (int i=0;i<MAX_MONSTERS;i++) {
        monsters[i].id = i+1;
        monsters[i].alive = false;
        monsters[i].active = false;
        monsters[i].hp = 0;
        monsters[i].attack_damage = 0;
        monsters[i].vision_range = 0;
        monsters[i].attack_range = 0;
        monsters[i].pos = {0,0};
    }
    // default values as in PDF example
    monsters[0].hp = 50; monsters[0].attack_damage = 10; monsters[0].vision_range = 5; monsters[0].attack_range = 1; monsters[0].pos = {8,4}; monsters[0].alive = true;
    monsters[1].hp = 50; monsters[1].attack_damage = 10; monsters[1].vision_range = 5; monsters[1].attack_range = 1; monsters[1].pos = {15,10}; monsters[1].alive = true;
    monsters[2].hp = 80; monsters[2].attack_damage = 15; monsters[2].vision_range = 4; monsters[2].attack_range = 2; monsters[2].pos = {5,8}; monsters[2].alive = true;

    // si se entrega config, parsear y sobrescribir (respeta límites)
    if (argc >= 2) parse_config(argv[1]);

    // iniciar grid y mostrar estado inicial
    init_grid();
    place_entities_on_grid();
    print_grid_and_stats();

    // inicializar semáforo
    sem_init(&step_sem, 0, 0);

    // lanzar hilos: 1 héroe + monster_count monstruos
    thread hero_thread(hero_thread_func);
    thread monster_threads[MAX_MONSTERS];
    for (int i = 0; i < monster_count; ++i) {
        monster_threads[i] = thread(monster_thread_func, i);
    }

    // bucle principal: cada ENTER -> liberar un paso para cada actor vivo y esperar a que terminen
    while (true) {
        // contar actores vivos y no en goal
        int count = 0;
        if (hero.alive && !hero.reached_goal) count++;
        for (int i = 0; i < monster_count; ++i) if (monsters[i].alive) count++;
        if (count == 0) break; // simulación terminada
        cout << "Presiona ENTER para avanzar (actores activos = " << count << ")...\n";
        cin.get();

        step_remaining.store(count);
        for (int i = 0; i < count; ++i) sem_post(&step_sem);

        // esperar a que todos consuman su paso
        unique_lock<mutex> lk(step_cv_mtx);
        step_cv.wait(lk, [](){ return step_remaining.load() == 0; });

        // actualizar grid y mostrar
        place_entities_on_grid();
        print_grid_and_stats();

        // condición final: si héroe muerto o alcanzó meta o todos monstruos muertos
        bool any_mon_alive = false;
        for (int i=0;i<monster_count;++i) if (monsters[i].alive) any_mon_alive = true;
        if (!hero.alive) {
            cout << "El héroe ha muerto. Fin de la simulación.\n";
            break;
        }
        if (hero.reached_goal) {
            cout << "El héroe alcanzó la meta. Fin de la simulación.\n";
            break;
        }
        if (!any_mon_alive) {
            cout << "Todos los monstruos muertos. Fin de la simulación.\n";
            break;
        }
    }

    // asegurar desbloqueo de hilos (por si alguno quedó esperando)
    int posts = (1 + monster_count) * 2;
    for (int i=0;i<posts;++i) sem_post(&step_sem);

    // join hilos
    if (hero_thread.joinable()) hero_thread.join();
    for (int i=0;i<monster_count;++i) if (monster_threads[i].joinable()) monster_threads[i].join();

    sem_destroy(&step_sem);
    cout << "Simulación finalizada.\n";
    return 0;
}
