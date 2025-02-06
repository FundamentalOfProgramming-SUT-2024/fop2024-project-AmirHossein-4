#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <ncurses.h>
#include <time.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>

#define MAX_FLOORS 3
#define MAX_USERS 100
#define USERNAME_LENGTH 50
#define MAX_PASSWORD_LENGTH 50  
#define EMAIL_LENGTH 100
#define MAP_WIDTH 70
#define MAP_HEIGHT 30
#define MAX_ITEMS 100
#define ROOM_MAX_SIZE 15
#define ROOM_MIN_SIZE 6
#define MAX_ROOMS 10
#define MAX_EXIT_POINTS 4
#define MAX_ENEMIES 10
#define MAX_FIRES 10
#define MAX_BULLETS 10
#define MAX_FOODS 10
#define PASSWORD_LENGTH 4


// پیش اعلان ساختارها
typedef struct ExitPoint ExitPoint;
typedef struct Item Item;
typedef struct Room Room;
typedef struct Enemy Enemy;
typedef struct Fire Fire;
typedef struct Bullet Bullet;
typedef struct Food Food;
typedef struct Map Map;
typedef struct Player Player;
typedef struct GameState GameState;

// ساختار ExitPoint
struct ExitPoint {
    int x, y;
};

// ساختار Item
struct Item {
    int x, y;
    char symbol;
    char type;
    int value;
    int ammo;
};

// ساختار Room
struct Room {
    int x, y;
    int width, height;
};

// ساختار Enemy
struct Enemy {
    int x, y;
    char symbol;
    int health;
    int damage;
    int speed;
    int is_boss;
    char type;
    int room_index;
};

// ساختار Fire
struct Fire {
    int x, y;
    char symbol;
    int damage;
};

// ساختار Bullet
struct Bullet {
    int x, y;
    char symbol;
    int dx, dy;
};

// ساختار Food
struct Food {
    int x, y;
    char symbol;
    int is_poisonous;
};

// ساختار Map
struct Map {
    char tiles[MAP_HEIGHT][MAP_WIDTH];
    Item items[MAX_ITEMS];
    Room rooms[MAX_ROOMS];
    Enemy enemies[MAX_ENEMIES];
    Fire fires[MAX_FIRES];
    Bullet bullets[MAX_BULLETS];
    Food foods[MAX_FOODS];
    int item_count;
    int enemy_count;
    int fire_count;
    int bullet_count;
    int food_count;
    int level;
    int boss_active;
    int boss_room_active;
    int show_full_map;
    ExitPoint exits[MAX_EXIT_POINTS];
};

// ساختار Player
struct Player {
    int x, y;
    char symbol;
    int health;
    int gold;
    int score;
    int weapon_power;
    int ghost_mode;
    int current_room;
    int ammo;
    int cheat_mode;
    int current_color;
};

struct GameState {
    Map maps[MAX_FLOORS];
    int current_floor;
    int total_floors; 
    Player player;
    time_t start_time;
    int difficulty;
    int auto_save;
};


void generate_multi_floor_map(GameState *game);
void check_floor_transition(GameState *game, int direction);
void generate_random_map(Map *map);
void initialize_player(Player *player, int x, int y);
void game_menu(GameState *game);
void print_map_with_player(GameState *game, Map *map, Player *player);


void generate_multi_floor_map(GameState *game) {
    
    game->total_floors = MAX_FLOORS;
    game->current_floor = 0;
    
    for(int i = 0; i < MAX_FLOORS; i++) {
        generate_random_map(&game->maps[i]);
        
        if(i < MAX_FLOORS-1) {
            
            Room *last_room = &game->maps[i].rooms[MAX_ROOMS-1];
            game->maps[i].items[game->maps[i].item_count++] = (Item){
                .x = last_room->x + last_room->width/2,
                .y = last_room->y + last_room->height/2,
                .symbol = 'S',
                .type = 'S'
            };
        }
    }
    
    
    game->maps[MAX_FLOORS-1].boss_active = 1;
}

void check_floor_transition(GameState *game, int direction) {
    Map *current_map = &game->maps[game->current_floor];
    
    for(int i = 0; i < current_map->item_count; i++) {
        if(current_map->items[i].type == 'S' && 
           abs(game->player.x - current_map->items[i].x) <= 1 &&
           abs(game->player.y - current_map->items[i].y) <= 1) {
            
            int new_floor = game->current_floor + direction;
            
            if(new_floor >= 0 && new_floor < game->total_floors) {
                game->current_floor = new_floor;
                Room *target_room = &game->maps[game->current_floor].rooms[0];
                game->player.x = target_room->x + target_room->width/2;
                game->player.y = target_room->y + target_room->height/2;
            }
            break;
        }
    }
}

