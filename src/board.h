#ifndef MINESWEEPER_BOARD_H_
#define MINESWEEPER_BOARD_H_

#include <cstdint>
#include <optional>
#include <random>
#include <vector>

#include "difficulty.h"

/// @brief Everything that belongs to the Minesweeper game.
namespace minesweeper {

/// @brief A column/row coordinate on the board. `(0, 0)` is the top-left cell.
struct GridPoint {
  /// @brief Column index.
  int x = 0;
  /// @brief Row index (grows downwards).
  int y = 0;

  /// @brief Defaulted equality (C++20) compares `x` and `y` member by member.
  bool operator==(const GridPoint&) const = default;
};

/// @brief What the player can see on a single cell.
enum class CellState : std::uint8_t {
  /// @brief Covered; the player has not uncovered or flagged it.
  Hidden,
  /// @brief Uncovered; shows its adjacent-mine number (or blank for zero).
  Revealed,
  /// @brief Covered and marked with a flag, which blocks accidental reveals.
  Flagged,
};

/// @brief Where a game is in its lifecycle.
enum class GameStatus : std::uint8_t {
  /// @brief No cell revealed yet; mines are not placed until the first reveal.
  Ready,
  /// @brief Mines are placed and the clock is running.
  Playing,
  /// @brief Every safe cell is revealed.
  Won,
  /// @brief The player revealed a mine.
  Lost,
};

/// @brief One square on the board.
struct Cell {
  /// @brief True if a mine is hidden here.
  bool mine = false;
  /// @brief What the player currently sees.
  CellState state = CellState::Hidden;
  /// @brief Number of mines in the 8 surrounding cells (`0..8`).
  std::uint8_t adjacent = 0;
};

/// @brief The Minesweeper rules: mine placement, reveals, flags, and chords.
///
/// Mines are placed lazily on the first reveal so the first click is always
/// safe. When the board has room, the first click's 8 neighbours are kept
/// clear too, so the first click always opens an area rather than a lone
/// number. Placement uses a seeded `std::mt19937`, so a given seed and first
/// click always produce the same layout (handy for tests).
class Board {
 public:
  /// @brief Creates a Beginner board seeded from system entropy.
  Board();

  /// @brief Creates an empty board with a fixed seed.
  /// @param config Dimensions and mine count. Out-of-range values are clamped
  ///        to a playable board (at least 2x2, at most `width*height - 1`
  ///        mines).
  /// @param seed 32-bit PRNG seed used when mines are placed.
  Board(BoardConfig config, std::uint32_t seed);

  /// @brief Clears the board back to `GameStatus::Ready` with new settings.
  /// @param config Dimensions and mine count (clamped as in the constructor).
  /// @param seed 32-bit PRNG seed used when mines are placed.
  void Reset(BoardConfig config, std::uint32_t seed);

  /// @brief Replaces mine placement with an exact layout and starts play.
  ///
  /// Intended for tests and puzzles: it skips first-click safety, sets the
  /// mine count to the number of distinct in-bounds points, and moves the
  /// board straight to `GameStatus::Playing`.
  /// @param mines Cells that hold a mine. Out-of-bounds points are ignored.
  void SetMineLayout(const std::vector<GridPoint>& mines);

  /// @brief Uncovers the cell at `(x, y)`. Uncovering a zero cell flood-fills
  /// outwards through its neighbours, like the classic game.
  ///
  /// Does nothing on flagged or already revealed cells, outside the board,
  /// or once the game is over.
  /// @param x Column index.
  /// @param y Row index.
  /// @return Number of cells newly uncovered (0 if the cell was a mine).
  int Reveal(int x, int y);

  /// @brief "Chords" a revealed number: if as many neighbours are flagged as
  /// the number shows, every other covered neighbour is uncovered at once.
  ///
  /// A wrong flag means one of those neighbours is a mine, which loses the
  /// game exactly as revealing it by hand would.
  /// @param x Column index of a revealed numbered cell.
  /// @param y Row index of a revealed numbered cell.
  /// @return Number of cells newly uncovered.
  int Chord(int x, int y);

