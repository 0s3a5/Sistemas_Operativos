// juego.cpp
// Compilar: g++ -std=c++17 -pthread juego.cpp -o juego
// Ejecutar: ./juego ejemplo2.txt
#include <bits/stdc++.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
using namespace std;

// -----------------------------
// Estructuras
// -----------------------------
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

// -----------------------------
// Variables globales (dinámicas)
// -----------------------------
int ROWS = 20, COLS = 20;
vector<vector<int>> mapa; // 0 empty, 1 hero, 2 monster (if overlap choose priority printing)
vector<Heroe> heroes;
vector<Monstruo> monstruos;

pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;

// Semáforos y contadores para coordinación de rondas
vector<sem_t> sem_heroes;
vector<sem_t> sem_monstruos;
sem_t sem_heroes_done;    // contarán cuántos héroes terminaron
sem_t sem_monsters_done;  // contarán cuántos monstruos terminaron

bool juego_activo = true;

// Archivo de salida (append)
string salida_filename = "sim_output.txt";

// -----------------------------
// Utilidades
// -----------------------------
inline int distanciaManhattan(const Pos &a, const Pos &b) {
    return abs(a.x - b.x) + abs(a.y - b.y);
}

void imprimirMapaEnArchivoYConsola() {
    pthread_mutex_lock(&mtx);
    // Construir snapshot en string
    ostringstream ss;
    ss << "Mapa (" << ROWS << "x" << COLS << ")\n";
    for (int i = 0; i < ROWS; ++i) {
        for (int j = 0; j < COLS; ++j) {
            if (mapa[i][j] == 0) ss << ".";
            else if (mapa[i][j] == 1) ss << "H";
            else if (mapa[i][j] == 2) ss << "M";
            else ss << "?";
        }
        ss << "\n";
    }
    ss << "\n";
    // Stats
    for (auto &h : heroes) {
        ss << "Hero" << h.id << " HP=" << h.hp << " Pos=(" << h.pos.x << "," << h.pos.y << ")"
           << (h.alive ? "" : " DEAD") << (h.reached_goal ? " GOAL" : "") << "\n";
    }
    for (auto &m : monstruos) {
        ss << "Mon" << m.id << " HP=" << m.hp << " Pos=(" << m.pos.x << "," << m.pos.y << ")"
           << (m.alive ? (m.active ? " ACTIVE" : " PASSIVE") : " DEAD") << "\n";
    }
    ss << "-----------------------------\n";

    string snap = ss.str();
    // Console
    cout << snap;
    // Append to file
    ofstream ofs(salida_filename, ios::app);
    if (ofs) {
        ofs << snap;
        ofs.close();
    } else {
        cerr << "No se pudo abrir " << salida_filename << " para escribir.\n";
    }
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
        // limpiar anterior
        if (h.pos.x >= 0 && h.pos.x < ROWS && h.pos.y >= 0 && h.pos.y < COLS) {
            // sólo limpiar si aún hay héroe en esa casilla (puede que múltiples entidades colisionen)
            if (mapa[h.pos.x][h.pos.y] == 1) mapa[h.pos.x][h.pos.y] = 0;
        }
        h.pos = h.ruta[h.path_idx];
        if (h.pos.x >= 0 && h.pos.x < ROWS && h.pos.y >= 0 && h.pos.y < COLS)
            mapa[h.pos.x][h.pos.y] = 1;
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
            cout << "🗡️  Hero" << h.id << " ataca a Mon" << m.id
                 << " (-" << h.attack_damage << " HP). Queda HP=" << max(0, m.hp) << "\n";
            if (m.hp <= 0) {
                m.alive = false;
                if (m.pos.x >= 0 && m.pos.x < ROWS && m.pos.y >=0 && m.pos.y < COLS)
                    mapa[m.pos.x][m.pos.y] = 0;
                cout << "💀 Mon" << m.id << " ha muerto.\n";
            }
            ataco = true;
            break; // sólo 1 ataque por héroe por turno
        }
    }
    if (!ataco) {
        cout << "⚔️  Hero" << h.id << " no encontró monstruos en su rango.\n";
    }
    pthread_mutex_unlock(&mtx);
}

