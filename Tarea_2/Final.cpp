#include <bits/stdc++.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
using namespace std;
struct Pos { int x=0, y=0; };
struct Heroe {
    int id;
    int hp;
    int attack_damage;
    int attack_range;
    Pos pos;
    vector<Pos> ruta;
    int path_idx = 0;
    bool alive = true;
    bool reached_goal = false;
};
struct Monstruo {
    int id;
    int hp;
    int attack_damage;
    int vision_range;
    int attack_range;
    Pos pos;
    bool active = false;
    bool alive = true;
};
int ROWS=10;
int COLS=10;
vector<vector<int>> mapa; 
vector<Heroe> heroes;
vector<Monstruo> monstruos;

pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER; //mutex para mapa
vector<sem_t> sem_heroes;  //semafroo para heroes donde atacan y mueven
vector<sem_t> sem_monstruos;//semaforo para monstruo que espera lo mismo que el de heroe
sem_t sem_heroes_done;    // dunciona como contador de heroes 
sem_t sem_monsters_done;  // funciona como contador de monstruos 

bool juego_activo = true;

inline int distanciaManhattan(const Pos &a, const Pos &b) {
    return abs(a.x - b.x) + abs(a.y - b.y);
}

void imprimirMapaEnArchivoYConsola() {
    pthread_mutex_lock(&mtx);

    ostringstream ss;
    ss << "Mapa (" << ROWS << "x" << COLS << ")\n";
   for (int i = 0; i < ROWS; ++i) {
        for (int j = 0; j < COLS; ++j) {
            if (mapa[i][j] == 0) cout << ".";
            else if (mapa[i][j] == 1) {
                cout << "H";
            }
            else if (mapa[i][j] == 2) {
                cout << "M";
            }
        }
        cout << endl;
    }
    cout << endl;
    // Stats
    for (auto &h : heroes) {
        ss << "heroe" << h.id << " HP=" << h.hp << " posicion=(" << h.pos.x << "," << h.pos.y << ")"
           << (h.alive ? "" : " muerto") << (h.reached_goal ? " llego" : "") << "\n";
    }
    for (auto &m : monstruos) {
        ss << "Mon" << m.id << " HP=" << m.hp << " Pos=(" << m.pos.x << "," << m.pos.y << ")"
           << (m.alive ? (m.active ? " activo" : " pasivo") : " muerto") << "\n";
    }
    ss << "-----------------------------\n";

     cout << ss.str();
    pthread_mutex_unlock(&mtx);
}

// -----------------------------
// Movimiento y ataques
// -----------------------------
void moverHeroeIndice(int idx) {
    Heroe &h = heroes[idx];
    pthread_mutex_lock(&mtx);
    if (!h.alive || h.reached_goal) {
        pthread_mutex_unlock(&mtx);
        return;
    }
    if (h.path_idx < (int)h.ruta.size()) {
        if (h.pos.x >= 0 && h.pos.x < ROWS && h.pos.y >= 0 && h.pos.y < COLS) {// limites del mapa
            if (mapa[h.pos.x][h.pos.y] == 1) mapa[h.pos.x][h.pos.y] = 0;//se cambia a posicion vacia
        }
        h.pos = h.ruta[h.path_idx];
        if (h.pos.x >= 0 && h.pos.x < ROWS && h.pos.y >= 0 && h.pos.y < COLS)// se verifica que no pase el mapa
            mapa[h.pos.x][h.pos.y] = 1;//se marca casilla como heroe
        h.path_idx++;
        if (h.path_idx >= (int)h.ruta.size()) h.reached_goal = true;//se verifica si llego a la posicion
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
            cout << "heroe" << h.id << " ataca a monstruo " << m.id
                 << " (-" << h.attack_damage << " HP). restante =" << max(0, m.hp) << "\n";
            if (m.hp <= 0) {
                m.alive = false;
                if (m.pos.x >= 0 && m.pos.x < ROWS && m.pos.y >=0 && m.pos.y < COLS)
                    mapa[m.pos.x][m.pos.y] = 0;
                cout << "monstruo" << m.id << " murio"<<endl;
            }
            ataco = true;
            break; 
        }
    }
    if (!ataco) {
        cout << "heroe" << h.id << " no tiene monstruos en rango"<<endl;
    }
    pthread_mutex_unlock(&mtx);
}

