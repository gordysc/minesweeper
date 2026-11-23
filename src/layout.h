#ifndef MINESWEEPER_LAYOUT_H_
#define MINESWEEPER_LAYOUT_H_

#include <raylib.h>

#include <optional>

#include "board.h"
#include "difficulty.h"

/// @brief Everything that belongs to the Minesweeper game.
namespace minesweeper {

/// @brief Where the board's cells sit on screen.
///
/// `Game` uses it to turn mouse positions into cells, and `Renderer` uses
/// the same values to draw, so input and graphics can never disagree.
struct BoardLayout {
  /// @brief Left edge of column 0 in pixels.
  float origin_x = 0.0f;
  /// @brief Top edge of row 0 in pixels.
  float origin_y = 0.0f;
  /// @brief Width and height of one cell in pixels.
  float cell_size = 32.0f;
  /// @brief Number of columns on the board.
  int columns = 0;
  /// @brief Number of rows on the board.
  int rows = 0;

  /// @brief Returns the rectangle covering every cell.
  /// @return Board bounds in pixels.
  Rectangle Bounds() const;

  /// @brief Returns the screen rectangle of cell `(x, y)`.
  /// @param x Column index.
  /// @param y Row index.
  /// @return Cell bounds in pixels (no gap removed).
  Rectangle CellRect(int x, int y) const;

  /// @brief Finds the cell under `point`.
  /// @param point Screen position, usually the mouse.
  /// @return The cell, or `std::nullopt` when `point` is off the board.
  std::optional<GridPoint> CellAt(Vector2 point) const;
};

/// @brief Fits a `columns` x `rows` board into the board area, as large as
/// fits (capped so Beginner tiles don't become huge) and centred.
/// @param columns Number of columns.
/// @param rows Number of rows.
/// @return Layout for that board.
BoardLayout ComputeBoardLayout(int columns, int rows);

/// @brief Panel across the top holding the counter, face, and timer.
/// @return Header bounds in pixels.
Rectangle HeaderRect();

/// @brief Mine counter box at the left of the header.
/// @return Counter bounds in pixels.
Rectangle MineCounterRect();

/// @brief Timer box at the right of the header.
/// @return Timer bounds in pixels.
Rectangle TimerRect();

/// @brief Reset button in the centre of the header.
/// @return Button bounds in pixels.
Rectangle FaceButtonRect();

/// @brief Clickable tab that switches to `difficulty`.
/// @param difficulty Tab to locate.
/// @return Tab bounds in pixels.
Rectangle DifficultyTabRect(Difficulty difficulty);

/// @brief Y coordinate of the hint / result line below the board.
/// @return Text baseline area top in pixels.
float FooterY();

}  // namespace minesweeper

#endif  // MINESWEEPER_LAYOUT_H_
