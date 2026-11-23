#include <raylib.h>

#include "game.h"
#include "screen.h"

/// @brief Opens the window, runs the game loop, and cleans up raylib
/// resources in reverse initialization order.
/// @return 0 on a normal exit.
int main() {
  // Ask for 4x multisample anti-aliasing before the OpenGL context exists, so
  // rounded tiles, circles, and lines have smooth edges.
  SetConfigFlags(FLAG_MSAA_4X_HINT);
  InitWindow(minesweeper::kScreenWidth, minesweeper::kScreenHeight,
             "Minesweeper");
  SetTargetFPS(60);

  // Explicit inner scope (RAII): `game` is destroyed at the closing brace, so
  // its destructor saves records while raylib is still initialized, before
  // CloseWindow() tears the window down.
  {
    minesweeper::Game game;
    // raylib's default exit key is Esc, so Esc (or the window's close
    // button) ends the loop.
    while (!WindowShouldClose()) {
      game.Update(GetFrameTime());

      BeginDrawing();
      game.Draw();
      EndDrawing();
    }
  }

  CloseWindow();
  return 0;
}
