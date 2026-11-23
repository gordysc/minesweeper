#ifndef MINESWEEPER_GAME_H_
#define MINESWEEPER_GAME_H_

#include <cstdint>
#include <filesystem>
#include <optional>
#include <vector>

#include "board.h"
#include "difficulty.h"
#include "effects.h"
#include "layout.h"
#include "renderer.h"
#include "storage.h"

/// @brief Everything that belongs to the Minesweeper game.
namespace minesweeper {

/// @brief Top-level coordinator: reads input, drives the Board, keeps the
/// clock, records results, and hands everything to the Renderer.
///
/// `main()` constructs one `Game` inside an explicit RAII scope block so its
/// destructor (which saves progress) runs before `CloseWindow()`.
class Game {
 public:
  /// @brief Loads saved records and starts a game on the last difficulty.
  Game();

  /// @brief Saves the selected difficulty and records on clean shutdown.
  ~Game();

  /// @brief Non-copyable: there is only ever one game, and a copy would save
  /// the same file twice when both copies are destroyed.
  Game(const Game&) = delete;
  /// @brief Non-copy-assignable for the same reason.
  Game& operator=(const Game&) = delete;

  /// @brief Handles input and advances clocks by one frame.
  /// @param dt Seconds elapsed since the previous frame.
  void Update(float dt);

  /// @brief Renders the current frame between raylib's `BeginDrawing()` and
  /// `EndDrawing()`. Marked `const` so drawing can never mutate game state.
  void Draw() const;

 private:
  /// @brief Deals a fresh board on the current difficulty.
  void StartNewGame();

  /// @brief Switches difficulty (saving the choice) and starts a new game.
  /// @param difficulty Preset to switch to.
  void SetDifficulty(Difficulty difficulty);

  /// @brief Handles clicks on the board, reset button, and tabs, and works
  /// out which cells to draw pressed-in.
  void HandleMouse();

  /// @brief Handles hotkeys and keyboard-only play (move cursor, reveal,
  /// flag).
  void HandleKeyboard();

  /// @brief Reveals a covered cell, or chords a revealed number.
  /// @param cell Target cell.
  void OpenCell(GridPoint cell);

  /// @brief Places or removes a flag.
  /// @param cell Target cell.
  void FlagCell(GridPoint cell);

  /// @brief Runs after every move: starts animations and, the first time
  /// the game ends, records the result and starts the loss shake.
  /// @param origin Cell the move targeted.
  void AfterMove(GridPoint origin);

  /// @brief Returns the saved record for the current difficulty.
  /// @return Reference into `save_` (valid as long as this Game lives).
  const DifficultyRecord& CurrentRecord() const;

  /// @brief Writes `save_` to `save_path_` so records persist.
  void PersistSaveData();

  /// @brief Generates a fresh seed for a new board.
  /// @return 32-bit PRNG seed.
  std::uint32_t NextSeed();

  /// @brief Filesystem path where records are stored.
  std::filesystem::path save_path_;
  /// @brief Loaded difficulty choice and records.
  SaveData save_{};
  /// @brief Difficulty currently being played.
  Difficulty difficulty_ = Difficulty::Beginner;
  /// @brief The rules and state of the current game.
  Board board_;
  /// @brief Screen placement of `board_`'s cells.
  BoardLayout layout_{};
  /// @brief Per-cell pop-in timings.
  RevealAnimator animator_;
  /// @brief Stateless drawing subsystem.
  Renderer renderer_;

  /// @brief Game clock in seconds; runs only while the board is `Playing`.
  float elapsed_ = 0.0f;
  /// @brief Animation clock in seconds; always runs.
  float now_ = 0.0f;
  /// @brief Seconds of board shake left after a loss.
  float shake_timer_ = 0.0f;
  /// @brief True once this game's win or loss has been recorded, so it is
  /// never counted twice.
  bool result_recorded_ = false;
  /// @brief True when this game's win set a new best time.
  bool new_record_ = false;

  /// @brief Cell under the mouse, if any.
  std::optional<GridPoint> hover_;
  /// @brief Cells drawn pressed-in this frame (press preview).
  std::vector<GridPoint> pressed_;
  /// @brief True while a left-button press that started on the board is held.
  bool board_armed_ = false;
  /// @brief True while a left-button press that started on the face is held.
  bool face_armed_ = false;

  /// @brief Keyboard cursor position.
  GridPoint cursor_{};
  /// @brief True while the keyboard is in use; moving the mouse hides the
  /// cursor again.
  bool keyboard_active_ = false;

  /// @brief Monotonic counter mixed into seeds so each board is different.
  std::uint32_t seed_counter_ = 1;
};

}  // namespace minesweeper

#endif  // MINESWEEPER_GAME_H_
