
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
