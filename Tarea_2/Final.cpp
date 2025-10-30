#include <bits/stdc++.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <iostream>
using namespace std;

struct Pos { int x=0, y=0; };
struct Heroe {
    int id = 0;
    int hp = 0;
    int attack_damage = 0;
    int attack_range = 0;
    Pos pos;
    vector<Pos> ruta;
    int path_idx = 0;
    bool alive = true;
    bool reached_goal = false;
};
struct Monstruo {
    int id = 0;
    int hp = 0;
    int attack_damage = 0;
    int vision_range = 0;
    int attack_range = 0;
    Pos pos;
    bool active = false;
    bool alive = true;
};

int ROWS = 10;
int COLS = 10;

vector<vector<int>> mapa; 
vector<Heroe> heroes;
vector<Monstruo> monstruos;

pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER; // mutex para mapa
vector<sem_t> sem_heroes;   // semáforos para héroes
vector<sem_t> sem_monstruos; // semáforos para monstruos
sem_t sem_heroes_done;    // contador de héroes terminado
sem_t sem_monsters_done;  // contador de monstruos terminado 

bool juego_activo = true;

inline int distanciaManhattan(const Pos &a, const Pos &b) {
    return abs(a.x - b.x) + abs(a.y - b.y);
}

void imprimirMapaEnArchivoYConsola() {
    pthread_mutex_lock(&mtx);

    cout << "Mapa (" << ROWS << "x" << COLS << ")\n";

    // proteger si mapa no inicializado
    if ((int)mapa.size() != ROWS) {
        cout << "(mapa no inicializado completamente)\n";
    } else {
        for (int i = 0; i < ROWS; ++i) {
            for (int j = 0; j < COLS; ++j) {
                if (mapa[i][j] == 0) cout << ".";
                else if (mapa[i][j] == 1) cout << "H";
                else if (mapa[i][j] == 2) cout << "M";
                else cout << "?";
            }
            cout << "\n";
        }
    }
    cout << "\n";

    // Stats de héroes
    for (size_t i = 0; i < heroes.size(); i++) {
        cout << "Hero" << heroes[i].id;
        cout << " HP=" << heroes[i].hp
             << " Pos=(" << heroes[i].pos.x << "," << heroes[i].pos.y << ")";
        if (!heroes[i].alive) cout << " DEAD";
        if (heroes[i].reached_goal) cout << " GOAL";
        cout << "\n";
    }

    // Stats de monstruos
    for (size_t i = 0; i < monstruos.size(); i++) {
        cout << "Mon" << monstruos[i].id
             << " HP=" << monstruos[i].hp
             << " Pos=(" << monstruos[i].pos.x << "," << monstruos[i].pos.y << ")";
        if (!monstruos[i].alive) {
            cout << " DEAD";
        } else {
            if (monstruos[i].active) cout << " ACTIVE";
            else cout << " PASSIVE";
        }
        cout << "\n";
    }

    cout << "-----------------------------\n";

    pthread_mutex_unlock(&mtx);
}

void moverHeroeIndice(int idx) {
    Heroe &h = heroes[idx];
    pthread_mutex_lock(&mtx);
    if (!h.alive || h.reached_goal) {
        pthread_mutex_unlock(&mtx);
        return;
    }
    if (h.path_idx < (int)h.ruta.size()) {
        if (h.pos.x >= 0 && h.pos.x < ROWS && h.pos.y >= 0 && h.pos.y < COLS) {
            if (mapa[h.pos.x][h.pos.y] == 1) mapa[h.pos.x][h.pos.y] = 0;
        }
        h.pos = h.ruta[h.path_idx];
        if (h.pos.x >= 0 && h.pos.x < ROWS && h.pos.y >= 0 && h.pos.y < COLS) {
            mapa[h.pos.x][h.pos.y] = 1;
        }
        h.path_idx++;
        if (h.path_idx >= (int)h.ruta.size()) h.reached_goal = true;
    }
    pthread_mutex_unlock(&mtx);
}

void ataqueHeroeIndice(int idx) {
    Heroe &h = heroes[idx];
    pthread_mutex_lock(&mtx);
    if (!h.alive) { pthread_mutex_unlock(&mtx); return; }

    bool ataco = false;
    for (size_t j = 0; j < monstruos.size(); ++j) {
        Monstruo &m = monstruos[j];
        if (!m.alive) continue;
        int dist = distanciaManhattan(h.pos, m.pos);
        if (dist <= h.attack_range) {
            m.hp -= h.attack_damage;
            cout << "heroe" << h.id << " ataca a monstruo" << m.id << "\n";
            cout << " (-" << h.attack_damage << " HP) le queda " << max(0, m.hp) << "\n";
            if (m.hp <= 0) {
                m.alive = false;
                if (m.pos.x >= 0 && m.pos.x < ROWS && m.pos.y >= 0 && m.pos.y < COLS) {
                    mapa[m.pos.x][m.pos.y] = 0;
                }
                cout << "monstruo" << m.id << " murio\n";
            }
            ataco = true;
            break; // solo 1 ataque por turno
        }
    }
    if (!ataco) {
        cout << "heroe" << h.id << " sin monstruo en rango\n";
    }
    pthread_mutex_unlock(&mtx);
}

