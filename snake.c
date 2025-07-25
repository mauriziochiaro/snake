#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <windows.h>

#define CTRL_KEY(k) ((k) & 0x1F) 
#define ESC "\x1b"

#define GRID_ROWS 28
#define GRID_COLS 120
#define GRID_AREA (GRID_ROWS * GRID_COLS)

#define SNAKE_HEAD_CHAR '0'
#define SNAKE_BODY_CHAR 'o'
#define ENEMY_CHAR '*'
#define FOOD_CHAR '$'
#define EMPTY_CHAR ' '
#define WALL_CHAR '`'

//#define SLEEPING_TIME 60 // ms

char screen_buffer[(GRID_COLS + 1) * GRID_ROWS + 1];

enum direction {
    UP = 0,
    DOWN,
    LEFT,
    RIGHT
};

enum EditorKey {
    ARROW_LEFT = 1000,
    ARROW_RIGHT,
    ARROW_UP,
    ARROW_DOWN
};

/* Data structures */
typedef struct {
    DWORD orig_mode;    // Original console mode
    HANDLE hStdin;      // Console input handle
    HANDLE hStdout;     // Console output handle
} TerminalConfig;

typedef struct _SnakeSegment {
    int x, y;                    // Position of the segment
    struct _SnakeSegment *next;  // Pointer to the next segment
} SnakeSegment;

typedef struct {
    int x, y;                    // Position of the enemy
} Enemy;

typedef struct {
    SnakeSegment *head;        // Pointer to the head of the snake
    Enemy *enemy;              // Pointer to the enemy
    int food_x, food_y;        // Position of the food
    int score;                 // Player's score
    enum direction dir;        // Direction of the snake
    int game_over;             // Game over flag
    //int speed;                 // Speed of the snake
} GameState;

/* Global state */
TerminalConfig terminal_config;
GameState game_state;
int g_speed = 60; // default speed in milliseconds
//int g_level = 1;  // default level

/* Terminal config API */
void clearScreen() {
    DWORD written;
    WriteConsole(terminal_config.hStdout, ESC "[2J", 4, &written, NULL);  // Clear screen
}

void moveCursorToTopLeft() {
    DWORD written;
    WriteConsole(terminal_config.hStdout, ESC "[H", 3, &written, NULL);  // Move cursor to top-left
}

void hideCursor() {
    DWORD written;
    WriteConsole(terminal_config.hStdout, ESC "[?25l", 6, &written, NULL);  // Hide cursor
}

void showCursor() {
    DWORD written;
    WriteConsole(terminal_config.hStdout, ESC "[?25h", 6, &written, NULL);  // Show cursor
}

/* Terminal API */
void die(const char *s) {
    clearScreen();
    moveCursorToTopLeft();
    perror(s);
    exit(1);
}

void disableRawMode() {
    if (!SetConsoleMode(terminal_config.hStdin, terminal_config.orig_mode)) die("SetConsoleMode");
}

void enableRawMode() {
    terminal_config.hStdin = GetStdHandle(STD_INPUT_HANDLE);
    terminal_config.hStdout = GetStdHandle(STD_OUTPUT_HANDLE);

    if (terminal_config.hStdin == INVALID_HANDLE_VALUE || terminal_config.hStdout == INVALID_HANDLE_VALUE)
        die("GetStdHandle");

    if (!GetConsoleMode(terminal_config.hStdin, &terminal_config.orig_mode)) die("GetConsoleMode");

    atexit(disableRawMode);

    DWORD mode = terminal_config.orig_mode;
    mode &= ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT);
    mode |= (ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT);
    if (!SetConsoleMode(terminal_config.hStdin, mode)) die("SetConsoleMode (input)");

    // enable ANSI escape sequences on the output
    DWORD outMode = 0;
    if (!GetConsoleMode(terminal_config.hStdout, &outMode)) die("GetConsoleMode (output)");
    outMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    if (!SetConsoleMode(terminal_config.hStdout, outMode)) die("SetConsoleMode (output)");
}

