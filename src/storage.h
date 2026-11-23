#ifndef MINESWEEPER_STORAGE_H_
#define MINESWEEPER_STORAGE_H_

#include <array>
#include <filesystem>

#include "difficulty.h"

/// @brief Everything that belongs to the Minesweeper game.
namespace minesweeper {

/// @brief Results recorded for one difficulty.
struct DifficultyRecord {
  /// @brief Fastest win in seconds (`0.0` = no win yet).
  float best_time = 0.0f;
  /// @brief Number of games won.
  int wins = 0;
  /// @brief Number of games lost. Games abandoned with a restart don't count.
  int losses = 0;
};

/// @brief Everything persisted between runs.
struct SaveData {
  /// @brief Difficulty selected when the game last closed.
  Difficulty difficulty = Difficulty::Beginner;
  /// @brief Records indexed by `static_cast<int>(Difficulty)`.
  std::array<DifficultyRecord, kDifficultyCount> records{};
};

/// @brief Returns the default save file path (`~/.minesweeper.cfg`).
/// @return `$HOME/.minesweeper.cfg`, or `.minesweeper.cfg` in the working
///         directory if `HOME` is unset.
std::filesystem::path DefaultSavePath();

/// @brief Loads SaveData from `path`. Missing or malformed fields fall back
/// to defaults without failing or throwing.
/// @param path File path to read.
/// @return Loaded SaveData (or defaults if the file does not exist yet).
SaveData LoadSaveData(const std::filesystem::path& path);

/// @brief Writes `data` to `path` in a human-readable `key=value` format.
/// @param path Destination file path (created or overwritten).
/// @param data Settings and records to persist.
/// @return True if the file was written cleanly.
bool WriteSaveData(const std::filesystem::path& path, const SaveData& data);

/// @brief Records a finished game and reports whether it set a new best time.
/// @param data SaveData modified in place (passed by non-const `&`).
/// @param difficulty Difficulty the game was played on.
/// @param won True for a win, false for a loss.
/// @param seconds Time the win took (ignored for losses).
/// @return True if this win beat (or set the first) best time.
bool RecordResult(SaveData& data, Difficulty difficulty, bool won,
                  float seconds);

}  // namespace minesweeper

#endif  // MINESWEEPER_STORAGE_H_
