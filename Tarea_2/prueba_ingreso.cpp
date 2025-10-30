#include <iostream>
#include <fstream>      // Para leer archivos (ifstream)
#include <string>
#include <vector>
#include <sstream>      // Para parsear strings (stringstream)
#include <map>          // Para guardar héroes/monstruos por ID
#include <cctype>       // Para isspace (verificar espacios en blanco)

// --- ESTRUCTURAS DE DATOS ---
// Almacena una coordenada (x, y)
struct Coord {
    int x = 0;
    int y = 0;
};

// Almacena los datos de un Héroe
struct Hero {
    int id = 0;
    int hp = 0;
    int attackDamage = 0;
    int attackRange = 0;
    Coord startPos;
    std::vector<Coord> path;
};

// Almacena los datos de un Monstruo
struct Monster {
    int id = 0;
    int hp = 0;
    int attackDamage = 0;
    int visionRange = 0;
    int attackRange = 0;
    Coord coords;
};

// Contenedor principal para todos los datos del juego
struct GameData {
    int gridWidth = 0;
    int gridHeight = 0;
    int monsterCount = 0;
    std::vector<Hero> heroes;
    std::vector<Monster> monsters;
};

/**
 * @brief Parsea un string con formato "(x,y)" a un struct Coord.
 * @param s El string de coordenada, ej: "(15,4)"
 * @return Un struct Coord.
 */
Coord parseCoord(const std::string& s) {
    // Quita el '(' inicial y el ')' final
    std::string cleaned = s.substr(1, s.length() - 2); // -> "x,y"
    
    std::stringstream ss(cleaned);
    std::string segment;
    Coord c;
    
    // Lee hasta la coma (,)
    std::getline(ss, segment, ',');
    c.x = std::stoi(segment);
    
    // Lee el resto (después de la coma)
    std::getline(ss, segment, ',');
    c.y = std::stoi(segment);
    
    return c;
}

/**
 * @brief Consume (lee y descarta) cualquier espacio en blanco o 
 * salto de línea en el stream del archivo.
 */
void consumeWhitespace(std::ifstream& file) {
    while (std::isspace(file.peek())) {
        file.get();
    }
}

