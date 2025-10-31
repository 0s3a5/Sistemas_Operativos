#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <cmath>
#include <algorithm>
#include <memory>
#include <map>
using namespace std;

mutex cout_mutex;

struct Coordenadas 
{
    int x, y;
};

struct Personaje 
{
    int id;
    int vida;
    int ATTACK_DAMAGE;
    int ATTACK_RANGE;
    Coordenadas posicion;
    bool esta_vivo = true;
    mutex personaje_mutex;
};

struct Heroe : Personaje 
{
    vector<Coordenadas> ruta;
    size_t indice_ruta = 0;
    bool en_combate = false;
    bool ruta_completada = false;
};

struct Monstruo : Personaje 
{
    int VISION_RANGE;
    bool esta_alertado = false;
};

struct EstadoJuego 
{
    Coordenadas tamano_grid;
    vector<unique_ptr<Heroe>> heroes;
    vector<unique_ptr<Monstruo>> monstruos;

    mutex juego_mutex;
    condition_variable alerta_monstruos_cv;
    bool juego_terminado = false;
};

int distancia_manhattan(Coordenadas a, Coordenadas b) 
{
    return abs(a.x - b.x) + abs(a.y - b.y);
}

void imprimir_seguro(const string& mensaje) 
{
    lock_guard<mutex> lock(cout_mutex);
    cout << mensaje << endl;
}

void logica_monstruo(EstadoJuego& estado_juego, int id_interno) 
{
    Monstruo& yo = *estado_juego.monstruos[id_interno];
    imprimir_seguro("Monstruo " + to_string(yo.id) + " aparece en (" + to_string(yo.posicion.x) + ", " + to_string(yo.posicion.y) + ").");

    {
        unique_lock<mutex> lock(estado_juego.juego_mutex);
        estado_juego.alerta_monstruos_cv.wait(lock, [&] { return yo.esta_alertado || estado_juego.juego_terminado; });
    }

    if(!estado_juego.juego_terminado && yo.esta_vivo) 
    {
        imprimir_seguro("Monstruo " + to_string(yo.id) + " ha sido alertado!");
    }

    while (yo.esta_vivo && !estado_juego.juego_terminado) 
    {
        int id_heroe_objetivo_interno = -1;
        int dist_minima = -1;

        {
            lock_guard<mutex> lock(estado_juego.juego_mutex);
            for (size_t i = 0; i < estado_juego.heroes.size(); ++i) 
            {
                if (estado_juego.heroes[i] && estado_juego.heroes[i]->esta_vivo) 
                {
                    int dist = distancia_manhattan(yo.posicion, estado_juego.heroes[i]->posicion);
                    if (id_heroe_objetivo_interno == -1 || dist < dist_minima) 
                    {
                        dist_minima = dist;
                        id_heroe_objetivo_interno = i;
                    }
                }
            }
        }
        
        if (id_heroe_objetivo_interno == -1) 
        {
            this_thread::sleep_for(chrono::milliseconds(1000));
            continue;
        }

        Heroe& objetivo = *estado_juego.heroes[id_heroe_objetivo_interno];

        if (dist_minima > yo.ATTACK_RANGE) 
            {
            lock_guard<mutex> lock(yo.personaje_mutex);
            Coordenadas& pos_objetivo = objetivo.posicion;
            if (yo.posicion.x < pos_objetivo.x) yo.posicion.x++;
            else if (yo.posicion.x > pos_objetivo.x) yo.posicion.x--;
            else if (yo.posicion.y < pos_objetivo.y) yo.posicion.y++;
            else if (yo.posicion.y > pos_objetivo.y) yo.posicion.y--;
            imprimir_seguro("Monstruo " + to_string(yo.id) + " se mueve a (" + to_string(yo.posicion.x) + ", " + to_string(yo.posicion.y) + ").");
            } 
        else 
        {
            lock_guard<mutex> lock(objetivo.personaje_mutex);
            if(objetivo.esta_vivo) 
            {
                objetivo.vida -= yo.ATTACK_DAMAGE;
                imprimir_seguro("Monstruo " + to_string(yo.id) + " ataca a Heroe " + to_string(objetivo.id) + ". Vida restante del heroe: " + to_string(objetivo.vida));
                if (objetivo.vida <= 0) 
                {
                    objetivo.esta_vivo = false;
                    imprimir_seguro("Heroe " + to_string(objetivo.id) + " ha muerto!");
                }
            }
        }
        this_thread::sleep_for(chrono::milliseconds(1000));
    }
    imprimir_seguro("Monstruo " + to_string(yo.id) + " ha terminado su ejecucion.");
}

