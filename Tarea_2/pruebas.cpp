void moverHeroe(Entidad &heroe, Posicion ruta[], int pasos) {
    for (int i = 0; i < pasos; i++) {
        this_thread::sleep_for(chrono::seconds(1));

        mtx.lock();
        // limpiar posición anterior
        matriz[heroe.pos.x][heroe.pos.y] = '.';
        // mover
        heroe.pos.x = ruta[i].x;
        heroe.pos.y = ruta[i].y;
        matriz[heroe.pos.x][heroe.pos.y] = heroe.simbolo;

        system("clear"); // en Windows usa "cls"
        imprimirMatriz();
        mtx.unlock();
    }
}

void vigilar(Entidad &monstruo, Entidad &heroe) {
    while (true) {
        this_thread::sleep_for(chrono::milliseconds(300));
        mtx.lock();
        double d = distancia(monstruo.pos, heroe.pos);
        if (d <= RANGO_DETECCION) {
            cout << "⚠️  Monstruo en (" << monstruo.pos.x << "," << monstruo.pos.y
                 << ") detecta al héroe a distancia " << d << endl;
        }
        mtx.unlock();
    }
}