// --- PROGRAMA PRINCIPAL ---
int main(int argc, char* argv[]) {
    // 1. Verificar que se pasó un nombre de archivo
    if (argc != 2) {
        std::cerr << "Uso: " << argv[0] << " <archivo.txt>" << std::endl;
        return 1;
    }

    // 2. Intentar abrir el archivo
    std::string filename = argv[1];
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: No se pudo abrir el archivo " << filename << std::endl;
        return 1;
    }

    GameData gameData;
    // Usamos 'mapas' para guardar temporalmente por ID,
    // permitiendo que el archivo defina héroes en cualquier orden.
    std::map<int, Hero> heroMap;
    std::map<int, Monster> monsterMap;

    std::string token;
    
    // 3. Bucle principal de parseo
    // Lee "palabra" por "palabra" (token).
    // Esto mágicamente resuelve el problema de "MONSTER_6_COORDS 33 \n 18"
    // porque '>>' ignora los saltos de línea.
    while (file >> token) {
        try {
            // Ignorar líneas de comentario
            if (token[0] == '#') {
                file.ignore(1024, '\n'); // Ignora el resto de la línea
                continue;
            }

            // --- Parseo de Datos Globales ---
            if (token == "GRID_SIZE") {
                file >> gameData.gridWidth >> gameData.gridHeight;
            } else if (token == "MONSTER_COUNT") {
                file >> gameData.monsterCount;
            }
            
            // --- Parseo de HÉROES ---
            else if (token.rfind("HERO_", 0) == 0) { // Si el token empieza con "HERO_"
                size_t first_ = token.find('_');
                size_t second_ = token.find('_', first_ + 1);
                
                if (second_ == std::string::npos) continue; // Token malformado

                // Extrae el ID y el Atributo de "HERO_ID_ATRIBUTO"
                int id = std::stoi(token.substr(first_ + 1, second_ - first_ - 1));
                std::string attr = token.substr(second_ + 1);
                
                heroMap[id].id = id; // Asegura que el héroe exista en el map

                // Asigna el valor según el atributo
                if (attr == "HP") {
                    file >> heroMap[id].hp;
                } else if (attr == "ATTACK_DAMAGE") {
                    file >> heroMap[id].attackDamage;
                } else if (attr == "ATTACK_RANGE") {
                    file >> heroMap[id].attackRange;
                } else if (attr == "START") {
                    file >> heroMap[id].startPos.x >> heroMap[id].startPos.y;
                } 
                // --- El caso especial: PATH ---
                else if (attr == "PATH") {
                    // Consume cualquier espacio/salto de línea entre "HERO_1_PATH" y "(4,3)"
                    consumeWhitespace(file);
                    
                    // Mientras el siguiente caracter sea '(', es una coordenada
                    while (file.peek() == '(') {
                        std::string coord_token;
                        file >> coord_token; // Lee el "(x,y)"
                        heroMap[id].path.push_back(parseCoord(coord_token));
                        consumeWhitespace(file); // Consume espacios para el siguiente peek()
                    }
                }
            }
            
            // --- Parseo de MONSTRUOS ---
            else if (token.rfind("MONSTER_", 0) == 0) { // Si el token empieza con "MONSTER_"
                size_t first_ = token.find('_');
                size_t second_ = token.find('_', first_ + 1);

                if (second_ == std::string::npos) continue;

                int id = std::stoi(token.substr(first_ + 1, second_ - first_ - 1));
                std::string attr = token.substr(second_ + 1);

                monsterMap[id].id = id; // Asegura que el monstruo exista

                if (attr == "HP") {
                    file >> monsterMap[id].hp;
                } else if (attr == "ATTACK_DAMAGE") {
                    file >> monsterMap[id].attackDamage;
                } else if (attr == "VISION_RANGE") {
                    file >> monsterMap[id].visionRange;
                } else if (attr == "ATTACK_RANGE") {
                    // Esto funciona para "MONSTER_23_ATTACK_RANGE \n 3"
                    file >> monsterMap[id].attackRange;
                } else if (attr == "COORDS") {
                    // Esto funciona para "MONSTER_6_COORDS 33 \n 18"
                    file >> monsterMap[id].coords.x >> monsterMap[id].coords.y;
                }
            }
        
        } catch (const std::exception& e) {
            std::cerr << "Error al parsear el token '" << token << "': " << e.what() << std::endl;
        }
    }

    file.close();

    // 4. Mover los datos de los 'mapas' a los 'vectores' finales
    for (auto const& [id, hero] : heroMap) {
        gameData.heroes.push_back(hero);
    }
    for (auto const& [id, monster] : monsterMap) {
        gameData.monsters.push_back(monster);
    }

    // 5. Imprimir un resumen para verificar
    std::cout << "--- Parseo Exitoso ---" << std::endl;
    std::cout << "Grid: " << gameData.gridWidth << "x" << gameData.gridHeight << std::endl;
    std::cout << "Héroes cargados: " << gameData.heroes.size() << std::endl;
    std::cout << "Monstruos contados: " << gameData.monsterCount << std::endl;
    std::cout << "Monstruos cargados: " << gameData.monsters.size() << std::endl;
    
    std::cout << "\n--- Datos Héroe 1 ---" << std::endl;
    std::cout << "HP: " << gameData.heroes[0].hp << std::endl;
    std::cout << "Start: (" << gameData.heroes[0].startPos.x << "," << gameData.heroes[0].startPos.y << ")" << std::endl;
    std::cout << "Puntos de Path: " << gameData.heroes[0].path.size() << std::endl;
    std::cout << "Último punto del path: (" 
              << gameData.heroes[0].path.back().x << "," 
              << gameData.heroes[0].path.back().y << ")" << std::endl;

    std::cout << "\n--- Datos Monstruo 6 (el del salto de línea) ---" << std::endl;
    std::cout << "Coords: (" << gameData.monsters[5].coords.x << "," << gameData.monsters[5].coords.y << ")" << std::endl;
    
    std::cout << "\n--- Datos Monstruo 23 (el del salto de línea) ---" << std::endl;
    std::cout << "Attack Range: " << gameData.monsters[22].attackRange << std::endl;


    // Aquí puedes empezar a usar el objeto 'gameData' en tu lógica de juego
    
    return 0;
}