void logica_heroe(EstadoJuego& estado_juego, int id_interno) 
{
    Heroe& yo = *estado_juego.heroes[id_interno];
    imprimir_seguro("Heroe " + to_string(yo.id) + " inicia en (" + to_string(yo.posicion.x) + ", " + to_string(yo.posicion.y) + ").");

    while (yo.esta_vivo && !yo.ruta_completada && !estado_juego.juego_terminado) 
    {
        if(yo.indice_ruta >= yo.ruta.size() && !yo.ruta.empty()) 
        {
            yo.ruta_completada = true;
            imprimir_seguro("Heroe " + to_string(yo.id) + " ha llegado a su destino!");
            continue;
        }

        vector<int> objetivos_internos;
        {
            lock_guard<mutex> lock(estado_juego.juego_mutex);
            for (size_t i = 0; i < estado_juego.monstruos.size(); ++i) 
            {
                if (estado_juego.monstruos[i]->esta_vivo && distancia_manhattan(yo.posicion, estado_juego.monstruos[i]->posicion) <= yo.ATTACK_RANGE) 
                {
                    objetivos_internos.push_back(i);
                }
            }
        }

        yo.en_combate = !objetivos_internos.empty();

        if (yo.en_combate) 
        {
            int id_monstruo_interno = objetivos_internos[0]; 
            Monstruo& objetivo = *estado_juego.monstruos[id_monstruo_interno];
            
            lock_guard<mutex> lock(objetivo.personaje_mutex);
            if(objetivo.esta_vivo) 
            {
                objetivo.vida -= yo.ATTACK_DAMAGE;
                imprimir_seguro("Heroe " + to_string(yo.id) + " ataca a Monstruo " + to_string(objetivo.id) + ". Vida restante del monstruo: " + to_string(objetivo.vida));
                if (objetivo.vida <= 0) 
                {
                    objetivo.esta_vivo = false;
                    imprimir_seguro("Monstruo " + to_string(objetivo.id) + " ha muerto!");
                }
            }
        } 
            else 
            {
            lock_guard<mutex> lock(yo.personaje_mutex);
            yo.posicion = yo.ruta[yo.indice_ruta];
            yo.indice_ruta++;
            imprimir_seguro("Heroe " + to_string(yo.id) + " se mueve a (" + to_string(yo.posicion.x) + ", " + to_string(yo.posicion.y) + ").");
            }

            {
            lock_guard<mutex> lock(estado_juego.juego_mutex);
            bool monstruo_fue_alertado = false;
            for (auto& puntero_monstruo : estado_juego.monstruos) 
            {
                if (puntero_monstruo->esta_vivo && !puntero_monstruo->esta_alertado && distancia_manhattan(yo.posicion, puntero_monstruo->posicion) <= puntero_monstruo->VISION_RANGE)  {
                    imprimir_seguro("Heroe " + to_string(yo.id) + " ha entrado en el rango de vision del Monstruo " + to_string(puntero_monstruo->id));
                    puntero_monstruo->esta_alertado = true;
                    monstruo_fue_alertado = true;
                    for(auto& otro_puntero_monstruo : estado_juego.monstruos) 
                    {
                        if(otro_puntero_monstruo->esta_vivo && !otro_puntero_monstruo->esta_alertado && distancia_manhattan(puntero_monstruo->posicion, otro_puntero_monstruo->posicion) <= puntero_monstruo->VISION_RANGE) 
                        {
                           otro_puntero_monstruo->esta_alertado = true;
                           imprimir_seguro("Monstruo " + to_string(puntero_monstruo->id) + " alerta a Monstruo " + to_string(otro_puntero_monstruo->id));
                        }
                    }
                }
            }
            if(monstruo_fue_alertado) 
            {
                estado_juego.alerta_monstruos_cv.notify_all();
            }
        }
        this_thread::sleep_for(chrono::milliseconds(800));
    }
     imprimir_seguro("Heroe " + to_string(yo.id) + " ha terminado su ejecucion.");
}