  /// @brief Places or removes a flag on a covered cell.
  /// @param x Column index.
  /// @param y Row index.
  /// @return True if the cell's state changed.
  bool ToggleFlag(int x, int y);

  /// @brief Returns true if `(x, y)` lies inside the board.
  /// @param x Column index.
  /// @param y Row index.
  /// @return True for a valid cell coordinate.
  bool InBounds(int x, int y) const;

  /// @brief Reads the cell at `(x, y)`.
  /// @param x Column index.
  /// @param y Row index.
  /// @return The cell, or a covered empty cell if out of bounds.
  const Cell& At(int x, int y) const;

  /// @brief Counts flagged cells among the 8 neighbours of `(x, y)`.
  /// @param x Column index.
  /// @param y Row index.
  /// @return Number of flagged neighbours (`0..8`).
  int CountAdjacentFlags(int x, int y) const;

  /// @brief Number of columns.
  int width() const { return config_.width; }
  /// @brief Number of rows.
  int height() const { return config_.height; }
  /// @brief Total number of mines on the board.
  int mine_count() const { return config_.mines; }
  /// @brief Current lifecycle stage.
  GameStatus status() const { return status_; }
  /// @brief True once the game is won or lost.
  bool is_over() const {
    return status_ == GameStatus::Won || status_ == GameStatus::Lost;
  }
  /// @brief Number of flags currently placed.
  int flags_placed() const { return flags_placed_; }
  /// @brief Mines minus flags, as shown on the mine counter. Goes negative
  /// if the player places more flags than there are mines.
  int mines_remaining() const { return config_.mines - flags_placed_; }
  /// @brief Number of safe cells uncovered so far.
  int revealed_count() const { return revealed_count_; }
  /// @brief The mine that ended the game, if it was lost.
  std::optional<GridPoint> exploded() const { return exploded_; }

 private:
  /// @brief Converts `(x, y)` to an index into `cells_`.
  /// @param x Column index (must be in bounds).
  /// @param y Row index (must be in bounds).
  /// @return Row-major index `y * width + x`.
  int Index(int x, int y) const { return y * config_.width + x; }

  /// @brief Scatters `config_.mines` mines, keeping `(safe_x, safe_y)` (and
  /// its neighbours when there is room) clear, then computes numbers.
  /// @param safe_x Column of the first reveal.
  /// @param safe_y Row of the first reveal.
  void PlaceMines(int safe_x, int safe_y);

  /// @brief Recomputes every cell's `adjacent` mine count.
  void ComputeAdjacency();

  /// @brief Uncovers `(x, y)` and, through zero cells, everything connected
  /// to it. Uses an explicit stack instead of recursion so a huge empty
  /// board can never overflow the call stack.
  /// @param x Column of a covered safe cell.
  /// @param y Row of a covered safe cell.
  /// @return Number of cells uncovered.
  int FloodReveal(int x, int y);

  /// @brief Ends the game as lost with `(x, y)` as the detonated mine.
  /// @param x Column of the mine.
  /// @param y Row of the mine.
  void Detonate(int x, int y);

  /// @brief Moves to `GameStatus::Won` when every safe cell is uncovered,
  /// and flags every remaining mine so the counter reads zero.
  void CheckForWin();

  /// @brief Dimensions and mine count after clamping.
  BoardConfig config_{};
  /// @brief Flat row-major cell storage: `cells_[y * width + x]`.
  std::vector<Cell> cells_;
  /// @brief Current lifecycle stage.
  GameStatus status_ = GameStatus::Ready;
  /// @brief Number of flags currently placed.
  int flags_placed_ = 0;
  /// @brief Number of safe cells uncovered so far.
  int revealed_count_ = 0;
  /// @brief The mine that ended the game, if any.
  std::optional<GridPoint> exploded_;
  /// @brief Mersenne Twister PRNG used for mine placement.
  std::mt19937 rng_;
};

}  // namespace minesweeper

#endif  // MINESWEEPER_BOARD_H_