void initialize_map(Map *map) {
    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            if (x == 0 || x == MAP_WIDTH - 1 || y == 0 || y == MAP_HEIGHT - 1) {
                map->tiles[y][x] = '#';
            } else {
                map->tiles[y][x] = ' ';
            }
        }
    }
    map->item_count = 0;
    map->enemy_count = 0;
    map->fire_count = 0;
    map->bullet_count = 0;
    map->food_count = 0;
    map->level = 1;
    map->boss_active = 0;
    map->boss_room_active = 0;
    map->show_full_map = 0;
}

void createRoom(Room *room) {
    room->width = ROOM_MIN_SIZE + rand() % (ROOM_MAX_SIZE - ROOM_MIN_SIZE + 1);
    room->height = ROOM_MIN_SIZE + rand() % (ROOM_MAX_SIZE - ROOM_MIN_SIZE + 1);
    room->x = rand() % (MAP_WIDTH - room->width - 1) + 1;
    room->y = rand() % (MAP_HEIGHT - room->height - 1) + 1;
}

int roomsOverlap(Room *a, Room *b) {
    return (a->x <= b->x + b->width && a->x + a->width >= b->x &&
            a->y <= b->y + b->height && a->y + a->height >= b->y);
}

void drawRoom(Room *room, Map *map) {
    for (int y = room->y; y < room->y + room->height; y++) {
        for (int x = room->x; x < room->x + room->width; x++) {
            if (y == room->y || y == room->y + room->height - 1 ||
                x == room->x || x == room->x + room->width - 1) {
                map->tiles[y][x] = '#';
            } else {
                map->tiles[y][x] = '.';
            }
        }
    }
}

void drawCorridor(Room *a, Room *b, Map *map) {
    int x1 = a->x + a->width / 2;
    int y1 = a->y + a->height / 2;
    int x2 = b->x + b->width / 2;
    int y2 = b->y + b->height / 2;

    for (int x = (x1 < x2 ? x1 : x2); x <= (x1 > x2 ? x1 : x2); x++) {
        map->tiles[y1][x] = '.';
    }

    for (int y = (y1 < y2 ? y1 : y2); y <= (y1 > y2 ? y1 : y2); y++) {
        map->tiles[y][x2] = '.';
    }
}

void initialize_enemy(Enemy *enemy, int x, int y, int room_index, char type) {
    enemy->x = x;
    enemy->y = y;
    enemy->symbol = type == 'S' ? 'S' : 
                    type == 'B' ? 'B' : 
                    type == 'X' ? 'X' : 
                    type == 'Y' ? 'Y' : 
                    type == 'Z' ? 'Z' : 'E';
    enemy->health = type == 'S' ? 70 : 
                    type == 'B' ? 500 : 
                    type == 'X' ? 60 : 
                    type == 'Y' ? 80 : 
                    type == 'Z' ? 90 : 50;
    enemy->damage = type == 'S' ? 15 : 
                    type == 'B' ? 30 : 
                    type == 'X' ? 12 : 
                    type == 'Y' ? 18 : 
                    type == 'Z' ? 20 : 10;
    enemy->speed = 1;
    enemy->is_boss = (type == 'B');
    enemy->type = type;
    enemy->room_index = room_index;
}


void initialize_fire(Fire *fire, int x, int y) {
    fire->x = x;
    fire->y = y;
    fire->symbol = '^';
    fire->damage = 5;
}

void initialize_bullet(Bullet *bullet, int x, int y, int dx, int dy) {
    bullet->x = x;
    bullet->y = y;
    bullet->symbol = '*';
    bullet->dx = dx;
    bullet->dy = dy;
}

void initialize_food(Food *food, int x, int y) {
    food->x = x;
    food->y = y;
    food->symbol = "opi"[rand() % 3];
    food->is_poisonous = rand() % 2;
}