int main(int argc, char* argv[]) 
{
    if (argc < 2) 
    {
        cout << "Uso: ./simulacion <archivo_configuracion>" << endl;
        return 1;
    }

    ifstream archivo_config(argv[1]);
    if (!archivo_config.is_open()) 
    {
        cout << "Error: No se pudo abrir el archivo de configuracion." << endl;
        return 1;
    }

    EstadoJuego estado_juego;
    string linea;
    int monster_count_from_file = 0;
    
    map<int, int> mapa_id_heroe_archivo_a_interno;
    int proximo_id_interno_heroe = 0;

    vector<string> lineas_del_archivo;
    string linea_actual;
    while(getline(archivo_config, linea_actual)) 
    {
        stringstream ss_test(linea_actual);
        string primer_palabra_test;
        ss_test >> primer_palabra_test;
        if (primer_palabra_test.empty() || primer_palabra_test[0] == '#') continue;

        if (primer_palabra_test.rfind("HERO_", 0) == 0 && primer_palabra_test.find("PATH") != string::npos) 
        {
            string path_completo = linea_actual;
            streampos pos_actual = archivo_config.tellg();
            string proxima_linea;
            while(getline(archivo_config, proxima_linea)) 
            {
                stringstream ss_prox(proxima_linea);
                string prox_primer_palabra;
                ss_prox >> prox_primer_palabra;
                if (prox_primer_palabra.find('(') == 0) 
                {
                    path_completo += " " + proxima_linea;
                    pos_actual = archivo_config.tellg();
                } else 
                    {
                    archivo_config.seekg(pos_actual);
                    break;
                    }
                }
            lineas_del_archivo.push_back(path_completo);
                } 
            else 
            {
             lineas_del_archivo.push_back(linea_actual);
            }
    }
    archivo_config.close();

    // ==============================================================================
    for(size_t i = 0; i < lineas_del_archivo.size(); ++i) 
    {
        
        stringstream ss(lineas_del_archivo[i]);
        string clave;
        ss >> clave;

        string resto_linea;
        getline(ss, resto_linea);
        
        if (resto_linea.find_first_not_of(" \t\n\r") == string::npos) 
        {
            if (i + 1 < lineas_del_archivo.size()) 
            {
                 stringstream ss_prox_test(lineas_del_archivo[i+1]);
                 string prox_palabra_test;
                 ss_prox_test >> prox_palabra_test;
                 try 
                 {
                    stoi(prox_palabra_test);
                    resto_linea = lineas_del_archivo[++i];
                 } catch(const std::exception& e) {}
            }
        }

        stringstream val_ss(resto_linea);
    
        if (clave == "GRID_SIZE") 
        {
            val_ss >> estado_juego.tamano_grid.x >> estado_juego.tamano_grid.y;
        } 
            else if (clave == "MONSTER_COUNT") 
            {
            val_ss >> monster_count_from_file;
            estado_juego.monstruos.clear(); // errores repetir
                for (int j = 0; j < monster_count_from_file; ++j) 
                {
                    estado_juego.monstruos.push_back(make_unique<Monstruo>());
                    estado_juego.monstruos.back()->id = j + 1;
                }
            } 
            else if (clave.rfind("HERO_", 0) == 0) 
            {
            string subclave;
            int id_archivo_original = 0;
            int indice_interno = 0;

            size_t primer_guion = clave.find('_');
            size_t segundo_guion = clave.find('_', primer_guion + 1);

            if (segundo_guion != string::npos) 
            {
                try 
                {
                    id_archivo_original = stoi(clave.substr(primer_guion + 1, segundo_guion - primer_guion - 1));
                    subclave = clave.substr(segundo_guion + 1);
                } catch(const std::exception& e) { continue; }

                 if (mapa_id_heroe_archivo_a_interno.find(id_archivo_original) == mapa_id_heroe_archivo_a_interno.end()) 
                {
                    mapa_id_heroe_archivo_a_interno[id_archivo_original] = proximo_id_interno_heroe++;
                    estado_juego.heroes.push_back(make_unique<Heroe>());
                }
                indice_interno = mapa_id_heroe_archivo_a_interno[id_archivo_original];

            } else 
                {
                subclave = clave.substr(primer_guion + 1);
                if (estado_juego.heroes.empty()) {
                    estado_juego.heroes.push_back(make_unique<Heroe>());
                }
                indice_interno = 0;
            }

            Heroe& h = *estado_juego.heroes[indice_interno];
            if(h.id == 0) h.id = estado_juego.heroes.size();

            if(subclave == "HP") val_ss >> h.vida;
            else if(subclave == "ATTACK_DAMAGE") val_ss >> h.ATTACK_DAMAGE;
            else if(subclave == "ATTACK_RANGE") val_ss >> h.ATTACK_RANGE;
            else if(subclave == "START") val_ss >> h.posicion.x >> h.posicion.y;
            else if(subclave == "PATH") {
                int x, y; char dummy;
                while(val_ss >> dummy >> x >> dummy >> y >> dummy) h.ruta.push_back({x, y});
            }

        } else if (clave.rfind("MONSTER_", 0) == 0)     
            {
            string id_str, subclave;
            size_t primer_guion = clave.find('_');
            size_t segundo_guion = clave.find('_', primer_guion + 1);
            id_str = clave.substr(primer_guion + 1, segundo_guion - primer_guion - 1);
            subclave = clave.substr(segundo_guion + 1);
            int id = stoi(id_str);

            if (id > 0 && id <= monster_count_from_file) 
            {
                Monstruo& m = *estado_juego.monstruos[id - 1];
                if(subclave == "HP") val_ss >> m.vida;
                else if(subclave == "ATTACK_DAMAGE") val_ss >> m.ATTACK_DAMAGE;
                else if(subclave == "VISION_RANGE") val_ss >> m.VISION_RANGE;
                else if(subclave == "ATTACK_RANGE") val_ss >> m.ATTACK_RANGE;
                else if(subclave == "COORDS") val_ss >> m.posicion.x >> m.posicion.y;
            }
        }
    }



    // la wea pa que jale
    imprimir_seguro("--- Simulacion iniciada con " + to_string(estado_juego.heroes.size()) + " heroe(s) y " + to_string(estado_juego.monstruos.size()) + " monstruo(s) ---");

    vector<thread> hilos;
    for (size_t i = 0; i < estado_juego.heroes.size(); ++i) hilos.emplace_back(logica_heroe, ref(estado_juego), i);
    for (size_t i = 0; i < estado_juego.monstruos.size(); ++i) hilos.emplace_back(logica_monstruo, ref(estado_juego), i);

    while(!estado_juego.juego_terminado) 
    {
        this_thread::sleep_for(chrono::seconds(1));
        lock_guard<mutex> lock(estado_juego.juego_mutex);
        
        bool todos_los_heroes_terminaron = true;
        if(estado_juego.heroes.empty()) 
        {
            if (monster_count_from_file > 0) 
            {
                 imprimir_seguro("No hay heroes para jugar. Fin del juego.");
                 estado_juego.juego_terminado = true;
                 estado_juego.alerta_monstruos_cv.notify_all();
            }
        } 
        else 
        {
             for(const auto& heroe : estado_juego.heroes)   
           {
                if(heroe && heroe->esta_vivo && !heroe->ruta_completada) 
               {
                    todos_los_heroes_terminaron = false;
                    break;
                }
            }
        }

        if(todos_los_heroes_terminaron) 
        {
            imprimir_seguro("Todos los heroes han completado su mision o han sido brutalmente desmembrados.");
            estado_juego.juego_terminado = true;
            estado_juego.alerta_monstruos_cv.notify_all();
        }
    }

    for (auto& h : hilos) 
    {
        h.join();
    }

    imprimir_seguro("--- simulacion terminada ---");

    for(const auto& puntero_heroe : estado_juego.heroes) 
        {
        if(puntero_heroe->esta_vivo && puntero_heroe->ruta_completada) 
        {
             imprimir_seguro("Heroe " + to_string(puntero_heroe->id) + " sobrevivio y completo su ruta.");
        } 
        else if (puntero_heroe->esta_vivo) 
        {
        imprimir_seguro("Heroe " + to_string(puntero_heroe->id) + " sobrevivio pero no completo su ruta.");
        } 
        else 
        {
             imprimir_seguro("Heroe " + to_string(puntero_heroe->id) + " fue derrotado.");
        }
    }
    for(const auto& puntero_monstruo : estado_juego.monstruos) 
    {
        if(!puntero_monstruo->esta_vivo) 
        {
             imprimir_seguro("Monstruo " + to_string(puntero_monstruo->id) + " fue derrotado.");
        }
    }

    return 0;
}