// key events processing
void process_key_events() {
    INPUT_RECORD ir;
    DWORD events;
    DWORD read;

    // check if there is input available
    GetNumberOfConsoleInputEvents(terminal_config.hStdin, &events);
    if (events == 0) return; // No input, exit immediately and continue the game

    // if there is input, read it
    if (!ReadConsoleInput(terminal_config.hStdin, &ir, 1, &read) || read != 1) return;

    int c = -1; 

    if (ir.EventType == KEY_EVENT && ir.Event.KeyEvent.bKeyDown) {
        WORD vk = ir.Event.KeyEvent.wVirtualKeyCode;
        CHAR ch = ir.Event.KeyEvent.uChar.AsciiChar;

        switch (vk) {
            case VK_LEFT:  c = ARROW_LEFT; break;
            case VK_UP:    c = ARROW_UP; break;
            case VK_RIGHT: c = ARROW_RIGHT; break;
            case VK_DOWN:  c = ARROW_DOWN; break;
            default: c = ch; break;
        }
    }

    switch (c) {
        case CTRL_KEY('q'):
            showCursor();
            clearScreen();
            moveCursorToTopLeft();
			exit(0);
            break;

        // update snake direction (prevent 180-degree turns)
        case ARROW_LEFT:
            if (game_state.dir != RIGHT) game_state.dir = LEFT;
			break;
        case ARROW_RIGHT:
            if (game_state.dir != LEFT) game_state.dir = RIGHT;
			break;
        case ARROW_UP:
            if (game_state.dir != DOWN) game_state.dir = UP;
			break;
        case ARROW_DOWN:
            if (game_state.dir != UP) game_state.dir = DOWN;
			break;
    }
}

/* Game API */
/* convert coordinates to index
NOTE: this function has a wrap-around logic that is not
utilised for the snake because hitting the walls cause death */
int coordinates_to_index(int row, int col) {
	if (row < 0) row = row + GRID_ROWS;
	if (col < 0) col = col + GRID_COLS;
	if (row >= GRID_ROWS) row = row % GRID_ROWS;
	if (col >= GRID_COLS) col = col % GRID_COLS;
	return col+GRID_COLS*row;
}

// set and get cell state in the grid
void set_cell_state(char *grid, int row, int col, char state) {
    int index = coordinates_to_index(row, col);
    if (index >= 0 && index < GRID_AREA) {
        grid[index] = state;
    }
}

// get the state of a cell in the grid
char get_cell_state(char *grid, int row, int col) {
    int index = coordinates_to_index(row, col);
    if (index >= 0 && index < GRID_AREA) {
        return grid[index];
    }
    return EMPTY_CHAR;
}

// draw the game state to the console
void draw_game(GameState *state) {
    moveCursorToTopLeft();

    DWORD written;

    char temp_buffer[(GRID_ROWS * (GRID_COLS + 1)) + 50]; // +50 for score
    char *p = temp_buffer;

    *p = '\n';
    p++;
    for (int i = 0; i < GRID_ROWS; i++) {
        memcpy(p, screen_buffer + (i * GRID_COLS), GRID_COLS);
        p += GRID_COLS;
        *p = '\n';
        p++;
    }

    // p += snprintf(p, 100, "Score: %d - Level: %d\n", state->score, g_level);
    p += snprintf(p, 100, "Score: %d\n", state->score);
    WriteConsole(terminal_config.hStdout, temp_buffer, p - temp_buffer, &written, NULL);
}

// place food in a random empty cell
void place_food(GameState *state) {
    int x, y;
    do {
        x = rand() % (GRID_COLS - 2) + 1;
        y = rand() % (GRID_ROWS - 2) + 1;
    } while (get_cell_state(screen_buffer, y, x) != EMPTY_CHAR && 
             get_cell_state(screen_buffer, y, x) != SNAKE_BODY_CHAR && 
             get_cell_state(screen_buffer, y, x) != SNAKE_HEAD_CHAR &&
             get_cell_state(screen_buffer, y, x) != ENEMY_CHAR);

    state->food_x = x;
    state->food_y = y;
    set_cell_state(screen_buffer, y, x, FOOD_CHAR);
}

