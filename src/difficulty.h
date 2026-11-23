#ifndef MINESWEEPER_DIFFICULTY_H_
#define MINESWEEPER_DIFFICULTY_H_

#include <cstdint>

/// @brief Everything that belongs to the Minesweeper game.
namespace minesweeper {

/// @brief The three classic board presets.
///
/// Using `enum class` instead of a plain `enum` keeps enumerators scoped
/// (`Difficulty::Expert`) so they never collide with raylib names, and
/// prevents accidental implicit conversion to `int`.
enum class Difficulty : std::uint8_t {
  /// @brief 9x9 board with 10 mines.
  Beginner = 0,
  /// @brief 16x16 board with 40 mines.
  Intermediate = 1,
  /// @brief 30x16 board with 99 mines.
  Expert = 2,
};

/// @brief Number of Difficulty values, for sizing per-difficulty arrays.
inline constexpr int kDifficultyCount = 3;

/// @brief Dimensions and mine count of one board.
struct BoardConfig {
  /// @brief Number of columns.
  int width = 9;
  /// @brief Number of rows.
  int height = 9;
  /// @brief Number of mines hidden on the board.
  int mines = 10;
};

/// @brief Returns the board dimensions and mine count for `difficulty`.
/// @param difficulty Preset to look up.
/// @return Matching BoardConfig.
BoardConfig ConfigFor(Difficulty difficulty);

/// @brief Returns the upper-case display name of `difficulty`.
/// @param difficulty Preset to name.
/// @return Pointer to a static string literal (never free it).
const char* DifficultyName(Difficulty difficulty);

/// @brief Returns the lower-case key used for `difficulty` in the save file.
/// @param difficulty Preset to name.
/// @return Pointer to a static string literal such as `"beginner"`.
const char* DifficultyKey(Difficulty difficulty);

}  // namespace minesweeper

#endif  // MINESWEEPER_DIFFICULTY_H_
