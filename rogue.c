#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <ncurses.h>
#include <time.h>
#include <string.h>
#include <ctype.h>

#define MAX_USERS 100
#define USERNAME_LENGTH 50
#define PASSWORD_LENGTH 50
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

typedef struct {
    char username[USERNAME_LENGTH];
    char password[PASSWORD_LENGTH];
    char email[EMAIL_LENGTH];
} User;

typedef struct {
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
} Player;

typedef struct {
    int x, y;
    char symbol;
    char type;
    int value;
    int ammo;
} Item;

typedef struct {
    int x, y;
    int width, height;
} Room;

typedef struct {
    int x, y;
    char symbol;
    int health;
    int damage;
    int speed;
    int is_boss;
    char type; 
    int room_index;
} Enemy;

typedef struct {
    int x, y;
    char symbol;
    int damage;
} Fire;

typedef struct {
    int x, y;
    char symbol;
    int dx, dy;
} Bullet;

typedef struct {
    int x, y;
    char symbol;
    int is_poisonous; 
} Food;

typedef struct {
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
} Map;

typedef struct {
    int x, y;
} ExitPoint;

User users[MAX_USERS];
int user_count = 0;

ExitPoint exitPoints[MAX_EXIT_POINTS] = {
    {MAP_WIDTH / 2, 1},
    {MAP_WIDTH / 2, MAP_HEIGHT - 2},
    {1, MAP_HEIGHT / 2},
    {MAP_WIDTH - 2, MAP_HEIGHT / 2}
};

int is_valid_email(const char *email) {
    const char *at = strchr(email, '@');
    const char *dot = strchr(email, '.');
    return at && dot && at < dot;
}

int is_valid_password(const char *password) {
    int has_upper = 0, has_lower = 0, has_digit = 0;
    for (int i = 0; password[i]; i++) {
        if (isupper(password[i])) has_upper = 1;
        if (islower(password[i])) has_lower = 1;
        if (isdigit(password[i])) has_digit = 1;
    }
    return has_upper && has_lower && has_digit && strlen(password) >= 7;
}

int is_username_taken(const char *username) {
    for (int i = 0; i < user_count; i++) {
        if (strcmp(users[i].username, username) == 0) {
            return 1;
        }
    }
    return 0;
}