// place enemy in a random empty cell every 5 points scored
void place_enemy(GameState *state) {
    if (state->enemy != NULL) {
        // set the previous enemy cell to empty (commenting this out to keep the enemy on the screen)
        // set_cell_state(screen_buffer, state->enemy->y, state->enemy->x, EMPTY_CHAR); 
        // NOTE: I should manage multiple enemies!!

        // free the previous enemy
        free(state->enemy); // free existing enemy
    }
    Enemy *enemy = malloc(sizeof(Enemy));
    if (!enemy) die("malloc");
    state->enemy = enemy;
    // place enemy in a random empty cell
    do {
        enemy->x = rand() % (GRID_COLS - 2) + 1;
        enemy->y = rand() % (GRID_ROWS - 2) + 1;
    } while (get_cell_state(screen_buffer, state->enemy->y, state->enemy->x) != EMPTY_CHAR && 
             get_cell_state(screen_buffer, state->enemy->y, state->enemy->x) != SNAKE_BODY_CHAR && 
             get_cell_state(screen_buffer, state->enemy->y, state->enemy->x) != SNAKE_HEAD_CHAR &&
             get_cell_state(screen_buffer, state->enemy->y, state->enemy->x) != FOOD_CHAR &&
             get_cell_state(screen_buffer, state->enemy->y, state->enemy->x) != ENEMY_CHAR);

    set_cell_state(screen_buffer, enemy->y, enemy->x, ENEMY_CHAR);
}

// initialize the snake at the center of the grid
void init_snake(GameState *state) {
    SnakeSegment *segment = malloc(sizeof(SnakeSegment));
    if (!segment) die("malloc");
    segment->x = GRID_COLS / 2;
    segment->y = GRID_ROWS / 2;
    segment->next = state->head;
    state->head = segment;

    set_cell_state(screen_buffer, segment->y, segment->x, SNAKE_HEAD_CHAR);
}

// Initialize the grid with walls and empty spaces
void init_grid() {
    for (int i = 0; i < GRID_ROWS; i++) {
        for (int j = 0; j < GRID_COLS; j++) {
            set_cell_state(screen_buffer, i, j, EMPTY_CHAR);
            if (i == 0 || i == GRID_ROWS - 1 || j == 0 || j == GRID_COLS - 1) {
                set_cell_state(screen_buffer, i, j, WALL_CHAR);
            }
        }
    }
}

// initialize the game state
void init_game(GameState *state) {
    // init terminal
    enableRawMode(); // has to be non-blocking
    srand(time(NULL)); 
    hideCursor();

    // draw board
    init_grid();

    // init the game state
    state->head = NULL;
    state->enemy = NULL;
    state->food_x = -1;
    state->food_y = -1;
    state->score = 0;
    state->dir = -1; // no initial direction
    state->game_over = 0;

    init_snake(state);
    place_food(state);
}

void compute_enemy_position(GameState *state) {
    if (!state->enemy) {
        return;
    }

    // delete the enemy from its current position
    set_cell_state(screen_buffer, state->enemy->y, state->enemy->x, EMPTY_CHAR);

    // down movement logic for the enemy
    int new_y = state->enemy->y + 1;
    int new_x = state->enemy->x;

    // wrap-around and collision logic
    if (new_y >= GRID_ROWS - 1) {
        new_y = 1;
        //new_x = rand() % (GRID_COLS - 2) + 1; // random x pos??
    }

    // check if the NEW position collides with the snake
    char cell_content = get_cell_state(screen_buffer, new_y, new_x);
    if (cell_content == SNAKE_BODY_CHAR || cell_content == SNAKE_HEAD_CHAR) {
        state->game_over = 1;
        return;
    }

    // no collision, update the enemy's coordinates
    state->enemy->y = new_y;
    state->enemy->x = new_x;

    // update the screen buffer with the new enemy position
    set_cell_state(screen_buffer, state->enemy->y, state->enemy->x, ENEMY_CHAR);
}

