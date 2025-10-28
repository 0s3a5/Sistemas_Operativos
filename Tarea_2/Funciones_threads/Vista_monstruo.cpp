void comprobarVision(Monstruo &monstruo, const Heroe& heroe) {
    int dx = abs(monstruo.pos.x - heroe.pos.x);
    int dy = abs(monstruo.pos.y - heroe.pos.y);
    bool en_rango = (dx <= monstruo.vision_range && dy <= monstruo.vision_range);

    if (en_rango && !monstruo.active) {
        monstruo.active = true;
        cout << "  Monstruo " << monstruo.id << " ha visto al héroe!" << endl;
    }
}

--EN EL MAIN --

  comprobarVision(monstruo, heroe);