void generate_random_map(Map *map) {
    srand(time(NULL));
    initialize_map(map);

    int roomCount = 0;

    while (roomCount < MAX_ROOMS) {
        Room newRoom;
        createRoom(&newRoom);

        int failed = 0;
        for (int i = 0; i < roomCount; i++) {
            if (roomsOverlap(&newRoom, &map->rooms[i])) {
                failed = 1;
                break;
            }
        }

        if (!failed || roomCount == 0) {
            drawRoom(&newRoom, map);

            if (roomCount != 0) {
                drawCorridor(&newRoom, &map->rooms[roomCount - 1], map);
            }

            map->rooms[roomCount] = newRoom;
            roomCount++;
        }
    }

    // Add items
    for (int i = 0; i < 10; i++) {
        int roomIndex = rand() % roomCount;
        Room *room = &map->rooms[roomIndex];
        map->items[i].x = room->x + 1 + rand() % (room->width - 2);
        map->items[i].y = room->y + 1 + rand() % (room->height - 2);
        map->items[i].symbol = 'G';
        map->items[i].type = 'G';
        map->items[i].value = rand() % 10 + 1;
        map->item_count++;
    }

    for (int i = 10; i < 15; i++) {
        int roomIndex = rand() % roomCount;
        Room *room = &map->rooms[roomIndex];
        map->items[i].x = room->x + 1 + rand() % (room->width - 2);
        map->items[i].y = room->y + 1 + rand() % (room->height - 2);
        map->items[i].symbol = 'H';
        map->items[i].type = 'H';
        map->items[i].value = rand() % 20 + 10;
        map->item_count++;
    }

    for (int i = 15; i < 20; i++) {
        int roomIndex = rand() % roomCount;
        Room *room = &map->rooms[roomIndex];
        map->items[i].x = room->x + 1 + rand() % (room->width - 2);
        map->items[i].y = room->y + 1 + rand() % (room->height - 2);
        map->items[i].symbol = 'W';
        map->items[i].type = 'W';
        map->items[i].value = rand() % 10 + 5;
        map->items[i].ammo = rand() % 20 + 10;
        map->item_count++;
    }

    for (int i = 20; i < 23; i++) {
        int roomIndex = rand() % roomCount;
        Room *room = &map->rooms[roomIndex];
        map->items[i].x = room->x + 1 + rand() % (room->width - 2);
        map->items[i].y = room->y + 1 + rand() % (room->height - 2);
        map->items[i].symbol = 'T';
        map->items[i].type = 'T';
        map->items[i].value = i - 19;
        map->item_count++;
    }

    // Add U items for boss activation
    for (int i = 23; i < 25; i++) {
        int roomIndex = rand() % roomCount;
        Room *room = &map->rooms[roomIndex];
        map->items[i].x = room->x + 1 + rand() % (room->width - 2);
        map->items[i].y = room->y + 1 + rand() % (room->height - 2);
        map->items[i].symbol = 'U';
        map->items[i].type = 'U';
        map->items[i].value = 0;
        map->item_count++;
    }

    // Initialize enemies
    for (int i = 0; i < MAX_ENEMIES; i++) {
        int roomIndex = rand() % roomCount;
        Room *room = &map->rooms[roomIndex];
        char type = 'E';
        if (rand() % 5 == 0) type = 'S';
        if (i == 0 && map->level % 3 == 0) type = 'B';
           
    // اضافه کردن دشمن‌های جدید X، Y و Z
    if (i == 1) type = 'X';
    if (i == 2) type = 'Y';
    if (i == 3) type = 'Z';
        initialize_enemy(&map->enemies[i],
                         room->x + 1 + rand() % (room->width - 2),
                         room->y + 1 + rand() % (room->height - 2),
                         roomIndex,
                         type);
        map->enemy_count++;
    }

    // Initialize fires
    for (int i = 0; i < MAX_FIRES; i++) {
        int roomIndex = rand() % roomCount;
        Room *room = &map->rooms[roomIndex];
        initialize_fire(&map->fires[i],
                        room->x + 1 + rand() % (room->width - 2),
                        room->y + 1 + rand() % (room->height - 2));
        map->fire_count++;
    }

    // Initialize foods
    for (int i = 0; i < MAX_FOODS; i++) {
        int roomIndex = rand() % roomCount;
        Room *room = &map->rooms[roomIndex];
        initialize_food(&map->foods[i],
                        room->x + 1 + rand() % (room->width - 2),
                        room->y + 1 + rand() % (room->height - 2));
        map->food_count++;
    }
}

void initialize_player(Player *player, int x, int y) {
    player->x = x;
    player->y = y;
    player->symbol = '@';
    player->health = 100;
    player->gold = 0;
    player->score = 0;
    player->weapon_power = 10;
    player->ghost_mode = 0;
    player->current_room = -1;
    player->ammo = 50;
    player->cheat_mode = 0;
    player->current_color = 6;
}

void ensure_player_on_floor(Player *player, Map *map) {
    if (map->tiles[player->y][player->x] != '.') {
        int found = 0;
        for (int y = 0; y < MAP_HEIGHT && !found; y++) {
            for (int x = 0; x < MAP_WIDTH && !found; x++) {
                if (map->tiles[y][x] == '.') {
                    player->x = x;
                    player->y = y;
                    found = 1;
                }
            }
        }

        if (!found) {
            Room emergencyRoom = {5, 5, 10, 10};
            drawRoom(&emergencyRoom, map);
            player->x = 6;
            player->y = 6;
        }
    }
}

void move_enemy_randomly(Enemy *enemy, Map *map) {
    Room *room = &map->rooms[enemy->room_index];
    int dx = rand() % 3 - 1;
    int dy = rand() % 3 - 1;

    int new_x = enemy->x + dx;
    int new_y = enemy->y + dy;

    if (new_x >= room->x && new_x < room->x + room->width &&
        new_y >= room->y && new_y < room->y + room->height &&
        map->tiles[new_y][new_x] == '.') {
        enemy->x = new_x;
        enemy->y = new_y;
    }
}

void move_toxic_enemy(Enemy *enemy, Player *player, Map *map) {
    int dx = (player->x > enemy->x) ? 1 : (player->x < enemy->x) ? -1 : 0;
    int dy = (player->y > enemy->y) ? 1 : (player->y < enemy->y) ? -1 : 0;

    if (rand() % 2 == 0) {
        enemy->x += dx;
        enemy->y += dy;
    }
}

