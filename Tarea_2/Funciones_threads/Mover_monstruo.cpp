
void comprobarVisionYAccionarMonstruo(int midx) {
    Monstruo &m = monstruos[midx];
    int best = -1;//heroe mas cercano
    int bestd = INT_MAX;
    pthread_mutex_lock(&mtx);
    for (size_t i = 0; i < heroes.size(); ++i) {
        if (!heroes[i].alive){
            continue;}
        int d = distanciaManhattan(m.pos, heroes[i].pos);
        if (d < bestd) {
            bestd = d; best = (int)i; }
    }
    pthread_mutex_unlock(&mtx);
    if (best == -1) return; 
    if (bestd <= m.vision_range) {//si detecta monstruo se activan todos
        pthread_mutex_lock(&mtx);
        alerta_global = true;
        if (!m.active) {
            m.active = true;
            cout << " mosntruo " << m.id << " dectoa heroe en (dist=" << bestd << ")"<<endl;
        }
        pthread_mutex_unlock(&mtx);
    }
    if (alerta_global) {
        pthread_mutex_lock(&mtx);
        m.active = true;//activa todo
        pthread_mutex_unlock(&mtx);
    } else {
        return; // nadie detectó aún, no hace nada
    }
    pthread_mutex_lock(&mtx);
    Heroe &target = heroes[best];
    if (bestd <= m.attack_range && target.alive) {//rango de ataque 
        target.hp -= m.attack_damage;
        cout << "monstruo" << m.id << " ataca a heroe" << target.id << " (-" << m.attack_damage << " HP)  vida " << max(0, target.hp) << endl;
        if (target.hp <= 0) {//si muere heore
            target.alive = false;
            if (target.pos.x >= 0 && target.pos.x < ROWS && target.pos.y >= 0 && target.pos.y < COLS){
                mapa[target.pos.x][target.pos.y] = 0;//se cambia casilla a vacia
            }
            cout << "heroe " << target.id << " murio"<<endl;
        }
        pthread_mutex_unlock(&mtx);
        return;
    }
    pthread_mutex_unlock(&mtx);// se mueve una casilla hacia el heroe si no esta en rango de ataque
    int dx = heroes[best].pos.x - m.pos.x;
    int dy = heroes[best].pos.y - m.pos.y;
    if (abs(dx) > abs(dy)) m.pos.x += (dx > 0 ? 1 : -1);
    else if (dy != 0) {
        m.pos.y += (dy > 0 ? 1 : -1);
    }
    pthread_mutex_lock(&mtx);
    if (m.pos.x >= 0 && m.pos.x < ROWS && m.pos.y >= 0 && m.pos.y < COLS){
        mapa[m.pos.x][m.pos.y] = 2;
    }
    pthread_mutex_unlock(&mtx);
}
