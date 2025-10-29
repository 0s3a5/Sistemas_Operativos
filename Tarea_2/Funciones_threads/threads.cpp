void* hiloHeroe(void*) {
    while (juego_activo && heroe.alive && !heroe.reached_goal) {
        // hero tiene su semáforo; main dará permiso inicial
        sem_wait(&sem_heroe);

        // esperar tecla 'n' (se lee con bloqueo fuera del mutex)
        char tecla;
        do {
            cout << "Presiona 'n' + Enter para siguiente turno: ";
            cin >> tecla;
        } while (tecla != 'n');

        // mover + ataque del héroe
        moverHeroe();
        ataqueHeroe();

        // mostrar mapa parcial antes de los monstruos (opcional)
        imprimirMapa();

        // dar turno a cada monstruo uno por uno
        for (int i = 0; i < 3; ++i) {
            sem_post(&sem_monstruos[i]);
        }
    }
    // Si sale, desbloquear monstruos para que terminen si quedan esperando
    for (int i = 0; i < 3; ++i) sem_post(&sem_monstruos[i]);
    return nullptr;
}

void* hiloMonstruo(void* arg) {
    int idx = *((int*)arg);
    while (juego_activo && monstruos[idx].alive && heroe.alive) {
        sem_wait(&sem_monstruos[idx]);

        if (!juego_activo) break;
        if (!monstruos[idx].alive) {
            // si murió mientras esperaba, el último devuelve turno al héroe
            if (idx == 2) sem_post(&sem_heroe);
            continue;
        }

        // comprobar visión, moverse y atacar si procede
        moverYAtacarMonstruo(monstruos[idx]);

        // si es el último monstruo, devolver turno al héroe (ciclo)
        if (idx == 2) sem_post(&sem_heroe);
    }
    return nullptr;
}