void move_boss_towards_player(Enemy *boss, Player *player) {
    if (rand() % 100 < 50) {
        if (boss->x < player->x) boss->x += boss->speed;
        else if (boss->x > player->x) boss->x -= boss->speed;

        if (boss->y < player->y) boss->y += boss->speed;
        else if (boss->y > player->y) boss->y -= boss->speed;
    }
}

void fire_weapon(Player *player, Map *map) {
    if (player->ammo > 0) {
        player->ammo--;
        int damage_dealt = 0;

        for (int i = 0; i < map->enemy_count; i++) {
            if (abs(map->enemies[i].x - player->x) <= 2 &&
                abs(map->enemies[i].y - player->y) <= 2) {

                int damage = player->weapon_power;
                if (map->enemies[i].is_boss) {
                    damage = damage / 2;
                }

                map->enemies[i].health -= damage;
                damage_dealt += damage;

                if (map->enemies[i].health <= 0) {
                    player->score += map->enemies[i].is_boss ? 100 : 10;
                    map->enemies[i] = map->enemies[map->enemy_count - 1];
                    map->enemy_count--;
                }
            }
        }

        printw("Fired! Damage: %d | Ammo: %d\n", damage_dealt, player->ammo);
    }
}

void create_boss_room(Map *map, Player *player) {
    initialize_map(map);

    int boss_room_width = 30;
    int boss_room_height = 15;
    int start_x = (MAP_WIDTH - boss_room_width) / 2;
    int start_y = (MAP_HEIGHT - boss_room_height) / 2;

    for (int y = start_y; y < start_y + boss_room_height; y++) {
        for (int x = start_x; x < start_x + boss_room_width; x++) {
            if (x == start_x || x == start_x + boss_room_width - 1 ||
                y == start_y || y == start_y + boss_room_height - 1) {
                map->tiles[y][x] = '#';
            } else {
                map->tiles[y][x] = '.';
            }
        }
    }

    Enemy boss = {
        start_x + boss_room_width / 2,
        start_y + boss_room_height / 2,
        'B', 500, 30, 1, 1, 'B', -1
    };
    map->enemies[map->enemy_count++] = boss;

    player->x = start_x + 2;
    player->y = start_y + 2;

    map->item_count = 0;
    map->fire_count = 0;
    map->boss_room_active = 1;
}

void activate_boss(Map *map, Player *player) {
    if (!map->boss_active) {
        map->boss_active = 1;
        create_boss_room(map, player);
        player->x = (MAP_WIDTH - 30) / 2 + 2;
        player->y = (MAP_HEIGHT - 15) / 2 + 2;
    }
}