bool alerta_global = false;


void comprobarVisionYAccionarMonstruo(int midx) {
    Monstruo &m = monstruos[midx];
    int best = -1;//heroe mas cercano
    int bestd = INT_MAX;
    pthread_mutex_lock(&mtx);
    for (size_t i = 0; i < heroes.size(); ++i) {
        if (!heroes[i].alive) continue;
        int d = distanciaManhattan(m.pos, heroes[i].pos);
        if (d < bestd) { bestd = d; best = (int)i; }
    }
    pthread_mutex_unlock(&mtx);
    if (best == -1) return; // no hay heroes vivos
    if (bestd <= m.vision_range) {
        pthread_mutex_lock(&mtx);//si detecta monstruo se activan todos
        alerta_global = true;
        if (!m.active) {
            m.active = true;
            cout << "monstruo" << m.id << " vio al heroe en (dist=" << bestd << ")"<<endl;
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
    if (bestd <= m.attack_range && target.alive) { //si esta en rango ataca
        target.hp -= m.attack_damage;
        cout << "monstruo" << m.id << " ataca a heroe" << target.id << " (-" << m.attack_damage << " HP). restante " << max(0, target.hp) << "\n";
        if (target.hp <= 0) {//si muere heore
            target.alive = false;
            if (target.pos.x >= 0 && target.pos.x < ROWS && target.pos.y >= 0 && target.pos.y < COLS)
                mapa[target.pos.x][target.pos.y] = 0;//se cambia casilla a vacia
            cout << "heroe " << target.id << " murio "<<endl;
        }
        pthread_mutex_unlock(&mtx);
        return;
    }
    pthread_mutex_unlock(&mtx);// se mueve una casilla hacia el heroe si no esta en rango de ataque
    int dx = heroes[best].pos.x - m.pos.x;
    int dy = heroes[best].pos.y - m.pos.y;
    if (abs(dx) > abs(dy)) m.pos.x += (dx > 0 ? 1 : -1);
    else if (dy != 0) m.pos.y += (dy > 0 ? 1 : -1);

    pthread_mutex_lock(&mtx);
    if (m.pos.x >= 0 && m.pos.x < ROWS && m.pos.y >= 0 && m.pos.y < COLS)
        mapa[m.pos.x][m.pos.y] = 2;
    pthread_mutex_unlock(&mtx);
}

void* hiloHeroe(void* arg) {
    int idx = *((int*)arg);
    while (juego_activo && heroes[idx].alive && !heroes[idx].reached_goal) {
        sem_wait(&sem_heroes[idx]);//semaforo que se activa en main
        if (!juego_activo) break;
        moverHeroeIndice(idx);
        ataqueHeroeIndice(idx);
        sem_post(&sem_heroes_done);//semaforo indica que termino turno
    }
    sem_post(&sem_heroes_done);//le devuelve el termino al main
    return nullptr;//se cierra todo semaforo y puntero
}

void* hiloMonstruo(void* arg) {
    int idx = *((int*)arg);
    while (juego_activo && monstruos[idx].alive) {
        sem_wait(&sem_monstruos[idx]);//semaforo se activa en main
        if (!juego_activo) break;
        comprobarVisionYAccionarMonstruo(idx);

        sem_post(&sem_monsters_done);//indica termino de accion con semaforo
    }
    sem_post(&sem_monsters_done);//se devuelve termino al main
    return nullptr;//se cierra todo semaforo y puntero
}

vector<string> splitWords(const string &s) {
    vector<string> out;
    istringstream iss(s);
    string w;
    while (iss >> w) out.push_back(w);
    return out;
}

string trim(const string &s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a==string::npos) return "";
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
        for (char &c : t) if (c=='('||c==')'||c==',' ) c = ' ';
        istringstream iss(t);
        int a,b;
        while (iss >> a >> b) {
            out.push_back(Pos{a,b});
        }
    }
    return out;
}

