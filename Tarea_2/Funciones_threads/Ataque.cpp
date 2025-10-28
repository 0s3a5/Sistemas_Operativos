void ejecutarAtaques(Heroe &heroe, Monstruo monstruos[], int numMonstruos) {
    lock_guard<mutex> lock(mtx);

    // ---- Héroe ataca primero ----
    if (!heroe.alive) return;

    bool atacó = false;
    for (int i = 0; i < numMonstruos; i++) {
        if (!monstruos[i].alive) continue;

        int dist = abs(heroe.pos.x - monstruos[i].pos.x) + abs(heroe.pos.y - monstruos[i].pos.y);
        if (dist <= heroe.attack_range) {
            monstruos[i].hp -= heroe.attack_damage;
            cout << "🗡️  Héroe ataca al Monstruo " << monstruos[i].id
                 << " (-" << heroe.attack_damage << " HP)" << endl;
            cout << "    Monstruo " << monstruos[i].id << " queda con " << monstruos[i].hp << " HP." << endl;
            atacó = true;

            if (monstruos[i].hp <= 0) {
                monstruos[i].alive = false;
                cout << "💀 Monstruo " << monstruos[i].id << " ha muerto." << endl;
            }
            break; // héroe solo ataca una vez
        }
    }

    if (!atacó)
        cout << "⚔️  Héroe no tiene enemigos en rango." << endl;

    // ---- Monstruos atacan luego ----
    for (int i = 0; i < numMonstruos; i++) {
        if (!monstruos[i].alive) continue;

        int dist = abs(monstruos[i].pos.x - heroe.pos.x) + abs(monstruos[i].pos.y - heroe.pos.y);
        if (dist <= monstruos[i].attack_range) {
            heroe.hp -= monstruos[i].attack_damage;
            cout << "🔥 Monstruo " << monstruos[i].id << " ataca al héroe (-"
                 << monstruos[i].attack_damage << " HP)" << endl;
            cout << "    Héroe queda con " << heroe.hp << " HP." << endl;

            if (heroe.hp <= 0) {
                heroe.alive = false;
                cout << "💀 ¡El héroe ha muerto!" << endl;
                return;
            }
        }
    }
}