void move_player(Player *player, Map *map, int dx, int dy, int speed) {
    if (player->cheat_mode) {
        while (1) {
            int new_x = player->x + dx * speed;
            int new_y = player->y + dy * speed;

            if (new_x < 0 || new_x >= MAP_WIDTH || new_y < 0 || new_y >= MAP_HEIGHT) {
                break;
            }

            char target_tile = map->tiles[new_y][new_x];

            if (target_tile == '.' || (target_tile == '#' && player->ghost_mode)) {
                player->x = new_x;
                player->y = new_y;
            } else {
                break;
            }

            for (int i = 0; i < map->item_count; i++) {
                if (map->items[i].x == player->x && map->items[i].y == player->y) {
                    if (map->items[i].type == 'G') {
                        player->gold += map->items[i].value;
                        
                    } else if (map->items[i].type == 'H') {
                        player->health += map->items[i].value;
                        
                    } else if (map->items[i].type == 'W') {
                        player->weapon_power += map->items[i].value;
                        player->ammo += map->items[i].ammo;
                    } else if (map->items[i].type == 'T') {
                        switch (map->items[i].value) {
                            case 1:
                                activate_boss(map, player);
                                
                                break;
                            case 2:
                                player->health = INT_MAX;
                            
                                break;
                            case 3:
                                player->ammo = INT_MAX;
                                
                                break;
                        }
                    } else if (map->items[i].type == 'U') { // اضافه شدن پردازش آیتم U
                        activate_boss(map, player);
                        printw("You activated the boss with U!\n");
                    }

                    map->items[i] = map->items[map->item_count - 1];
                    map->item_count--;
                    break;
                }
            }

            for (int i = 0; i < map->food_count; i++) {
                if (map->foods[i].x == player->x && map->foods[i].y == player->y) {
                    if (map->foods[i].is_poisonous) {
                        player->health -= 20;
                    } else {
                        player->health += 10;
                    }
                    map->foods[i] = map->foods[map->food_count - 1];
                    map->food_count--;
                    break;
                }
            }

            for (int i = 0; i < map->enemy_count; i++) {
                if (map->enemies[i].x == player->x && map->enemies[i].y == player->y) {
                    player->health -= map->enemies[i].damage;
                    break;
                }
            }

            for (int i = 0; i < map->fire_count; i++) {
                if (map->fires[i].x == player->x && map->fires[i].y == player->y) {
                    player->health -= map->fires[i].damage;
                    printw("Fire damage! Health: %d\n", player->health);
                    break;
                }
            }

            if (map->item_count == 0 && !map->boss_active) {
                activate_boss(map, player);
            }
            GameState game;
            print_map_with_player(&game,map, player);
            refresh();
            napms(100);
        }
    } else {
        int new_x = player->x + dx * speed;
        int new_y = player->y + dy * speed;

        if (new_x >= 0 && new_x < MAP_WIDTH && new_y >= 0 && new_y < MAP_HEIGHT) {
            char target_tile = map->tiles[new_y][new_x];

            if (target_tile == '.' || (target_tile == '#' && player->ghost_mode)) {
                int prev_room = player->current_room;
                int new_room = -1;

                for (int i = 0; i < MAX_ROOMS; i++) {
                    Room *room = &map->rooms[i];
                    if (new_x >= room->x && new_x < room->x + room->width &&
                        new_y >= room->y && new_y < room->y + room->height) {
                        new_room = i;
                        break;
                    }
                }

                if (prev_room != -1 && new_room == -1) {
                    for (int i = 0; i < MAX_EXIT_POINTS; i++) {
                        ExitPoint ep = map->exits[i];
                    }
                }

                player->x = new_x;
                player->y = new_y;
                player->current_room = new_room;

                for (int i = 0; i < map->item_count; i++) {
                    if (map->items[i].x == player->x && map->items[i].y == player->y) {
                        if (map->items[i].type == 'G') {
                            player->gold += map->items[i].value;
                            printw("You found %d gold!\n", map->items[i].value);
                        } else if (map->items[i].type == 'H') {
                            player->health += map->items[i].value;
                            printw("Health +%d!\n", map->items[i].value);
                        } else if (map->items[i].type == 'W') {
                            player->weapon_power += map->items[i].value;
                            player->ammo += map->items[i].ammo;
                            printw("Weapon upgraded! Power +%d | Ammo +%d\n",
                                  map->items[i].value, map->items[i].ammo);
                        } else if (map->items[i].type == 'T') {
                            switch (map->items[i].value) {
                                case 1:
                                    activate_boss(map, player);
                                    printw("You activated the boss!\n");
                                    break;
                                case 2:
                                    player->health = INT_MAX;
                                    printw("Your health is now infinite!\n");
                                    break;
                                case 3:
                                    player->ammo = INT_MAX;
                                    printw("Your ammo is now infinite!\n");
                                    break;
                            }
                        } else if (map->items[i].type == 'U') { // پردازش آیتم U
                            activate_boss(map, player);
                            printw("You activated the boss with U!\n");
                        }

                        map->items[i] = map->items[map->item_count - 1];
                        map->item_count--;
                        break;
                    }
                }

                for (int i = 0; i < map->food_count; i++) {
                    if (map->foods[i].x == player->x && map->foods[i].y == player->y) {
                        if (map->foods[i].is_poisonous) {
                            player->health -= 20;
                            printw("You ate poisonous food! Health -20\n");
                        } else {
                            player->health += 10;
                            printw("You ate food! Health +10\n");
                        }
                        map->foods[i] = map->foods[map->food_count - 1];
                        map->food_count--;
                        break;
                    }
                }

                for (int i = 0; i < map->enemy_count; i++) {
                    if (map->enemies[i].x == player->x && map->enemies[i].y == player->y) {
                        player->health -= map->enemies[i].damage;
                        printw("Attacked by %s! Health: %d\n",
                              map->enemies[i].is_boss ? "BOSS" : "enemy",
                              player->health);
                        break;
                    }
                }

                for (int i = 0; i < map->fire_count; i++) {
                    if (map->fires[i].x == player->x && map->fires[i].y == player->y) {
                        player->health -= map->fires[i].damage;
                        printw("Fire damage! Health: %d\n", player->health);
                        break;
                    }
                }

                if (map->item_count == 0 && !map->boss_active) {
                    activate_boss(map, player);
                }
            }
        }
    }
}

