
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
int mapa [30][20]={0};

pthread_mutex_t mutex_mapa;

void imprimir_mapa() {
    pthread_mutex_lock(&mutex_mapa);
    for (int i = 0; i < largo; i++) {
        for (int j = 0; j < ancho; j++) {
            if(mapa[i][j] == 0) {
                cout << ".";
            }
            else if(mapa[i][j] == 1) {
                cout << "H";
            }
            else if(mapa[i][j] == 2) {
                cout << "M";
            }
        }
        cout << endl;
    }
    pthread_mutex_unlock(&mutex_mapa);
}

void moverHeroe(Heroe &heroe, Pos ruta[], int cont) {
    //g¿hacer que cont sea el numero de movimiento
    //si el n de movimineto es lo mismo que largo de ruta llego
        sleep(1); 
        pthread_mutex_lock(&mutex_mapa);
        mapa[heroe.pos.x][heroe.pos.y] = 0;
        heroe.pos.x = ruta[cont].x;
        heroe.pos.y = ruta[cont].y;
        mapa[heroe.pos.x][heroe.pos.y] = 1;
        imprimir_mapa();
        pthread_mutex_unlock(&mutex_mapa);
}
void activar_monstruo(Monstruo &monstruo) {
    pthread_mutex_lock(&mutex_mapa);
    monstruo.active = true;
    cout << "Monstruo " << monstruo.id << " activado!" << endl;
    pthread_mutex_unlock(&mutex_mapa);
}
void vigilar(Monstruo &monstruo) {
    while (true) {
        sleep(1); 
        pthread_mutex_lock(&mutex_mapa);
        for(int i = -monstruo.vision_range; i <= monstruo.vision_range; i++) {
            for(int j = -monstruo.vision_range; j <= monstruo.vision_range; j++) {
                int x = monstruo.pos.x + i;
                int y = monstruo.pos.y + j;
                if(x >= 0 && x < largo && y >= 0 && y < ancho) {
                    if(mapa[x][y] == 1) { 
                        activar_monstruo(monstruo);
                    }
                }
            }
        }
        
        pthread_mutex_unlock(&mutex_mapa);
    }
}

int main(){
    pthread_mutex_init(&mutex_mapa, NULL);
  
    Heroe heroe1 = {1, 100, 20, 1, {0, 0}, 0, 0, true, false};
    Monstruo monstruo1 = {1, 100, 15, 5, 1, {10, 10}, false, true};

    mapa[heroe1.pos.x][heroe1.pos.y] = 1;
    mapa[monstruo1.pos.x][monstruo1.pos.y] = 2;

    thread vigilante(vigilar, ref(monstruo1));

    Pos ruta_heroe[] = {{0,0}, {1,0}, {2,0}, {3,0}, {4,0}, {5,0}, {6,0}, {7,0}, {8,0}, {9,0}, {10,0}, {10,1}, {10,2}, {10,3}, {10,4}, {10,5}, {10,6}, {10,7}, {10,8}, {10,9}, {10,10}};
     heroe1.path_len = sizeof(ruta_heroe) / sizeof(ruta_heroe[0]);

    // Movimiento del héroe
    while (heroe1.alive && !heroe1.reached_goal) {
        moverHeroe(heroe1, ruta_heroe, heroe1.path_idx);

        if (heroe1.pos.x == 10 && heroe1.pos.y == 10) {
            heroe1.reached_goal = true;
            cout << "El héroe ha llegado a su objetivo!" << endl;
        }
    }

    // Espera a que el hilo del monstruo termine
    

    vigilante.join();
    pthread_mutex_destroy(&mutex_mapa);
    return 0;

}

