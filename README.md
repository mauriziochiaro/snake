# Terminal Snake

An implementation of the classic Snake game that runs entirely within the Windows Command Prompt. This project is written in pure C and uses the Windows Console API for direct terminal manipulation, input handling, and rendering. No external graphics libraries needed.

<img width="1740" height="937" alt="image" src="https://github.com/user-attachments/assets/952bf854-5796-45a1-a32c-03b68cc64980" />

## Features

*   **Dynamic Enemies**: Avoid the deadly '*' enemies that appear as your score increases!
*   **Direct Terminal Rendering**: No external graphics libraries needed. The game is rendered entirely with text characters using ANSI escape codes.
*   **Raw Mode Input**: The terminal is set to "raw mode" to process key presses instantly without needing to press Enter.
*   **Collision Detection**: The game ends if the snake collides with the walls, itself, or an enemy.

## Built With

*   **Language**: C
*   **API**: Windows Console API
*   **Platform**: Windows

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
    ```

**Controls:**
*   **Arrow Keys** (`↑`, `↓`, `←`, `→`): Change the snake's direction.
*   **CTRL + Q**: Quit the game at any time.

## License

MIT