void print_map_with_player(GameState *game, Map *map, Player *player) {
    clear();
    int vision_radius = 4;

    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            int dx = abs(x - player->x);
            int dy = abs(y - player->y);

            if (map->show_full_map || (dx <= vision_radius && dy <= vision_radius)) {
                if (x == player->x && y == player->y) {
                    attron(COLOR_PAIR(player->current_color));
                    printw("@");
                    attroff(COLOR_PAIR(player->current_color));
                } else {
                    int printed = 0;

                    // Print enemies
                    for (int i = 0; i < map->enemy_count; i++) {
                        if (map->enemies[i].x == x && map->enemies[i].y == y) {
                            int color_pair = 1;
                            if (map->enemies[i].type == 'B') color_pair = 2;
                            else if (map->enemies[i].type == 'S') color_pair = 7;
                            attron(COLOR_PAIR(color_pair));
                            printw("%c", map->enemies[i].symbol);
                            attroff(COLOR_PAIR(color_pair));
                            printed = 1;
                            break;
                        }
                    }

                    // Print fires
                    if (!printed) {
                        for (int i = 0; i < map->fire_count; i++) {
                            if (map->fires[i].x == x && map->fires[i].y == y) {
                                attron(COLOR_PAIR(3));
                                printw("%c", map->fires[i].symbol);
                                attroff(COLOR_PAIR(3));
                                printed = 1;
                                break;
                            }
                        }
                    }

                    // Print items
                    if (!printed) {
                        for (int i = 0; i < map->item_count; i++) {
                            if (map->items[i].x == x && map->items[i].y == y) {
                                int color_pair = 4;
                                if (map->items[i].type == 'U') color_pair = 10; // رنگ جدید برای U
                                attron(COLOR_PAIR(color_pair));
                                printw("%c", map->items[i].symbol);
                                attroff(COLOR_PAIR(color_pair));
                                printed = 1;
                                break;
                            }
                        }
                    }

                    // Print foods
                    if (!printed) {
                        for (int i = 0; i < map->food_count; i++) {
                            if (map->foods[i].x == x && map->foods[i].y == y) {
                                attron(COLOR_PAIR(map->foods[i].is_poisonous ? 8 : 9));
                                printw("%c", map->foods[i].symbol);
                                attroff(COLOR_PAIR(map->foods[i].is_poisonous ? 8 : 9));
                                printed = 1;
                                break;
                            }
                        }
                    }

                    // Print bullets
                    if (!printed) {
                        for (int i = 0; i < map->bullet_count; i++) {
                            if (map->bullets[i].x == x && map->bullets[i].y == y) {
                                attron(COLOR_PAIR(5));
                                printw("%c", map->bullets[i].symbol);
                                attroff(COLOR_PAIR(5));
                                printed = 1;
                                break;
                            }
                        }
                    }

                    if (!printed) {
                        printw("%c", map->tiles[y][x]);
                    }
                }
            } else {
                printw(" ");
            }
        }
        printw("\n");
    }

printw("Floor: %d/%d | Health: %d | Gold: %d | Score: %d | Level: %d | Ghost: %s | Ammo: %d | Cheat: %s\n",
       game->current_floor + 1, game->total_floors,
       player->health, player->gold, player->score, map->level,
       player->ghost_mode ? "ON" : "OFF", player->ammo,
       player->cheat_mode ? "ON" : "OFF");
    refresh();
}

void ensure_floor_transition(GameState *game) {
    Map *new_map = &game->maps[game->current_floor];
    Room *first_room = &new_map->rooms[0];
    game->player.x = first_room->x + first_room->width/2;
    game->player.y = first_room->y + first_room->height/2;
}

