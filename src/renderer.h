#ifndef MINESWEEPER_RENDERER_H_
#define MINESWEEPER_RENDERER_H_

#include <raylib.h>

#include <optional>
#include <span>

#include "board.h"
#include "difficulty.h"
#include "effects.h"
#include "layout.h"
#include "storage.h"

/// @brief Everything that belongs to the Minesweeper game.
namespace minesweeper {

/// @brief Read-only snapshot of UI state handed from Game to Renderer, so
/// drawing stays completely `const` and side-effect-free.
struct FrameView {
  /// @brief Difficulty currently being played.
  Difficulty difficulty = Difficulty::Beginner;
  /// @brief Seconds on the game clock.
  float elapsed = 0.0f;
  /// @brief Animation clock in seconds (never pauses).
  float now = 0.0f;
  /// @brief Saved results for `difficulty`.
  DifficultyRecord record{};
  /// @brief True when the game just won beat the saved best time.
  bool new_record = false;
  /// @brief Cell under the mouse pointer, if any.
  std::optional<GridPoint> hover;
  /// @brief Keyboard cursor cell, shown only while the keyboard is in use.
  std::optional<GridPoint> cursor;
  /// @brief Covered cells drawn pushed-in while a mouse button is held.
  ///
  /// `std::span` (C++20) is a non-owning view of a contiguous sequence: a
  /// pointer plus a length. It lets the Renderer read Game's vector without
  /// copying it, but it must not outlive that vector (it would dangle).
  std::span<const GridPoint> pressed;
  /// @brief True while the mouse is held on the reset button.
  bool face_pressed = false;
  /// @brief True while a mouse button is held on the board, which makes the
  /// face look nervous.
  bool board_pressed = false;
  /// @brief Board offset used to shake the board after a loss.
  Vector2 shake{0.0f, 0.0f};
  /// @brief Mouse position, for hover highlights on buttons.
  Vector2 mouse{0.0f, 0.0f};
};

/// @brief Stateless drawing subsystem: turns a Board plus a FrameView into
/// one frame of raylib draw calls.
class Renderer {
 public:
  /// @brief Draws the whole window. Call between `BeginDrawing()` and
  /// `EndDrawing()`.
  /// @param board Board to draw.
  /// @param layout Where the board sits on screen.
  /// @param animator Per-cell pop-in timings.
  /// @param view UI state for this frame.
  void Draw(const Board& board, const BoardLayout& layout,
            const RevealAnimator& animator, const FrameView& view) const;
};

}  // namespace minesweeper

#endif  // MINESWEEPER_RENDERER_H_
