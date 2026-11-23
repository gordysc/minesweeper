#ifndef MINESWEEPER_EFFECTS_H_
#define MINESWEEPER_EFFECTS_H_

#include <cstdint>
#include <vector>

#include "board.h"

/// @brief Everything that belongs to the Minesweeper game.
namespace minesweeper {

/// @brief Times the "pop" animation of each cell as it becomes visible.
///
/// After every move, `Sync()` compares the board with what it saw last time.
/// Each newly visible cell gets a start time delayed by its distance from the
/// click, so a flood fill ripples outwards and, on a loss, the mines go off
/// one after another starting at the one you hit.
///
/// It never changes the Board; it only reads it. That keeps animation purely
/// cosmetic, so the rules in `minesweeper_core` stay simple and testable.
class RevealAnimator {
 public:
  /// @brief Forgets all animations and sizes storage for a new board.
  /// @param columns Board width.
  /// @param rows Board height.
  void Reset(int columns, int rows);

  /// @brief Starts animations for cells that became visible since the last
  /// call.
  /// @param board Board after the move (read-only).
  /// @param origin Cell the player acted on; ripples start here.
  /// @param now Current animation clock in seconds.
  void Sync(const Board& board, GridPoint origin, float now);

  /// @brief Returns how far cell `(x, y)` is through its pop-in animation.
  /// @param x Column index.
  /// @param y Row index.
  /// @param now Current animation clock in seconds.
  /// @return `0.0` (not started) to `1.0` (finished or never animated).
  float Progress(int x, int y, float now) const;

 private:
  /// @brief Board width, for turning `(x, y)` into an index.
  int columns_ = 0;
  /// @brief Animation start time per cell. Cells that never animated keep a
  /// time far in the past, so `Progress()` reports them as finished.
  std::vector<float> start_times_;
  /// @brief Whether each cell was already visible at the previous `Sync()`.
  ///
  /// `std::uint8_t` instead of `bool` on purpose: `std::vector<bool>` is a
  /// special bit-packed container whose elements aren't real `bool`s (you
  /// can't take a `bool&` to one), which surprises people.
  std::vector<std::uint8_t> was_visible_;
  /// @brief Each cell's state at the previous `Sync()`, so flags the player
  /// placed by hand don't replay their pop-in when a win shows every flag.
  std::vector<CellState> last_states_;
};

}  // namespace minesweeper

#endif  // MINESWEEPER_EFFECTS_H_