void game_menu(GameState *game) {
    // Initialize ncurses settings
    initscr();
    start_color();
        init_pair(10, COLOR_RED, COLOR_BLACK);    
init_pair(11, COLOR_GREEN, COLOR_BLACK);  
init_pair(12, COLOR_BLUE, COLOR_BLACK); 
    init_pair(1, COLOR_RED, COLOR_BLACK);
    init_pair(2, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(3, COLOR_YELLOW, COLOR_BLACK);
    init_pair(4, COLOR_CYAN, COLOR_BLACK);
    init_pair(5, COLOR_GREEN, COLOR_BLACK);
    init_pair(6, COLOR_WHITE, COLOR_BLACK);
    init_pair(7, COLOR_GREEN, COLOR_BLACK);
    init_pair(8, COLOR_RED, COLOR_BLACK);
    init_pair(9, COLOR_BLUE, COLOR_BLACK);
    init_pair(10, COLOR_YELLOW, COLOR_BLACK);
    
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);

    int ch;
    int speed = 1;
    int boss_defeated = 0;
    
    // Main game loop
    while ((ch = getch()) != 'q') {
        Map *current_map = &game->maps[game->current_floor];
        Player *player = &game->player;

        // Handle input
        switch(ch) {
            case 'v': speed = 3; break;
            case 'd': case 'D': 
                player->ghost_mode = !player->ghost_mode;
                break;
            case 'f': 
                fire_weapon(player, current_map); 
                break;
            case 'j': case 'J': 
                player->cheat_mode = !player->cheat_mode;
                break;
            case 'c': case 'C':
                player->current_color = (player->current_color == 6) ? 9 : 6;
                break;
            case KEY_UP: 
                move_player(player, current_map, 0, -1, speed); 
                break;
            case KEY_DOWN: 
                move_player(player, current_map, 0, 1, speed); 
                break;
            case KEY_LEFT: 
                move_player(player, current_map, -1, 0, speed); 
                break;
            case KEY_RIGHT: 
                move_player(player, current_map, 1, 0, speed); 
                break;
            case 's': 
                current_map->show_full_map = !current_map->show_full_map; 
                break;
            case 'w': case 'W':
                check_floor_transition(game,1);
        break;
    case 'y': case 'Y': 
        check_floor_transition(game, -1);
        break;
        }

        // Check floor transition after movement
        if(ch == KEY_UP || ch == KEY_DOWN || 
           ch == KEY_LEFT || ch == KEY_RIGHT) {
            check_floor_transition(game,0);
        }

        // Enemy AI
        for(int i = 0; i < current_map->enemy_count; i++) {
            if(current_map->enemies[i].type == 'B') {
                move_boss_towards_player(&current_map->enemies[i], player);
            } else if(current_map->enemies[i].type == 'S') {
                move_toxic_enemy(&current_map->enemies[i], player, current_map);
            } else {
                move_enemy_randomly(&current_map->enemies[i], current_map);
            }
        }

        // Boss fire mechanics
        if(current_map->boss_active) {
            for(int i = 0; i < current_map->enemy_count; i++) {
                if(current_map->enemies[i].is_boss && 
                   current_map->fire_count < MAX_FIRES) {
                    if(rand() % 100 < 20) {
                        for(int dx = -1; dx <= 1; dx++) {
                            for(int dy = -1; dy <= 1; dy++) {
                                int fx = current_map->enemies[i].x + dx;
                                int fy = current_map->enemies[i].y + dy;
                                if(fx >= 0 && fx < MAP_WIDTH &&
                                   fy >= 0 && fy < MAP_HEIGHT) {
                                    initialize_fire(&current_map->fires[current_map->fire_count++], fx, fy);
                                }
                            }
                        }
                    }
                }
            }
        }

        // Check victory condition
        boss_defeated = 0;
        for(int i = 0; i < current_map->enemy_count; i++) {
            if(current_map->enemies[i].is_boss && 
               current_map->enemies[i].health <= 0) {
                boss_defeated = 1;
                break;
            }
        }

        // Update display
        clear();
        print_map_with_player(game, current_map, player);
        printw("Floor: %d/%d | Health: %d | Gold: %d | Score: %d\n",
              game->current_floor + 1, game->total_floors,
              player->health, player->gold, player->score);
        refresh();

        // End conditions
        if(boss_defeated && game->current_floor == game->total_floors - 1) {
            printw("\nFINAL VICTORY! ALL FLOORS CLEARED!\n");
            refresh();
            getch();
            break;
        }

        if(player->health <= 0) {
            printw("\nGAME OVER! Press any key...\n");
            refresh();
            getch();
            break;
        }

        speed = 1; // Reset speed after movement
    }

    // Cleanup
    endwin();
}
void main_menu() {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    start_color();
    curs_set(0);

    int highlight = 0;
    int choice = 0;
    const char *menu_items[] = {
        "Start Game(easy mode)",
        "Start Game(hard mode)",
        "Exit"
    };
    int num_items = sizeof(menu_items)/sizeof(menu_items[0]);

    init_pair(1, COLOR_WHITE, COLOR_BLUE);
    init_pair(2, COLOR_BLACK, COLOR_WHITE);

    while(1) {
        clear();
        attron(COLOR_PAIR(1) | A_BOLD);
        printw("\n  === Dungeon Adventure ===\n\n");
        attroff(COLOR_PAIR(1) | A_BOLD);

        for(int i = 0; i < num_items; i++) {
            if(i == highlight) {
                attron(COLOR_PAIR(2));
                printw("> %s \n", menu_items[i]);
                attroff(COLOR_PAIR(2));
            } else {
                printw("  %s \n", menu_items[i]);
            }
        }

        refresh();
        int c = getch();
        
        switch(c) {
            case KEY_UP:
                highlight--;
                if(highlight < 0) highlight = num_items - 1;
                break;
            case KEY_DOWN:
                highlight++;
                if(highlight >= num_items) highlight = 0;
                break;
            case 10: // Enter key
                choice = highlight + 1;
                break;
            case 'q':
                choice = num_items;
                break;
        }

        if(choice != 0) {
            break;
        }
    }

    if(choice == 1) {
        Map map;
        Player player;
        generate_random_map(&map);
        initialize_player(&player, MAP_WIDTH/2, MAP_HEIGHT/2);
        ensure_player_on_floor(&player, &map);
        game_menu(&map);
    }

    endwin();
}

// تابع برای تولید رمز چهارحرفی تصادفی
// تابع برای تولید رمز چهارحرفی تصادفی
void generate_random_password(char *password) {
    srand(time(NULL));
    for (int i = 0; i < PASSWORD_LENGTH; i++) {
        password[i] = 'A' + rand() % 26; // تولید حروف بزرگ انگلیسی
    }
    password[PASSWORD_LENGTH] = '\0'; // پایان رشته
}

