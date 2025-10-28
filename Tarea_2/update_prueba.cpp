#include <iostream>
#include <cmath>
#include <mutex>

using namespace std;

// =============================
// Constantes
// =============================
const int FILAS = 30;
const int COLUMNAS = 20;

// =============================
// Estructuras
// =============================
struct Pos {
    int x, y;
};

struct Heroe {
    int id, hp, attack_damage, attack_range;
    Pos pos;
    int path_len, path_idx;
    bool alive, reached_goal;
};

struct Monstruo {
    int id, hp, attack_damage, vision_range, attack_range;
    Pos pos;
    bool active, alive;
};

// =============================
// Variables globales
// =============================
int mapa[FILAS][COLUMNAS]; // mapa como arreglo bidimensional
mutex mtx; // para proteger la zona crítica

// =============================
// Funciones auxiliares
// =============================
void imprimirMapa() {
    lock_guard<mutex> lock(mtx); // zona crítica protegida
    for (int i = 0; i < FILAS; i++) {
        for (int j = 0; j < COLUMNAS; j++) {
            if (mapa[i][j] == 0) cout << ".";
            else if (mapa[i][j] == 1) cout << "H";
            else if (mapa[i][j] == 2) cout << "M";
        }
        cout << "\n";
    }
    cout << endl;
}

// =============================
// Función: mover héroe
// =============================
void moverHeroe(Heroe &heroe, Pos ruta[]) {
    lock_guard<mutex> lock(mtx); // bloquear mientras se mueve el héroe
    if (heroe.path_idx < heroe.path_len) {
        mapa[heroe.pos.x][heroe.pos.y] = 0; // limpia posición anterior
        heroe.pos = ruta[heroe.path_idx];
        mapa[heroe.pos.x][heroe.pos.y] = 1; // nueva posición
        heroe.path_idx++;

        if (heroe.path_idx >= heroe.path_len)
            heroe.reached_goal = true;
    }
}

// =============================
// Función: comprobar visión
// =============================
void comprobarVision(Monstruo &monstruo, const Heroe& heroe) {
   lock_guard<mutex> lock(mtx); // zona crítica protegida (mapa)
    int d = abs(monstruo.pos.x - heroe.pos.x) + abs(monstruo.pos.y - heroe.pos.y);
    if (d <= monstruo.vision_range && !monstruo.active) {
        monstruo.active = true;
        cout << "⚠️  Monstruo " << monstruo.id << " ha visto al héroe!" << endl;
    }
}

// =============================
// NUEVA Función: mover monstruo (la tuya)
// =============================
void moverMonstruo(Monstruo &monstruo, const Heroe& heroe) {
    lock_guard<mutex> lock(mtx); // zona crítica protegida (mapa)

    int distanciaX = abs(monstruo.pos.x - heroe.pos.x);
    int distanciaY = abs(monstruo.pos.y - heroe.pos.y);
    int distancia = distanciaX + distanciaY;

    // Si el héroe está fuera del rango de visión
    if (distancia > monstruo.vision_range) {
        monstruo.active = false;
        return;
    }

    monstruo.active = true; // activo mientras lo ve

    // Limpiar posición anterior
    mapa[monstruo.pos.x][monstruo.pos.y] = 0;

    // Calcular diferencias de posición
    int dx = heroe.pos.x - monstruo.pos.x;
    int dy = heroe.pos.y - monstruo.pos.y;

    // Movimiento Manhattan: se mueve en la dirección que reduce la distancia
    if (abs(dx) > abs(dy)) {
        monstruo.pos.x += (dx > 0 ? 1 : -1);
    } else if (dy != 0) {
        monstruo.pos.y += (dy > 0 ? 1 : -1);
    }

    // Actualizar posición en el mapa
    mapa[monstruo.pos.x][monstruo.pos.y] = 2;
}

// =============================
// Función principal
// =============================
int main() {
    // Inicializar mapa
    for (int i = 0; i < FILAS; i++)
        for (int j = 0; j < COLUMNAS; j++)
            mapa[i][j] = 0;

    // Crear héroe y monstruo
    Heroe heroe = {1, 100, 20, 1, {0, 0}, 0, 0, true, false};
    Monstruo monstruo = {1, 100, 15, 5, 1, {10, 10}, false, true};

    // Definir ruta del héroe (arreglo)
    Pos ruta_heroe[] = {
        {0,0}, {1,0}, {2,0}, {3,0}, {4,0}, {5,0}, {6,0}, {7,0}, {8,0}, {9,0},
        {10,0}, {10,1}, {10,2}, {10,3}, {10,4}, {10,5}, {10,6}, {10,7}, {10,8}, {10,9}, {10,10}
    };
    heroe.path_len = sizeof(ruta_heroe)/sizeof(ruta_heroe[0]);

    // Colocar héroe y monstruo en el mapa
    mapa[heroe.pos.x][heroe.pos.y] = 1;
    mapa[monstruo.pos.x][monstruo.pos.y] = 2;

    cout << "=== JUEGO INICIADO ===\n";
    cout << "Presiona 'n' + Enter para avanzar turno.\n\n";

    char tecla;

    while (heroe.alive && monstruo.alive && !heroe.reached_goal) {
        cout << "Presiona 'n' para siguiente turno: ";
        cin >> tecla;
        if (tecla != 'n') continue;

        // --- Turno del héroe ---
        moverHeroe(heroe, ruta_heroe);

        // --- Turno del monstruo ---
        comprobarVision(monstruo, heroe);
        moverMonstruo(monstruo, heroe);

        // --- Verificar colisión ---
        if (monstruo.pos.x == heroe.pos.x && monstruo.pos.y == heroe.pos.y) {
            heroe.alive = false;
            cout << "💀 El monstruo atrapó al héroe!" << endl;
        }

        // --- Mostrar mapa ---
        imprimirMapa();

        if (heroe.reached_goal)
            cout << "🏆 ¡El héroe ha llegado a su objetivo!" << endl;
    }

    cout << "\n=== JUEGO TERMINADO ===" << endl;
    return 0;
}