bool alerta_global = false;

void comprobarVisionYAccionarMonstruo(int midx) {
    Monstruo &m = monstruos[midx];
    int best = -1;
    int bestd = INT_MAX;

    pthread_mutex_lock(&mtx);
    for (size_t i = 0; i < heroes.size(); ++i) {
        if (!heroes[i].alive) continue;
        int d = distanciaManhattan(m.pos, heroes[i].pos);
        if (d < bestd) { bestd = d; best = (int)i; }
    }
    pthread_mutex_unlock(&mtx);

    if (best == -1) return;
    if (bestd <= m.vision_range) {
        pthread_mutex_lock(&mtx);
        alerta_global = true;
        if (!m.active) {
            m.active = true;
            cout << "monstruo " << m.id << " detecta heroe en (dist=" << bestd << ")\n";
        }
        pthread_mutex_unlock(&mtx);
    }
    if (alerta_global) {
        pthread_mutex_lock(&mtx);
        m.active = true;
        pthread_mutex_unlock(&mtx);
    } else {
        return;
    }

    pthread_mutex_lock(&mtx);
    Heroe &target = heroes[best];
    if (bestd <= m.attack_range && target.alive) {
        target.hp -= m.attack_damage;
        cout << "monstruo" << m.id << " ataca a heroe" << target.id
             << " (-" << m.attack_damage << " HP) vida " << max(0, target.hp) << "\n";
        if (target.hp <= 0) {
            target.alive = false;
            if (target.pos.x >= 0 && target.pos.x < ROWS && target.pos.y >= 0 && target.pos.y < COLS) {
                mapa[target.pos.x][target.pos.y] = 0;
            }
            cout << "heroe " << target.id << " murio\n";
        }
        pthread_mutex_unlock(&mtx);
        return;
    }
    pthread_mutex_unlock(&mtx);

    // mover una casilla hacia el héroe
    int dx = heroes[best].pos.x - m.pos.x;
    int dy = heroes[best].pos.y - m.pos.y;
    if (abs(dx) > abs(dy)) m.pos.x += (dx > 0 ? 1 : -1);
    else if (dy != 0) m.pos.y += (dy > 0 ? 1 : -1);

    pthread_mutex_lock(&mtx);
    if (m.pos.x >= 0 && m.pos.x < ROWS && m.pos.y >= 0 && m.pos.y < COLS) {
        mapa[m.pos.x][m.pos.y] = 2;
    }
    pthread_mutex_unlock(&mtx);
}

void* hiloHeroe(void* arg) {
    int idx = *((int*)arg);
    while (juego_activo && heroes[idx].alive && !heroes[idx].reached_goal) {
        sem_wait(&sem_heroes[idx]);
        if (!juego_activo) break;
        moverHeroeIndice(idx);
        ataqueHeroeIndice(idx);
        sem_post(&sem_heroes_done);
    }
    sem_post(&sem_heroes_done);
    return nullptr;
}

void* hiloMonstruo(void* arg) {
    int idx = *((int*)arg);
    while (juego_activo && monstruos[idx].alive) {
        sem_wait(&sem_monstruos[idx]);
        if (!juego_activo) break;
        comprobarVisionYAccionarMonstruo(idx);
        sem_post(&sem_monsters_done);
    }
    sem_post(&sem_monsters_done);
    return nullptr;
}

// utilidades de parsing
vector<string> splitWords(const string &s) {
    vector<string> out;
    istringstream iss(s);
    string w;
    while (iss >> w) out.push_back(w);
    return out;
}

string trim(const string &s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b-a+1);
}

bool startsWith(const string &s, const string &p) {
    if (s.size() < p.size()) return false;
    return s.substr(0, p.size()) == p;
}

vector<Pos> parsePathLines(const vector<string>& lines) {
    vector<Pos> out;
    for (auto &ln : lines) {
        string t = ln;
        for (char &c : t) if (c=='('||c==')'||c==',') c = ' ';
        istringstream iss(t);
        int a,b;
        while (iss >> a >> b) out.push_back(Pos{a,b});
    }
    return out;
}

