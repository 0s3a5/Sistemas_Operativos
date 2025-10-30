void ataqueHeroeIndice(int idx) {
    Heroe &h = heroes[idx];
    pthread_mutex_lock(&mtx);
    if (!h.alive) {
pthread_mutex_unlock(&mtx); return; 
    }

    bool ataco = false;
    for (size_t j = 0; j < monstruos.size(); ++j) {
        Monstruo &m = monstruos[j];
        if (!m.alive){
            continue;
        }
        int dist = distanciaManhattan(h.pos, m.pos);
        if (dist <= h.attack_range) {
            m.hp -= h.attack_damage;
            cout << " heroe" << h.id << " ataca a mosntruo" << m.id<<endl;
            cout << " (-" << h.attack_damage << " HP) le queda" << max(0, m.hp) << endl;;
            if (m.hp <= 0) {
                m.alive = false;
                if (m.pos.x >= 0 && m.pos.x < ROWS && m.pos.y >=0 && m.pos.y < COLS){
                    mapa[m.pos.x][m.pos.y] = 0;
                cout << "monstruo" << m.id << " murio"<< endl;}
                
            }
            ataco = true;
            break; // sólo 1 ataque por héroe por turno
        }
    }
    if (!ataco) {
        cout << "heroe" << h.id << " sin monstruo en rango"<<endl;
    }
    pthread_mutex_unlock(&mtx);
}