// compute the new position of the snake based on its direction
void compute_snake_position(GameState *state) {
    // waiting for the first input
    if (state->dir == -1) {
        return;
    }
    // new head segment
    SnakeSegment *new_head = malloc(sizeof(SnakeSegment));
    if (!new_head) die("malloc");

    // new head position based on current direction
    new_head->x = state->head->x;
    new_head->y = state->head->y;
    switch (state->dir) {
        case UP:    new_head->y--; break;
        case DOWN:  new_head->y++; break;
        case LEFT:  new_head->x--; break;
        case RIGHT: new_head->x++; break;
    }

    // check for collisions
    char cell_content = get_cell_state(screen_buffer, new_head->y, new_head->x);
    if (cell_content == WALL_CHAR || cell_content == SNAKE_BODY_CHAR || cell_content == ENEMY_CHAR) {
        state->game_over = 1;
        free(new_head);
        return;
    }

    // update the new head position
    new_head->next = state->head;
    state->head = new_head;

    // update the screen buffer
    set_cell_state(screen_buffer, state->head->y, state->head->x, SNAKE_HEAD_CHAR);
    if (state->head->next) {
        set_cell_state(screen_buffer, state->head->next->y, state->head->next->x, SNAKE_BODY_CHAR);
    }

    // check if the snake has eaten
    if (new_head->x == state->food_x && new_head->y == state->food_y) {
        state->score++;
        place_food(state); // snake grows (we don't remove the tail)
        // place an enemy
        if (state->score % 1 == 0) { // adding anemies and increasing speed every 1 point (seems more for fun)
            place_enemy(state);
            g_speed > 0 ? g_speed-- : 1;
            //g_level++;
        }
    } else {
        // if it hasn't eaten, remove the tail segment
        SnakeSegment *tail = state->head;
        SnakeSegment *prev = NULL;
        while (tail->next != NULL) {
            prev = tail;
            tail = tail->next;
        }
        // remove the tail segment from the screen buffer
        if (prev) {
            prev->next = NULL;
            set_cell_state(screen_buffer, tail->y, tail->x, EMPTY_CHAR);
            free(tail);
        }
    }
}

void compute_game_state(GameState *state) {
    compute_snake_position(&game_state);
    if (game_state.game_over) return;
    compute_enemy_position(&game_state);
    if (game_state.game_over) return;
}

void end_game(GameState *state) {
    char gameOverMsg[100];
    DWORD written;
    int len = sprintf(gameOverMsg, "\n\n        GAME OVER! Final Score: %d\n\n", game_state.score);
    WriteConsole(terminal_config.hStdout, gameOverMsg, len, &written, NULL);
    showCursor();
}

void free_snake(SnakeSegment *head) {
    SnakeSegment *current = head;
    SnakeSegment *next;
    while (current != NULL) {
        next = current->next;
        free(current);
        current = next;
    }
}

int main(int argc, char *argv[]) {
    for (int i = 1; i < argc; i++) {
        // read -speed value (i.e --speed 100)
        if (!strcmp(argv[i], "--speed") && i + 1 < argc) {
            int speed = atoi(argv[++i]);
            //printf("Setting speed to %d ms\n", speed);
            if (speed > 0) {
                g_speed = speed;
            } else {
                printf("Invalid speed value: %s\n", argv[i]);
            }
        }
        // --help
        else if (!strcmp(argv[i], "--help")) {
            printf("Usage: snake [--speed <milliseconds>]\n");
            printf("Options:\n");
            printf("  --speed <milliseconds>  Set the speed of the snake (default is 60 ms)\n");
            return 0;
        } else {
            printf("Unknown option: %s\n", argv[i]);
            return 1;
        }
    }

    init_game(&game_state);

    // game loop
    while (!game_state.game_over) {
        process_key_events();
        compute_game_state(&game_state);
        draw_game(&game_state);
        Sleep(g_speed);
    }
    end_game(&game_state);
    free_snake(game_state.head);
    free(game_state.enemy);
    return 0;
}