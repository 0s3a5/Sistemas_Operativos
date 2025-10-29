void comprobarVision(Monstruo &monstruo, const Heroe& heroe) {
    lock_guard<mutex> lock(mtx);
    for(int i=(monstruo.pos.x-monstruo.vision_range); i<monstruo.pos.x+monstruo.vision_range; i++) {
        for(int j=(monstruo.pos.y-monstruo.vision_range); i<monstruo.pos.y+monstruo.vision_range;  j++) {
            if(mapa[i][j] == 2) {
                int distanciaX = abs(i - heroe.pos.x);
                int distanciaY = abs(j - heroe.pos.y);
                int distancia = distanciaX + distanciaY;

                if(distancia <= monstruo.vision_range) {
                    monstruo.active = true;
                    return;
                }
            }
        }
    }
}
