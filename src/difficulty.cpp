#include "difficulty.h"

/// @brief Everything that belongs to the Minesweeper game.
namespace minesweeper {

// Each switch below has an explicit `case` for every Difficulty and no
// `default`, so the compiler's `-Wswitch` warning catches any new preset we
// forget to handle. The trailing return only silences "control reaches end
// of non-void function" for out-of-range values cast from an int.

BoardConfig ConfigFor(Difficulty difficulty) {
  switch (difficulty) {
    case Difficulty::Beginner:
      return {9, 9, 10};
    case Difficulty::Intermediate:
      return {16, 16, 40};
    case Difficulty::Expert:
      return {30, 16, 99};
  }
  return {9, 9, 10};
}

const char* DifficultyName(Difficulty difficulty) {
  switch (difficulty) {
    case Difficulty::Beginner:
      return "BEGINNER";
    case Difficulty::Intermediate:
      return "INTERMEDIATE";
    case Difficulty::Expert:
      return "EXPERT";
  }
  return "BEGINNER";
}

const char* DifficultyKey(Difficulty difficulty) {
  switch (difficulty) {
    case Difficulty::Beginner:
      return "beginner";
    case Difficulty::Intermediate:
      return "intermediate";
    case Difficulty::Expert:
      return "expert";
  }
  return "beginner";
}

}  // namespace minesweeper
