void moverHeroe(Heroe &heroe, vector<vector<int>>& mapa, const vector<Pos>& ruta) {
    if (heroe.path_idx < heroe.path_len) {
        mapa[heroe.pos.x][heroe.pos.y] = 0; // limpia la posición anterior
        heroe.pos = ruta[heroe.path_idx];
        mapa[heroe.pos.x][heroe.pos.y] = 1; // marca nueva posición
        heroe.path_idx++;

        if (heroe.path_idx >= heroe.path_len)
            heroe.reached_goal = true;
    }
}

-- EN EL MAIN ---

 vector<Pos> ruta_heroe = {
        {0,0}, {1,0}, {2,0}, {3,0}, {4,0}, {5,0}, {6,0}, {7,0}, {8,0}, {9,0},
        {10,0}, {10,1}, {10,2}, {10,3}, {10,4}, {10,5}, {10,6}, {10,7}, {10,8}, {10,9}, {10,10}
    };
   moverHeroe(heroe, mapa, ruta_heroe);
