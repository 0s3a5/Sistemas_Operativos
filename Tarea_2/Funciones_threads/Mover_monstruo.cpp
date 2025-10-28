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