bool cargarDesdeArchivo(const string &filename) {
    ifstream ifs(filename);
    if (!ifs) {
        cerr << "No se pudo abrir " << filename << endl;
        return false;
    }

    string line;
    vector<string> bufferPathLines;
    map<int, Heroe> tmpHeroes;
    map<int, Monstruo> tmpMonsters;

    while (getline(ifs, line)) {
        string s = trim(line);
        if (s.empty()) continue;
        if (s[0] == '#') continue;
        vector<string> parts = splitWords(s);
        if (parts.empty()) continue;

        if (parts[0] == "GRID_SIZE" && parts.size() >= 3) {
            ROWS = stoi(parts[1]);
            COLS = stoi(parts[2]);
            continue;
        }

        if (startsWith(parts[0], "HERO_")) {
            string key = parts[0];
            vector<string> toks;
            {
                istringstream is(key);
                string t;
                while (getline(is, t, '_')) toks.push_back(t);
            }
            if (toks.size() < 3) continue;
            int id = stoi(toks[1]);
            string prop = toks[2];

            if (prop == "PATH") {
                string rest = s.substr(s.find("PATH") + 4);
                bufferPathLines.clear();
                if (trim(rest).size() > 0) bufferPathLines.push_back(rest);
                streampos lastPos = ifs.tellg();
                string next;
                while (getline(ifs, next)) {
                    string tn = trim(next);
                    if (tn.empty()) { lastPos = ifs.tellg(); continue; }
                    if (tn.find('(') != string::npos) {
                        bufferPathLines.push_back(tn);
                        lastPos = ifs.tellg();
                    } else {
                        ifs.seekg(lastPos);
                        break;
                    }
                    lastPos = ifs.tellg();
                }
                vector<Pos> ruta = parsePathLines(bufferPathLines);
                tmpHeroes[id].ruta = ruta;
                tmpHeroes[id].id = id;
                continue;
            } else if (prop == "START") {
                string rest = s.substr(s.find("START") + 5);
                istringstream iss(rest);
                int x,y;
                if (iss >> x >> y) {
                    tmpHeroes[id].pos = Pos{x,y};
                    tmpHeroes[id].id = id;
                }
                continue;
            } else {
                string rest = s.substr(s.find(prop) + prop.size());
                istringstream iss(rest);
                int val;
                if (!(iss >> val)) {
                    if (parts.size() >= 2) val = stoi(parts[1]);
                    else continue;
                }
                if (prop == "HP") tmpHeroes[id].hp = val;
                else if (prop == "ATTACK_DAMAGE") tmpHeroes[id].attack_damage = val;
                else if (prop == "ATTACK_RANGE") tmpHeroes[id].attack_range = val;
                else if (prop == "ATTACK") { // fallback if wrote ATTACK <val>
                    tmpHeroes[id].attack_damage = val;
                }
                continue;
            }
        }

        if (startsWith(parts[0], "MONSTER_")) {
            vector<string> toks;
            {
                istringstream is(parts[0]);
                string t;
                while (getline(is, t, '_')) toks.push_back(t);
            }
            if (toks.size() < 3) continue;
            int id = stoi(toks[1]);
            string prop = toks[2];
            string rest;
            if (s.size() > parts[0].size()) rest = trim(s.substr(parts[0].size()));
            else rest = "";

            if (prop == "COORDS") {
                istringstream iss(rest);
                int x,y; if (iss >> x >> y) {
                    tmpMonsters[id].pos = Pos{x,y};
                    tmpMonsters[id].id = id;
                }
            } else {
                int val = 0;
                if (!rest.empty()) {
                    istringstream iss(rest); iss >> val;
                } else if (parts.size() >= 2) {
                    val = stoi(parts[1]);
                }
                if (prop == "HP") tmpMonsters[id].hp = val;
                else if (prop == "ATTACK_DAMAGE") tmpMonsters[id].attack_damage = val;
                else if (prop == "VISION_RANGE") tmpMonsters[id].vision_range = val;
                else if (prop == "ATTACK_RANGE") tmpMonsters[id].attack_range = val;
                else if (prop == "ATTACK") tmpMonsters[id].attack_damage = val;
                else if (prop == "VISION") tmpMonsters[id].vision_range = val;
            }
            continue;
        }

        // ignorar otras líneas (MONSTER_COUNT, etc.)
    }

    // transferir tmp a vectores reales
    heroes.clear();
    monstruos.clear();

    if (!tmpHeroes.empty()) {
        vector<int> keys;
        for (auto &p : tmpHeroes) keys.push_back(p.first);
        sort(keys.begin(), keys.end());
        for (int k : keys) {
            Heroe h = tmpHeroes[k];
            if (h.attack_range == 0) h.attack_range = 1;
            if (h.attack_damage == 0) h.attack_damage = 10;
            if (h.hp == 0) h.hp = 100;
            heroes.push_back(h);
        }
    }

    if (!tmpMonsters.empty()) {
        vector<int> keys;
        for (auto &p : tmpMonsters) keys.push_back(p.first);
        sort(keys.begin(), keys.end());
        for (int k : keys) {
            Monstruo m = tmpMonsters[k];
            if (m.attack_ran_