bool cargarDesdeArchivo(const string &filename) {
    ifstream ifs(filename);
    if (!ifs) {
        cerr << "No se pudo abrir " << filename << "\n";
        return false;
    }

    string line;
    vector<string> bufferPathLines;
    int currentHeroId = 0;
    int currentMonsterId = 0;
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
            size_t p1 = key.find('_',5); 
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
                if (parts.size() >= 3) {
                    int x = stoi(parts[1+1]); 
                }
                string rest = s.substr(s.find("START") + 5);
                istringstream iss(rest);
                int x,y; if (iss >> x >> y) {
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
                else if (prop == "ATTACK") {
                    if (key.find("ATTACK_DAMAGE") != string::npos) tmpHeroes[id].attack_damage = val;
                } else if (prop == "ATTACK_DAMAGE") tmpHeroes[id].attack_damage = val;
                else if (prop == "ATTACK_RANGE") tmpHeroes[id].attack_range = val;
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
                    istringstream iss(rest);
                    iss >> val;
                } else if (parts.size() >= 2) {
                    val = stoi(parts[1]);
                }
                if (prop == "HP") tmpMonsters[id].hp = val;
                else if (prop == "ATTACK") {
                    if (s.find("ATTACK_DAMAGE") != string::npos) tmpMonsters[id].attack_damage = val;
                } else if (prop == "ATTACK_DAMAGE") tmpMonsters[id].attack_damage = val;
                else if (prop == "VISION") {
                    if (s.find("VISION_RANGE") != string::npos) tmpMonsters[id].vision_range = val;
                } else if (prop == "VISION_RANGE") tmpMonsters[id].vision_range = val;
                else if (prop == "ATTACK_RANGE") tmpMonsters[id].attack_range = val;
            }
            continue;
        }

        if (parts[0] == "MONSTER_COUNT") {
            continue;
        }
    } 
    heroes.clear();
    monstruos.clear();
    if (!tmpHeroes.empty()) {
        vector<int> keys;
        for (auto &p : tmpHeroes) keys.push_back(p.first);
        sort(keys.begin(), keys.end());
        for (int k : keys) {
            Heroe h = tmpHeroes[k];
            if (h.attack_range==0) h.attack_range = 1;
            if (h.attack_damage==0) h.attack_damage = 10;
            if (h.hp==0) h.hp = 100;
            heroes.push_back(h);
        }
    }

    if (!tmpMonsters.empty()) {
        vector<int> keys;
        for (auto &p : tmpMonsters) keys.push_back(p.first);
        sort(keys.begin(), keys.end());
        for (int k : keys) {
            Monstruo m = tmpMonsters[k];
            if (m.attack_range==0) m.attack_range = 1;
            if (m.attack_damage==0) m.attack_damage = 5;
            if (m.hp==0) m.hp = 50;
            if (m.vision_range==0) m.vision_range = 5;
            monstruos.push_back(m);
        }
    }
    if (heroes.empty()) {
        cerr << "no hay heroes"<<endl;
    }
    if (monstruos.empty()) {
        cerr << "no hay monstruos"<<endl;
    }

    return true;
}
int main(int argc, char** argv) {
    if (argc < 2) {
        cout << "Uso: " << argv[0] << " <archivo_config.txt>"<<endl;//carga dek archivo
        return 1;
    }
    string filename = argv[1];
    if (!cargarDesdeArchivo(filename)) {
        cerr << "Error cargando archivo. Abortando.\n";
        return 1;
    }
    mapa.assign(ROWS, vector<int>(COLS, 0));//inicia mapa en 0 ya que no se han definido monstruos ni heroes
    for (auto &h : heroes) {
        if (h.pos.x >= 0 && h.pos.x < ROWS && h.pos.y >= 0 && h.pos.y < COLS)//dentro de limites
            mapa[h.pos.x][h.pos.y] = 1;//se colocan heroes en mapa
    }
    for (auto &m : monstruos) {
        if (m.pos.x >= 0 && m.pos.x < ROWS && m.pos.y >= 0 && m.pos.y < COLS) {//dentro de limites
            if (mapa[m.pos.x][m.pos.y] == 0) mapa[m.pos.x][m.pos.y] = 2;//se colocan monstruos en el mapa
        }
    }
    int H = (int)heroes.size();
    int M = (int)monstruos.size();
    sem_heroes.assign(H, {});//inicio semaforo heroe
    sem_monstruos.assign(M, {});//inicio semaforo monstruo
    for (int i = 0; i < H; ++i) sem_init(&sem_heroes[i], 0, 0);
    for (int i = 0; i < M; ++i) sem_init(&sem_monstruos[i], 0, 0);

    sem_init(&sem_heroes_done, 0, 0);
    sem_init(&sem_monsters_done, 0, 0);

    // Crear hilos
    vector<pthread_t> th_heroes(H);//thread de heroe
    vector<pthread_t> th_monsters(M);//thread de monstrio
    vector<int> idsH(H), idsM(M);//thread de mapa
    for (int i = 0; i < H; ++i) idsH[i] = i;
    for (int i = 0; i < M; ++i) idsM[i] = i;

    for (int i = 0; i < H; ++i)
        pthread_create(&th_heroes[i], nullptr, hiloHeroe, &idsH[i]);
    for (int i = 0; i < M; ++i)
        pthread_create(&th_monsters[i], nullptr, hiloMonstruo, &idsM[i]);
    while (juego_activo) {
        imprimirMapaEnArchivoYConsola();

        cout << "Presiona 'n' + Enter para siguiente turno (q para salir): ";
        string cmd;
        if (!(cin >> cmd)) break;
        if (cmd == "q") { juego_activo = false; break; }
        if (cmd != "n") continue;

        for (int i = 0; i < H; ++i) {//dar turno a heroes
            if (heroes[i].alive && !heroes[i].reached_goal)//verifica si esta vivo y si no llego a meta
                sem_post(&sem_heroes[i]);
            else
                sem_post(&sem_heroes_done);//se espera semaforo si no se ocupa
        }
        for (int i = 0; i < H; ++i) sem_wait(&sem_heroes_done);//espera señal de termino de cada hereo
        imprimirMapaEnArchivoYConsola();
        for (int i = 0; i < M; ++i) {//dar turno a monstruos
            if (monstruos[i].alive)
                sem_post(&sem_monstruos[i]);
            else
                sem_post(&sem_monsters_done);//si no tienen turno se cierra semaforo
        }
        for (int i = 0; i < M; ++i) sem_wait(&sem_monsters_done);//espera cada termino de monstruo
        imprimirMapaEnArchivoYConsola();
        bool anyHeroAlive = false;
        bool allReached = true;
        for (auto &h : heroes) {
            if (h.alive) anyHeroAlive = true;//verifica si todos estan vivos
            if (!h.reached_goal) allReached = false;//verifica si todo llego ameta
        }
        if (!anyHeroAlive) {
            cout << "todos murieron puros mancos"<<endl ;
            juego_activo = false;
            break;
        }
        if (allReached) {
            cout << "todos llegaron unas maquinas";
            juego_activo = false;
            break;
        }

        // small sleep to avoid too fast loop (opcional)
        usleep(100000);
    }
    for (int i = 0; i < H; ++i) sem_post(&sem_heroes[i]);//se espera semaforos de heroes
    for (int i = 0; i < M; ++i) sem_post(&sem_monstruos[i]);//se espera semaforo de monstruos
    for (int i = 0; i < H; ++i) pthread_join(th_heroes[i], nullptr);//se espera threas de heroes
    for (int i = 0; i < M; ++i) pthread_join(th_monsters[i], nullptr);//se espera threas de monstruos
    imprimirMapaEnArchivoYConsola();
    for (int i = 0; i < H; ++i) sem_destroy(&sem_heroes[i]);//se destruyen semafors de heroes
    for (int i = 0; i < M; ++i) sem_destroy(&sem_monstruos[i]);//se destruyen semafors de mosntruos
    sem_destroy(&sem_heroes_done);//se cierran semaforors auciliares de heroes
    sem_destroy(&sem_monsters_done);//se cierran semaforos auxiliares de monstruos
    pthread_mutex_destroy(&mtx);//se destruye el tread del mapa

    cout << "fin";
    return 0;
}
