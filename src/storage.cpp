#include "storage.h"

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <string>

/// @brief Everything that belongs to the Minesweeper game.
namespace minesweeper {

/// @brief Defensive parsing helpers internal to `storage.cpp`.
namespace {

/// @brief Parses an integer from `value` and clamps it to `[min_val, max_val]`.
///
/// `std::stoi` throws `std::invalid_argument` or `std::out_of_range` on bad
/// input; catching all exceptions guarantees a hand-edited or corrupted save
/// file can never crash startup.
///
/// @param value Raw string slice after `=`.
/// @param fallback Value returned if parsing fails.
/// @param min_val Minimum permitted value.
/// @param max_val Maximum permitted value.
/// @return Clamped integer result or `fallback`.
int ParseInt(const std::string& value, int fallback, int min_val, int max_val) {
  try {
    return std::clamp(std::stoi(value), min_val, max_val);
  } catch (...) {
    return fallback;
  }
}

/// @brief Parses a floating-point number from `value` and clamps it to
/// `[min_val, max_val]`, returning `fallback` on malformed text.
/// @param value Raw string slice after `=`.
/// @param fallback Value returned if `std::stof` throws.
/// @param min_val Minimum permitted value.
/// @param max_val Maximum permitted value.
/// @return Clamped float result or `fallback`.
float ParseFloat(const std::string& value, float fallback, float min_val,
                 float max_val) {
  try {
    return std::clamp(std::stof(value), min_val, max_val);
  } catch (...) {
    return fallback;
  }
}

}  // namespace

std::filesystem::path DefaultSavePath() {
  // `if (init; cond)` (C++17) limits `home`'s scope to this `if` statement.
  // `std::getenv` returns a non-owning pointer into the process environment
  // block (never free it), or `nullptr` if HOME is unset.
  if (const char* home = std::getenv("HOME");
      home != nullptr && *home != '\0') {
    // `operator/` on `std::filesystem::path` joins two path pieces with the
    // platform's directory separator.
    return std::filesystem::path(home) / ".minesweeper.cfg";
  }
  return std::filesystem::path(".minesweeper.cfg");
}

SaveData LoadSaveData(const std::filesystem::path& path) {
  SaveData data;
  // RAII: `std::ifstream` opens the file in its constructor and automatically
  // closes the file handle in its destructor when this function returns.
  std::ifstream input(path);
  if (!input.is_open()) return data;

  std::string line;
  while (std::getline(input, line)) {
    if (line.empty() || line[0] == '#') continue;
    const std::size_t eq = line.find('=');
    if (eq == std::string::npos) continue;

    const std::string key = line.substr(0, eq);
    const std::string val = line.substr(eq + 1);

    if (key == "difficulty") {
      data.difficulty =
          static_cast<Difficulty>(ParseInt(val, 0, 0, kDifficultyCount - 1));
      continue;
    }

    // Per-difficulty keys look like `expert_best_time`. Try each prefix.
    for (int i = 0; i < kDifficultyCount; ++i) {
      const std::string prefix =
          std::string(DifficultyKey(static_cast<Difficulty>(i))) + "_";
      // `rfind(prefix, 0)` only looks for a match starting at index 0, which
      // makes it a cheap "starts with" test (C++20 also has `starts_with`).
      if (key.rfind(prefix, 0) != 0) continue;
      const std::string field = key.substr(prefix.size());
      // `records[i]` by reference, so the assignments below write into
      // `data` rather than into a temporary copy.
      DifficultyRecord& record = data.records[static_cast<std::size_t>(i)];
      if (field == "best_time") {
        record.best_time = ParseFloat(val, 0.0f, 0.0f, 99999.0f);
      } else if (field == "wins") {
        record.wins = ParseInt(val, 0, 0, 999999999);
      } else if (field == "losses") {
        record.losses = ParseInt(val, 0, 0, 999999999);
      }
    }
  }
  return data;
}

bool WriteSaveData(const std::filesystem::path& path, const SaveData& data) {
  // `std::ofstream` truncates (empties) an existing file on open by default.
  std::ofstream output(path);
  if (!output.is_open()) return false;

  output << "# Minesweeper save file\n";
  output << "difficulty=" << static_cast<int>(data.difficulty) << "\n";
  // `std::fixed` + `std::setprecision(3)` print floats as `12.345` instead
  // of switching to scientific notation for very small or large values.
  output << std::fixed << std::setprecision(3);
  for (int i = 0; i < kDifficultyCount; ++i) {
    const std::string key = DifficultyKey(static_cast<Difficulty>(i));
    const DifficultyRecord& record = data.records[static_cast<std::size_t>(i)];
    output << key << "_best_time=" << record.best_time << "\n";
    output << key << "_wins=" << record.wins << "\n";
    output << key << "_losses=" << record.losses << "\n";
  }
  // `good()` is false if any write above failed (disk full, permissions).
  return output.good();
}

bool RecordResult(SaveData& data, Difficulty difficulty, bool won,
                  float seconds) {
  DifficultyRecord& record = data.records[static_cast<std::size_t>(difficulty)];
  if (!won) {
    ++record.losses;
    return false;
  }
  ++record.wins;
  if (record.best_time <= 0.0f || seconds < record.best_time) {
    record.best_time = seconds;
    return true;
  }
  return false;
}

}  // namespace minesweeper