void comprobarVisionYAccionarMonstruo(int midx) {
    Monstruo &m = monstruos[midx];
    // comprobar visión (sin lock pesado)
    int dist;
    {
        pthread_mutex_lock(&mtx);
        dist = distanciaManhattan(m.pos, /* héroes: buscar el más cercano */ heroes.size() ? heroes[0].pos : Pos{0,0});
        // en vez de fijarnos solo en hero[0], determinemos la distancia mínima a cualquier héroe
        int minD = INT_MAX;
        for (auto &h : heroes) {
            if (!h.alive) continue;
            minD = min(minD, distanciaManhattan(m.pos, h.pos));
        }
        dist = (minD==INT_MAX ? INT_MAX : minD);
        pthread_mutex_unlock(&mtx);
    }

    if (dist <= m.vision_range) {
        pthread_mutex_lock(&mtx);
        if (!m.active) {
            m.active = true;
            cout << "👁️  Mon" << m.id << " detectó a un héroe (dist=" << dist << ", vision=" << m.vision_range << ").\n";
        }
        pthread_mutex_unlock(&mtx);
    } else {
        pthread_mutex_lock(&mtx);
        m.active = false;
        pthread_mutex_unlock(&mtx);
        return;
    }

    // Buscar el héroe vivo más cercano
    int best = -1;
    int bestd = INT_MAX;
    pthread_mutex_lock(&mtx);
    for (size_t i = 0; i < heroes.size(); ++i) {
        if (!heroes[i].alive) continue;
        int d = distanciaManhattan(m.pos, heroes[i].pos);
        if (d < bestd) { bestd = d; best = (int)i; }
    }
    if (best == -1 || !juego_activo) { pthread_mutex_unlock(&mtx); return; }
    Heroe &target = heroes[best];

    // si en rango de ataque, atacar
    if (bestd <= m.attack_range) {
        target.hp -= m.attack_damage;
        cout << "🔥 Mon" << m.id << " ataca a Hero" << target.id << " (-" << m.attack_damage << " HP). Hero HP=" << max(0, target.hp) << "\n";
        if (target.hp <= 0) {
            target.alive = false;
            // limpiar mapa si aún estaba
            if (target.pos.x >= 0 && target.pos.x < ROWS && target.pos.y >=0 && target.pos.y < COLS)
                mapa[target.pos.x][target.pos.y] = 0;
            cout << "💀 Hero" << target.id << " ha muerto.\n";
            // si todos los heroes murieron, final
            bool anyAlive = false;
            for (auto &h : heroes) if (h.alive) { anyAlive = true; break; }
            if (!anyAlive) {
                juego_activo = false;
                cout << "💀 Todos los héroes han muerto. Fin del juego.\n";
            }
        }
        pthread_mutex_unlock(&mtx);
        return;
    }

    // si no puede atacar, moverse una casilla que reduzca la distancia hacia el target
    if (m.pos.x >= 0 && m.pos.x < ROWS && m.pos.y >= 0 && m.pos.y < COLS) {
        pthread_mutex_lock(&mtx);
        if (mapa[m.pos.x][m.pos.y] == 2) mapa[m.pos.x][m.pos.y] = 0;
        pthread_mutex_unlock(&mtx);
    }
    int dx = target.pos.x - m.pos.x;
    int dy = target.pos.y - m.pos.y;
    if (abs(dx) > abs(dy)) m.pos.x += (dx > 0 ? 1 : -1);
    else if (dy != 0) m.pos.y += (dy > 0 ? 1 : -1);

    pthread_mutex_lock(&mtx);
    if (m.pos.x >= 0 && m.pos.x < ROWS && m.pos.y >= 0 && m.pos.y < COLS) {
        // si había un héroe en la casilla, no sobreescribir (priorizamos héroe en el mapa)
        if (mapa[m.pos.x][m.pos.y] == 0) mapa[m.pos.x][m.pos.y] = 2;
    }
    cout << "[Mon" << m.id << "] se mueve a (" << m.pos.x << "," << m.pos.y << ")\n";
    pthread_mutex_unlock(&mtx);
}

