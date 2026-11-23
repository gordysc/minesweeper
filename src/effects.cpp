#include "effects.h"

#include <algorithm>
#include <cmath>

/// @brief Everything that belongs to the Minesweeper game.
namespace minesweeper {

/// @brief Animation timings internal to `effects.cpp`.
namespace {

/// @brief A start time so far in the past that any animation has finished.
constexpr float kLongAgo = -1.0e9f;
/// @brief How long one cell's pop-in lasts, in seconds.
constexpr float kPopSeconds = 0.16f;
/// @brief Extra delay per cell of distance from the click for revealed cells.
constexpr float kRevealRippleDelay = 0.014f;
/// @brief Extra delay per cell of distance for mines going off after a loss.
constexpr float kMineRippleDelay = 0.045f;
/// @brief Longest delay any ripple may add, so huge floods don't drag on.
constexpr float kMaxRippleDelay = 0.9f;

/// @brief Decides whether the renderer shows a cell's contents.
/// @param board Board to inspect.
/// @param cell The cell at `(x, y)` (passed in to avoid a second lookup).
/// @return True for uncovered cells, and after a loss for every unflagged
///         mine and every wrong flag (which the renderer crosses out). After
///         a win it is true for the auto-placed flags too.
bool IsVisible(const Board& board, const Cell& cell) {
  if (cell.state == CellState::Revealed) return true;
  switch (board.status()) {
    case GameStatus::Lost:
      return cell.mine != (cell.state == CellState::Flagged);
    case GameStatus::Won:
      return cell.mine;
    case GameStatus::Ready:
    case GameStatus::Playing:
      return false;
  }
  return false;
}

}  // namespace

void RevealAnimator::Reset(int columns, int rows) {
  columns_ = columns;
  const auto count = static_cast<std::size_t>(columns * rows);
  start_times_.assign(count, kLongAgo);
  was_visible_.assign(count, 0);
  last_states_.assign(count, CellState::Hidden);
}

void RevealAnimator::Sync(const Board& board, GridPoint origin, float now) {
  // After a loss the explosion, not the click, is where mines ripple from.
  // (They are the same cell unless the loss came from a chord.)
  // `value_or` returns the optional's value, or the fallback when empty.
  const GridPoint mine_origin = board.exploded().value_or(origin);

  for (int y = 0; y < board.height(); ++y) {
    for (int x = 0; x < board.width(); ++x) {
      const auto i = static_cast<std::size_t>(y * columns_ + x);
      if (i >= was_visible_.size()) return;  // Board size changed: ignore.
      const Cell& cell = board.At(x, y);
      const CellState previous = last_states_[i];
      last_states_[i] = cell.state;
      if (was_visible_[i] || !IsVisible(board, cell)) continue;
      was_visible_[i] = 1;
      // A flag that was already on screen stays put instead of re-popping.
      if (previous == CellState::Flagged && cell.state == CellState::Flagged) {
        continue;
      }

      const bool is_mine_burst = board.status() == GameStatus::Lost &&
                                 cell.state != CellState::Revealed;
      const GridPoint from = is_mine_burst ? mine_origin : origin;
      const float per_cell =
          is_mine_burst ? kMineRippleDelay : kRevealRippleDelay;
      // `std::hypot(dx, dy)` is sqrt(dx*dx + dy*dy), the straight-line
      // distance, computed without overflow for large values.
      const float distance = std::hypot(static_cast<float>(x - from.x),
                                        static_cast<float>(y - from.y));
      start_times_[i] = now + std::min(distance * per_cell, kMaxRippleDelay);
    }
  }
}

float RevealAnimator::Progress(int x, int y, float now) const {
  const auto i = static_cast<std::size_t>(y * columns_ + x);
  if (x < 0 || y < 0 || x >= columns_ || i >= start_times_.size()) return 1.0f;
  return std::clamp((now - start_times_[i]) / kPopSeconds, 0.0f, 1.0f);
}

}  // namespace minesweeper
