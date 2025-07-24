# Terminal Snake

A simple ~400 lines implementation of the classic Snake game that runs entirely within the Windows Command Prompt. This project is written in pure C and uses the Windows Console API for direct terminal manipulation, input handling, and rendering. No external graphics libraries needed.

## Features

*   **Classic Gameplay**: Control a growing snake to eat food and increase your score.
*   **Dynamic Enemies**: Avoid the deadly 'X' enemies that appear as your score increases!
*   **Direct Terminal Rendering**: No external graphics libraries needed. The game is rendered entirely with text characters using ANSI escape codes.
*   **Raw Mode Input**: The terminal is set to "raw mode" to process key presses instantly without needing to press Enter.
*   **Collision Detection**: The game ends if the snake collides with the walls, itself, or an enemy.

## Built With

*   **Language**: C
*   **API**: Windows Console API
*   **Platform**: Windows

## Getting Started

Follow these instructions to get a copy of the project up and running on your local machine.

### Prerequisites

You will need a C compiler configured for Windows. Common options include:
*   **MinGW-w64** (providing `gcc`)
*   **Microsoft Visual C++ compiler** (`cl.exe`) from the Visual Studio Build Tools.

### Compilation

1.  Clone the repository or download the source code.
2.  Open a command prompt (like the Developer Command Prompt for VS, or a terminal where `gcc` is in your PATH).
3.  Navigate to the project directory.
4.  Compile the source file (`snake.c`).

**Using GCC (MinGW):**
```sh
gcc snake.c -o snake.exe -Wall
```

**Using Microsoft C Compiler (cl.exe):**
```sh
cl snake.c
```

This will create an executable file named `snake.exe`.

### How to Play

1.  Run the compiled executable from your terminal:
    ```sh
    .\snake.exe
    ```2.  The game will start immediately.

**Controls:**
*   **Arrow Keys** (`↑`, `↓`, `←`, `→`): Change the snake's direction.
*   **CTRL + Q**: Quit the game at any time.

## Code Overview

The entire game is contained within a single `snake.c` file and is structured around a few key concepts:

*   **Terminal Management**:
    *   `enableRawMode()`: Uses Windows API functions (`GetConsoleMode`, `SetConsoleMode`) to put the terminal into a non-blocking, non-echoing "raw" state. It also enables ANSI escape sequence processing.
    *   `disableRawMode()`: Restores the terminal to its original settings upon exit. This is registered with `atexit()` to ensure it runs even if the program crashes.
    *   A small API (`hideCursor()`, `showCursor()`, `clearScreen()`) provides abstractions for common terminal operations.

*   **Game Loop**:
    *   The `main()` function contains the core game loop (`while (!game_state.game_over)`).
    *   In each iteration, the loop:
        1.  Processes user input (`process_key_events()`).
        2.  Updates the snake's position and checks for collisions (`compute_snake_position()`).
        3.  Redraws the entire game grid on the screen (`draw_game()`).
        4.  Pauses for a short duration (`Sleep()`) to control the game speed.

*   **Data Structures**:
    *   `GameState`: A central struct that holds all state information: the snake, food position, score, and game-over flag.
    *   `SnakeSegment`: The snake itself is implemented as a **singly-linked list** of `SnakeSegment` structs. This allows the snake to grow dynamically when it eats food.

## License

MIT