// تابع برای درخواست رمز از کاربر و بررسی آن
int ask_for_password(const char *correct_password) {
    char user_password[PASSWORD_LENGTH + 1];
    printf("Enter the 4-letter password: ");
    scanf("%4s", user_password); // فقط 4 حرف اول را می‌خواند

    if (strcmp(user_password, correct_password) == 0) {
        printf("Password correct! Starting the game...\n");
        return 1; // رمز صحیح است
    } else {
        printf("Incorrect password. Access denied.\n");
        return 0; // رمز نادرست است
    }
}

// نمایش پنجره ورود رمز با رنگ
int password_window(const char *correct_pass) {
    WINDOW *win = newwin(10, 50, (LINES-10)/2, (COLS-50)/2);
    start_color();
    init_pair(1, COLOR_RED, COLOR_BLACK);
    init_pair(2, COLOR_GREEN, COLOR_BLACK);
    
    wattron(win, COLOR_PAIR(1));
    box(win, 0, 0);
    wattroff(win, COLOR_PAIR(1));
    
    int attempts = 3;
    char input[PASSWORD_LENGTH+1] = {0};
    int result = 0;
    
    while(attempts > 0) {
        werase(win);
        mvwprintw(win, 1, 2, "Enter 4-digit password:");
        mvwprintw(win, 2, 2, "Attempts left: ");
        wattron(win, COLOR_PAIR(1) | A_BOLD);
        wprintw(win, "%d", attempts);
        wattroff(win, COLOR_PAIR(1) | A_BOLD);
        
        mvwprintw(win, 4, 2, "Password: [    ]");
        wmove(win, 4, 12);
        
        echo();
        curs_set(1);
        mvwgetnstr(win, 4, 12, input, PASSWORD_LENGTH);
        noecho();
        curs_set(0);
        
        if(strcmp(input, correct_pass) == 0) {
            wattron(win, COLOR_PAIR(2) | A_BLINK);
            mvwprintw(win, 6, 2, "Correct Password!");
            wattroff(win, COLOR_PAIR(2) | A_BLINK);
            wrefresh(win);
            napms(1000);
            result = 1;
            break;
        }
        
        attempts--;
        wattron(win, COLOR_PAIR(1));
        mvwprintw(win, 6, 2, "Wrong Password!");
        wattroff(win, COLOR_PAIR(1));
        wrefresh(win);
        napms(1000);
    }
    
    delwin(win);
    return result;
}

// تابع math_challenge اصلاح شده
int math_challenge() {
    WINDOW *math_win = newwin(10, 50, (LINES-10)/2, (COLS-50)/2);
    start_color();
    init_pair(3, COLOR_CYAN, COLOR_BLACK);
    
    wattron(math_win, COLOR_PAIR(3));
    box(math_win, 0, 0);
    wattroff(math_win, COLOR_PAIR(3));
    
    int a = 10 + rand()%50;
    int b = 10 + rand()%50;
    int answer = a + b;
    int user_answer;
    int result = 0;
    
    mvwprintw(math_win, 1, 2, "Math Verification Required!");
    mvwprintw(math_win, 3, 2, "Solve: %d + %d = ?", a, b);
    mvwprintw(math_win, 5, 2, "Your answer: ");
    
    echo();
    curs_set(1);
    mvwscanw(math_win, 5, 15, "%d", &user_answer);
    noecho();
    curs_set(0);
    
    if(user_answer == answer) {
        wattron(math_win, COLOR_PAIR(2));
        mvwprintw(math_win, 7, 2, "Correct! Access granted");
        result = 1;
    } else {
        wattron(math_win, COLOR_PAIR(1));
        mvwprintw(math_win, 7, 2, "Wrong answer!");
    }
    wattroff(math_win, COLOR_PAIR(2));
    wattroff(math_win, COLOR_PAIR(1));
    
    wrefresh(math_win);
    napms(1500);
    delwin(math_win);
    
    return result;
}

int main() {

    // Initialize game state
GameState game;
generate_multi_floor_map(&game);
initialize_player(&game.player, MAP_WIDTH/2, MAP_HEIGHT/2);
    Room *first_room = &game.maps[0].rooms[0];
game.player.x = first_room->x + first_room->width/2;
game.player.y = first_room->y + first_room->height/2;
    
    // Initialize ncurses
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    start_color();
    
    // Generate password
    char correct_pass[PASSWORD_LENGTH+1];
    generate_random_password(correct_pass);
    
    int access_granted = 0;
    
    // Password check
    if(password_window(correct_pass)) {
        access_granted = 1;
    } else {
        // Math challenge fallback
        if(math_challenge()) {
            access_granted = 1;
        }
    }
    
    if(access_granted) {
        // Start game with first floor
        game.current_floor = 0;
        Room *first_room = &game.maps[0].rooms[0];
        game.player.x = first_room->x + 2;
        game.player.y = first_room->y + 2;
        
        // Start game loop
        game_menu(&game);
    } else {
        // Access denied
        printw("\n🚫 Access denied! Press any key to exit...");
        refresh();
        getch();
    }
    
    // Cleanup
    endwin();
    return 0;
}