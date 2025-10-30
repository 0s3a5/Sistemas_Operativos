void moverHeroeIndice(int idx) {
    Heroe &h = heroes[idx];
    pthread_mutex_lock(&mtx);
    if (!h.alive || h.reached_goal) {
        pthread_mutex_unlock(&mtx);
        return;
    }
    if (h.path_idx < (int)h.ruta.size()) {
        if (h.pos.x >= 0 && h.pos.x < ROWS && h.pos.y >= 0 && h.pos.y < COLS) {// limites del mapa
            if (mapa[h.pos.x][h.pos.y] == 1){//se cambia a posicion vacia
                mapa[h.pos.x][h.pos.y] = 0;
            }
        }
        h.pos = h.ruta[h.path_idx];
        if (h.pos.x >= 0 && h.pos.x < ROWS && h.pos.y >= 0 && h.pos.y < COLS){// se verifica que no pase el mapa
            mapa[h.pos.x][h.pos.y] = 1;//se marca casilla como heroe
        }
        h.path_idx++;
        if (h.path_idx >= (int)h.ruta.size()){
            h.reached_goal = true;//se verifica si llego a la posicion
        }
    }
    pthread_mutex_unlock(&mtx);
}