// -----------------------------
// Hilos
// -----------------------------
void* hiloHeroe(void* arg) {
    int idx = *((int*)arg);
    while (juego_activo && heroes[idx].alive && !heroes[idx].reached_goal) {
        // esperar señal del main para ejecutar turno
        sem_wait(&sem_heroes[idx]);
        if (!juego_activo) break;

        // ejecutar movimiento y ataque
        moverHeroeIndice(idx);
        ataqueHeroeIndice(idx);

        // indicar que este héroe terminó su acción
        sem_post(&sem_heroes_done);
    }
    // si sale, indicar terminado (por si main espera)
    sem_post(&sem_heroes_done);
    return nullptr;
}

void* hiloMonstruo(void* arg) {
    int idx = *((int*)arg);
    while (juego_activo && monstruos[idx].alive) {
        sem_wait(&sem_monstruos[idx]);
        if (!juego_activo) break;

        // comprobar visión, moverse y atacar/accionar
        comprobarVisionYAccionarMonstruo(idx);

        // indicar que este monstruo terminó su acción
        sem_post(&sem_monsters_done);
    }
    // si sale, indicar terminado (por si main espera)
    sem_post(&sem_monsters_done);
    return nullptr;
}

// -----------------------------
// Lectura del archivo de configuración
// -----------------------------
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