void create_new_user() {
    User new_user;
    printf("Enter username: ");
    scanf("%s", new_user.username);
    if (is_username_taken(new_user.username)) {
        printf("Username is already taken.\n");
        return;
    }

    printf("Enter password: ");
    scanf("%s", new_user.password);
    if (!is_valid_password(new_user.password)) {
        printf("Password must be at least 7 characters long and contain at least one uppercase letter, one lowercase letter, and one digit.\n");
        return;
    }

    printf("Enter email: ");
    scanf("%s", new_user.email);
    if (!is_valid_email(new_user.email)) {
        printf("Invalid email format.\n");
        return;
    }

    users[user_count++] = new_user;
    printf("User created successfully!\n");
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
    enemy->symbol = type == 'S' ? 'S' : 'E';
    enemy->health = type == 'S' ? 70 : 50;
    enemy->damage = type == 'S' ? 15 : 10;
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

    for (int i = 0; i < MAX_ENEMIES; i++) {
        int roomIndex = rand() % roomCount;
        Room *room = &map->rooms[roomIndex];
        char type = 'E';
        if (rand() % 5 == 0) type = 'S'; 
        if (i == 0 && map->level % 3 == 0) type = 'B'; 
        initialize_enemy(&map->enemies[i], 
                         room->x + 1 + rand() % (room->width - 2), 
                         room->y + 1 + rand() % (room->height - 2),
                         roomIndex,
                         type);
        map->enemy_count++;
    }

    for (int i = 0; i < MAX_FIRES; i++) {
        int roomIndex = rand() % roomCount;
        Room *room = &map->rooms[roomIndex];
        initialize_fire(&map->fires[i], 
                        room->x + 1 + rand() % (room->width - 2), 
                        room->y + 1 + rand() % (room->height - 2));
        map->fire_count++;
    }

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
    if (rand() % 100 < 30) {
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
    } else {
        printw("Out of ammo!\n");
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
        printw("Dragon Boss activated! Defeat it to win!\n");
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
                        printw("You found %d gold!\n", map->items[i].value);
                    } else if (map->items[i].type == 'H') {
                        player->health += map->items[i].value;
                        printw("Health +%d!\n", map->items[i].value);
                    } else if (map->items[i].type == 'W') {
                        player->weapon_power += map->items[i].value;
                        player->ammo += map->items[i].ammo;
                        printw("Weapon upgraded! Power +%d | Ammo +%d\n",
                              map->items[i].value, map->items[i].ammo);
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

            print_map_with_player(map, player);
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
                        ExitPoint ep = exitPoints[i];
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

void print_map_with_player(Map *map, Player *player) {
    clear();
    int vision_radius = 4;
    
    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            int dx = abs(x - player->x);
            int dy = abs(y - player->y);
            
            if (map->show_full_map || (dx <= vision_radius && dy <= vision_radius)) {
                if (x == player->x && y == player->y) {
                    attron(COLOR_PAIR(6));
                    printw("@");
                    attroff(COLOR_PAIR(6));
                } else {
                    int printed = 0;

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

                    if (!printed) {
                        for (int i = 0; i < map->item_count; i++) {
                            if (map->items[i].x == x && map->items[i].y == y) {
                                attron(COLOR_PAIR(4));
                                printw("%c", map->items[i].symbol);
                                attroff(COLOR_PAIR(4));
                                printed = 1;
                                break;
                            }
                        }
                    }

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
    
    printw("Health: %d | Gold: %d | Score: %d | Level: %d | Ghost: %s | Ammo: %d | Cheat: %s\n", 
           player->health, player->gold, player->score, map->level,
           player->ghost_mode ? "ON" : "OFF", player->ammo,
           player->cheat_mode ? "ON" : "OFF");
    refresh();
}

void game_menu(Map *map, Player *player) {
    initscr();
    start_color();
    init_pair(1, COLOR_RED, COLOR_BLACK);
    init_pair(2, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(3, COLOR_YELLOW, COLOR_BLACK);
    init_pair(4, COLOR_CYAN, COLOR_BLACK);
    init_pair(5, COLOR_GREEN, COLOR_BLACK);
    init_pair(6, COLOR_WHITE, COLOR_BLACK);
    init_pair(7, COLOR_GREEN, COLOR_BLACK); 
    init_pair(8, COLOR_RED, COLOR_BLACK); 
    init_pair(9, COLOR_BLUE, COLOR_BLACK); 
    cbreak();
    noecho();
    keypad(stdscr, TRUE);

    print_map_with_player(map, player);

    int ch;
    int speed = 1;
    while ((ch = getch()) != 'q') {
        if (ch == 'v') {
            speed = 3;
        } else if (ch == 'd' || ch == 'D') {
            player->ghost_mode = !player->ghost_mode;
            printw("Ghost mode %s!\n", player->ghost_mode ? "ON" : "OFF");
        } else if (ch == 'f') {
            fire_weapon(player, map);
        } else if (ch == 'j' || ch == 'J') {
            player->cheat_mode = !player->cheat_mode;
            printw("Cheat mode %s!\n", player->cheat_mode ? "ON" : "OFF");
        } else {
            speed = 1;
        }

        switch (ch) {
            case KEY_UP: move_player(player, map, 0, -1, speed); break;
            case KEY_DOWN: move_player(player, map, 0, 1, speed); break;
            case KEY_LEFT: move_player(player, map, -1, 0, speed); break;
            case KEY_RIGHT: move_player(player, map, 1, 0, speed); break;
            case 's': map->show_full_map = !map->show_full_map; break;
        }

        for (int i = 0; i < map->enemy_count; i++) {
            if (map->enemies[i].type == 'B') {
                move_boss_towards_player(&map->enemies[i], player);
            } else if (map->enemies[i].type == 'S') {
                move_toxic_enemy(&map->enemies[i], player, map);
            } else {
                move_enemy_randomly(&map->enemies[i], map);
            }
        }

        for (int i = 0; i < map->enemy_count; i++) {
            if (map->enemies[i].is_boss && map->fire_count < MAX_FIRES) {
                if (rand() % 100 < 20) {
                    for (int dx = -1; dx <= 1; dx++) {
                        for (int dy = -1; dy <= 1; dy++) {
                            int fx = map->enemies[i].x + dx;
                            int fy = map->enemies[i].y + dy;
                            if (fx >= 0 && fx < MAP_WIDTH && 
                                fy >= 0 && fy < MAP_HEIGHT) {
                                initialize_fire(&map->fires[map->fire_count++], fx, fy);
                            }
                        }
                    }
                    printw("BOSS breathes fire!\n");
                }
            }
        }
        
        print_map_with_player(map, player);

        int boss_defeated = 0;
        for (int i = 0; i < map->enemy_count; i++) {
            if (map->enemies[i].is_boss && map->enemies[i].health <= 0) {
                boss_defeated = 1;
                break;
            }
        }

        if (boss_defeated) {
            printw("YOU DEFEATED THE BOSS! VICTORY!\nPress any key to return to main menu...");
            refresh();
            getch();
            break;
        }

        if (player->health <= 0) {
            printw("GAME OVER! Press any key...\n");
            refresh();
            getch();
            break;
        }
    }

    endwin();
}

int main() {
    Map map;
    Player player;

    generate_random_map(&map);
    initialize_player(&player, 1, 1);
    ensure_player_on_floor(&player, &map);

    int choice;
    while (1) {
        printf("1. Create new user\n");
        printf("2. Start game\n");
        printf("3. Exit\n");
        printf("Enter your choice: ");
        scanf("%d", &choice);

        
        if (choice == 1) {
            create_new_user();
        } else if (choice == 2) {
            game_menu(&map, &player);
        } else if (choice == 3) {
            break;
        } else {
            printf("Invalid choice. Please try again.\n");
        }
    }

    return 0;
}  