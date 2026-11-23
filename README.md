# Minesweeper

This is Minesweeper in **C++20** with [raylib](https://www.raylib.com/). A pure C++20 core library (`minesweeper_core`) holds the rules. The game draws all graphics from shapes and text, so it has no asset files.

## Highlights

- **Classic rules**: Beginner (9x9, 10 mines), Intermediate (16x16, 40 mines), and Expert (30x16, 99 mines).
- **Safe first click**: The game places the mines after your first reveal. No mine is on that cell or next to it, so each game opens an area.
- **Flood fill and chords**: A click on a zero opens all of the connected cells. A click on a number with all of its flags in place opens its other neighbours.
- **Press preview**: Hold the mouse button to see the cells that a click or a chord opens. Move off the cell to cancel.
- **Keyboard play**: Move the cursor with the arrow keys or WASD. Reveal a cell with Space. Flag a cell with F.
- **Animations**: Reveals move out from the click in a ripple. After a loss, the mines go off one by one and the board shakes.
- **Records**: The game saves the best time and the win and loss counts for each difficulty to `~/.minesweeper.cfg`.

## Build in CLion

1. Open this folder in CLion.
2. Make sure that the computer has an internet connection. On the first load, CMake downloads raylib 5.5 with `FetchContent`.
3. Select the `minesweeper` run configuration.
4. Push Run.

To run the core test suite, select the `minesweeper_tests` run configuration and push Run.

## Build and Test from the Command Line

```bash
cmake -S . -B build
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./build/minesweeper
```

## Controls

| Action | Mouse | Keyboard |
| --- | --- | --- |
| Reveal a cell | Left click | `Space` or `Enter` |
| Put or remove a flag | Right click | `F` |
| Chord (open the cells around a number) | Left click or middle click on the number | `Space` on the number |
| Move the cursor | — | Arrow keys or `WASD` |
| Start a new game | Click the face | `R` or `F2` |
| Change the difficulty | Click a tab | `1`, `2`, or `3` |
| Quit | Close the window | `Esc` |

## Project Layout

- `src/board.*`, `src/difficulty.*`, `src/storage.*`: The pure C++20 core (`minesweeper_core`). It has no raylib dependency.
- `src/layout.*`, `src/theme.h`, `src/effects.*`, `src/renderer.*`, `src/game.*`, `src/main.cpp`: The raylib screen layout, the colours, the reveal animations, the draw code, and the game loop.
- `tests/minesweeper_tests.cpp`: The headless CTest suite. It tests the presets, first-click safety, the flood fill, flags, chords, losses, config limits, and the save file.