// parse coordenadas formato: "(x,y)" o "x y"
vector<Pos> parsePathLines(const vector<string>& lines) {
    vector<Pos> out;
    // extraer todos los números presentes
    for (auto &ln : lines) {
        string t = ln;
        // reemplazar paréntesis y comas por espacios
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

    // Temporal storage for hero properties before pushback
    map<int, Heroe> tmpHeroes;
    map<int, Monstruo> tmpMonsters;

    while (getline(ifs, line)) {
        string s = trim(line);
        if (s.empty()) continue;
        if (s[0] == '#') continue;

        // Key and rest
        vector<string> parts = splitWords(s);
        if (parts.empty()) continue;

        // GRID_SIZE
        if (parts[0] == "GRID_SIZE" && parts.size() >= 3) {
            ROWS = stoi(parts[1]);
            COLS = stoi(parts[2]);
            continue;
        }

        // HERO_N properties
        if (startsWith(parts[0], "HERO_")) {
            // ejemplo: HERO_1_HP or HERO_1_START or HERO_1_PATH
            string key = parts[0]; // e.g., HERO_1_HP
            // extract id between HERO_ and _
            size_t p1 = key.find('_',5); // second underscore position
            // alternative parse: find second underscore
            // simpler: find third token after splitting by _
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
                // PATH can be multi-line: collect current line remainder and then subsequent lines starting with '('
                // collect the substring after "HERO_n_PATH"
                string rest = s.substr(s.find("PATH") + 4);
                bufferPathLines.clear();
                if (trim(rest).size() > 0) bufferPathLines.push_back(rest);
                // read ahead lines that start with '(' or contain '('
                streampos lastPos = ifs.tellg();
                string next;
                while (getline(ifs, next)) {
                    string tn = trim(next);
                    if (tn.empty()) { lastPos = ifs.tellg(); continue; }
                    // if next line starts with '(' or contains '(' treat as path continuation
                    if (tn.find('(') != string::npos) {
                        bufferPathLines.push_back(tn);
                        lastPos = ifs.tellg();
                    } else {
                        // rollback one line (we read too far)
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
                    int x = stoi(parts[1+1]); // parts[1] normally is like "HERO_1_HP"? but safer: parse full line
                }
                // parse numbers from the whole line (after the key)
                string rest = s.substr(s.find("START") + 5);
                istringstream iss(rest);
                int x,y; if (iss >> x >> y) {
                    tmpHeroes[id].pos = Pos{x,y};
                    tmpHeroes[id].id = id;
                }
                continue;
            } else {
                // numeric properties like HP, ATTACK_DAMAGE, ATTACK_RANGE
                string rest = s.substr(s.find(prop) + prop.size());
                istringstream iss(rest);
                int val;
                if (!(iss >> val)) {
                    // maybe value is the next token
                    if (parts.size() >= 2) val = stoi(parts[1]);
                    else continue;
                }
                if (prop == "HP") tmpHeroes[id].hp = val;
                else if (prop == "ATTACK") {
                    // maybe ATTACK_DAMAGE (two tokens)
                    if (key.find("ATTACK_DAMAGE") != string::npos) tmpHeroes[id].attack_damage = val;
                } else if (prop == "ATTACK_DAMAGE") tmpHeroes[id].attack_damage = val;
                else if (prop == "ATTACK_RANGE") tmpHeroes[id].attack_range = val;
                continue;
            }
        }

        // MONSTER properties
        if (startsWith(parts[0], "MONSTER_")) {
            // MONSTER_1_HP etc
            vector<string> toks;
            {
                istringstream is(parts[0]);
                string t;
                while (getline(is, t, '_')) toks.push_back(t);
            }
            if (toks.size() < 3) continue;
            int id = stoi(toks[1]);
            string prop = toks[2];
            // read numeric from rest
            string rest;
            if (s.size() > parts[0].size()) rest = trim(s.substr(parts[0].size()));
            else rest = "";
            // handle keys like MONSTER_1_COORDS which have two numbers
            if (prop == "COORDS") {
                istringstream iss(rest);
                int x,y; if (iss >> x >> y) {
                    tmpMonsters[id].pos = Pos{x,y};
                    tmpMonsters[id].id = id;
                }
            } else {
                // parse single integer value
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

        // MONSTER_COUNT
        if (parts[0] == "MONSTER_COUNT") {
            // we don't need it strictly, we'll use parsed monsters
            continue;
        }
    } // end file read

    // Now, move tmpHeroes/tmpMonsters maps to vectors (in ID order)
    heroes.clear();
    monstruos.clear();

    // Heroes: find max id
    if (!tmpHeroes.empty()) {
        vector<int> keys;
        for (auto &p : tmpHeroes) keys.push_back(p.first);
        sort(keys.begin(), keys.end());
        for (int k : keys) {
            Heroe h = tmpHeroes[k];
            // defaults
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

    // If no heroes/monsters found, return false
    if (heroes.empty()) {
        cerr << "No se encontraron héroes en el archivo.\n";
        // but we can still continue if monsters exist (test)
    }
    if (monstruos.empty()) {
        cerr << "No se encontraron monstruos en el archivo.\n";
    }

    return true;
}

// -----------------------------
// MAIN
// -----------------------------
int main(int argc, char** argv) {
    if (argc < 2) {
        cout << "Uso: " << argv[0] << " <archivo_config.txt>\n";
        return 1;
    }
    string filename = argv[1];

    // limpiar archivo de salida
    ofstream ofs(salida_filename, ios::trunc);
    if (!ofs) cerr << "No se pudo crear archivo de salida " << salida_filename << "\n";
    else ofs.close();

    if (!cargarDesdeArchivo(filename)) {
        cerr << "Error cargando archivo. Abortando.\n";
        return 1;
    }

    // Inicializar mapa con tamaño dado
    mapa.assign(ROWS, vector<int>(COLS, 0));

    // Colocar héroes y monstruos en mapa
    for (auto &h : heroes) {
        if (h.pos.x >= 0 && h.pos.x < ROWS && h.pos.y >= 0 && h.pos.y < COLS)
            mapa[h.pos.x][h.pos.y] = 1;
    }
    for (auto &m : monstruos) {
        if (m.pos.x >= 0 && m.pos.x < ROWS && m.pos.y >= 0 && m.pos.y < COLS) {
            // don't overwrite hero
            if (mapa[m.pos.x][m.pos.y] == 0) mapa[m.pos.x][m.pos.y] = 2;
        }
    }

    // Inicializar semáforos (por héroe y por monstruo)
    int H = (int)heroes.size();
    int M = (int)monstruos.size();

    sem_heroes.assign(H, {});
    sem_monstruos.assign(M, {});
    for (int i = 0; i < H; ++i) sem_init(&sem_heroes[i], 0, 0);
    for (int i = 0; i < M; ++i) sem_init(&sem_monstruos[i], 0, 0);

    sem_init(&sem_heroes_done, 0, 0);
    sem_init(&sem_monsters_done, 0, 0);

    // Crear hilos
    vector<pthread_t> th_heroes(H);
    vector<pthread_t> th_monsters(M);
    vector<int> idsH(H), idsM(M);
    for (int i = 0; i < H; ++i) idsH[i] = i;
    for (int i = 0; i < M; ++i) idsM[i] = i;

    for (int i = 0; i < H; ++i)
        pthread_create(&th_heroes[i], nullptr, hiloHeroe, &idsH[i]);
    for (int i = 0; i < M; ++i)
        pthread_create(&th_monsters[i], nullptr, hiloMonstruo, &idsM[i]);

    // Bucle principal por turnos: -> leer 'n' para avanzar
    while (juego_activo) {
        // Mostrar mapa y pedir input
        imprimirMapaEnArchivoYConsola();

        cout << "Presiona 'n' + Enter para siguiente turno (q para salir): ";
        string cmd;
        if (!(cin >> cmd)) break;
        if (cmd == "q") { juego_activo = false; break; }
        if (cmd != "n") continue;

        // 1) DAR TURNO A TODOS LOS HEROES (paralelo)
        for (int i = 0; i < H; ++i) {
            if (heroes[i].alive && !heroes[i].reached_goal)
                sem_post(&sem_heroes[i]);
            else
                // si no se ejecutará, igualmente marcar como "terminado" para que main no espere indefinidamente
                sem_post(&sem_heroes_done);
        }

        // esperar hasta recibir H posts de sem_heroes_done (uno por héroe)
        for (int i = 0; i < H; ++i) sem_wait(&sem_heroes_done);

        // 2) Después de héroes, imprimir el mapa parcial y guardarlo
        imprimirMapaEnArchivoYConsola();

        // 3) DAR TURNO A TODOS LOS MONSTRUOS (paralelo)
        for (int i = 0; i < M; ++i) {
            if (monstruos[i].alive)
                sem_post(&sem_monstruos[i]);
            else
                sem_post(&sem_monsters_done);
        }

        // esperar hasta recibir M posts de sem_monsters_done (uno por monstruo)
        for (int i = 0; i < M; ++i) sem_wait(&sem_monsters_done);

        // 4) imprimir mapa final del turno
        imprimirMapaEnArchivoYConsola();

        // comprobar condiciones de finalización: todos los héroes muertos o todos alcanzaron meta
        bool anyHeroAlive = false;
        bool allReached = true;
        for (auto &h : heroes) {
            if (h.alive) anyHeroAlive = true;
            if (!h.reached_goal) allReached = false;
        }
        if (!anyHeroAlive) {
            cout << "💀 Todos los héroes han muerto. Fin del juego.\n";
            juego_activo = false;
            break;
        }
        if (allReached) {
            cout << "🏆 Todos los héroes alcanzaron sus metas. Fin del juego.\n";
            juego_activo = false;
            break;
        }

        // small sleep to avoid too fast loop (opcional)
        usleep(100000);
    }

    // Terminar: asegurar que todos los hilos despierten y terminen
    for (int i = 0; i < H; ++i) sem_post(&sem_heroes[i]);
    for (int i = 0; i < M; ++i) sem_post(&sem_monstruos[i]);

    for (int i = 0; i < H; ++i) pthread_join(th_heroes[i], nullptr);
    for (int i = 0; i < M; ++i) pthread_join(th_monsters[i], nullptr);

    // imprimir estado final
    imprimirMapaEnArchivoYConsola();

    // destruir semáforos
    for (int i = 0; i < H; ++i) sem_destroy(&sem_heroes[i]);
    for (int i = 0; i < M; ++i) sem_destroy(&sem_monstruos[i]);
    sem_destroy(&sem_heroes_done);
    sem_destroy(&sem_monsters_done);
    pthread_mutex_destroy(&mtx);

    cout << "Simulación finalizada. Salida guardada en: " << salida_filename << "\n";
    return 0;
}
