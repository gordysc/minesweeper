#include <cmath>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <system_error>
#include <vector>

#include "board.h"
#include "difficulty.h"
#include "storage.h"

/// @brief Minimal self-contained unit test suite for `minesweeper_core`.
namespace {

/// @brief Total number of assertions that failed during this test run.
int g_failures = 0;

/// @brief Checks a boolean condition and prints a diagnostic message on
/// failure.
/// @param condition Expression expected to be true.
/// @param message Human-readable failure description.
void Expect(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << "\n";
    ++g_failures;
  }
}

/// @brief Counts the mines actually placed on `board`.
/// @param board Board to scan (by `const&`: read-only, no copy).
/// @return Number of cells with `mine == true`.
int CountMines(const minesweeper::Board& board) {
  int mines = 0;
  for (int y = 0; y < board.height(); ++y) {
    for (int x = 0; x < board.width(); ++x) {
      if (board.At(x, y).mine) ++mines;
    }
  }
  return mines;
}

/// @brief Verifies the three presets match the classic dimensions.
void TestDifficultyPresets() {
  using minesweeper::ConfigFor;
  using minesweeper::Difficulty;
  const auto beginner = ConfigFor(Difficulty::Beginner);
  const auto intermediate = ConfigFor(Difficulty::Intermediate);
  const auto expert = ConfigFor(Difficulty::Expert);
  Expect(beginner.width == 9 && beginner.height == 9 && beginner.mines == 10,
         "Beginner is 9x9 with 10 mines");
  Expect(intermediate.width == 16 && intermediate.height == 16 &&
             intermediate.mines == 40,
         "Intermediate is 16x16 with 40 mines");
  Expect(expert.width == 30 && expert.height == 16 && expert.mines == 99,
         "Expert is 30x16 with 99 mines");
}

/// @brief Verifies mines are placed only on the first reveal, never under or
/// next to it, and identically for the same seed.
void TestFirstClickIsSafe() {
  const auto config = minesweeper::ConfigFor(minesweeper::Difficulty::Expert);
  for (std::uint32_t seed = 1; seed <= 200; ++seed) {
    minesweeper::Board board(config, seed);
    Expect(CountMines(board) == 0, "No mines exist before the first reveal");

    const int cx = static_cast<int>(seed % 30);
    const int cy = static_cast<int>(seed % 16);
    board.Reveal(cx, cy);
    Expect(board.status() != minesweeper::GameStatus::Lost,
           "First reveal never hits a mine");
    Expect(CountMines(board) == 99, "Exactly 99 mines placed on Expert");
    Expect(board.At(cx, cy).adjacent == 0,
           "First reveal opens a zero cell when the board has room");
  }

  minesweeper::Board a(config, 7);
  minesweeper::Board b(config, 7);
  a.Reveal(4, 4);
  b.Reveal(4, 4);
  bool same = true;
  for (int y = 0; y < a.height(); ++y) {
    for (int x = 0; x < a.width(); ++x) {
      if (a.At(x, y).mine != b.At(x, y).mine) same = false;
    }
  }
  Expect(same, "Same seed and first click give an identical layout");

  // A crowded board can't keep 9 cells clear, but the clicked cell still is.
  minesweeper::Board crowded({3, 3, 8}, 3);
  crowded.Reveal(1, 1);
  Expect(crowded.status() == minesweeper::GameStatus::Won,
         "3x3 board with 8 mines is won by its single safe first click");
}

/// @brief Verifies adjacency numbers and flood fill on a fixed layout.
void TestFloodFillAndNumbers() {
  // 5x5 board, one mine in the bottom-right corner:
  //   . . . . .
  //   . . . . .
  //   . . . . .
  //   . . . 1 1
  //   . . . 1 *
  minesweeper::Board board({5, 5, 1}, 1);
  board.SetMineLayout({{4, 4}});
  Expect(board.At(3, 3).adjacent == 1, "Diagonal neighbour of the mine is 1");
  Expect(board.At(0, 0).adjacent == 0, "Far corner is 0");

  const int uncovered = board.Reveal(0, 0);
  Expect(uncovered == 24, "Flood fill from a zero uncovers all 24 safe cells");
  Expect(board.status() == minesweeper::GameStatus::Won,
         "Uncovering every safe cell wins");
  Expect(board.At(4, 4).state == minesweeper::CellState::Flagged,
         "Winning auto-flags the remaining mine");
  Expect(board.mines_remaining() == 0, "Mine counter reads 0 after a win");
}

/// @brief Verifies flags block reveals, stop floods, and drive the counter.
void TestFlags() {
  minesweeper::Board board({5, 5, 1}, 1);
  board.SetMineLayout({{4, 4}});

  Expect(board.ToggleFlag(0, 4), "Flagging a covered cell succeeds");
  Expect(board.mines_remaining() == 0, "Counter drops by one per flag");
  Expect(board.Reveal(0, 4) == 0, "Flagged cells can't be revealed");

  board.Reveal(0, 0);
  Expect(board.At(0, 4).state == minesweeper::CellState::Flagged,
         "Flood fill leaves flagged cells alone");
  Expect(board.status() == minesweeper::GameStatus::Playing,
         "Game isn't won while a safe cell is still flagged");

  Expect(board.ToggleFlag(0, 4), "Removing a flag succeeds");
  Expect(board.mines_remaining() == 1, "Counter rises when a flag is removed");
  Expect(!board.ToggleFlag(1, 1), "Revealed cells can't be flagged");
}

/// @brief Verifies chording with correct and incorrect flags.
void TestChord() {
  // 3x3 board, mine at top-left. The centre shows 1.
  //   * 1 .
  //   1 1 .
  //   . . .
  {
    minesweeper::Board board({3, 3, 1}, 1);
    board.SetMineLayout({{0, 0}});
    board.Reveal(1, 1);
    Expect(board.Chord(1, 1) == 0, "Chord needs matching flags first");
    board.ToggleFlag(0, 0);
    const int uncovered = board.Chord(1, 1);
    Expect(uncovered == 7, "Correct chord uncovers the 7 other safe cells");
    Expect(board.status() == minesweeper::GameStatus::Won,
           "Correct chord can finish the game");
  }
  {
    minesweeper::Board board({3, 3, 1}, 1);
    board.SetMineLayout({{0, 0}});
    board.Reveal(1, 1);
    board.ToggleFlag(2, 2);  // Wrong cell.
    board.Chord(1, 1);
    Expect(board.status() == minesweeper::GameStatus::Lost,
           "Chord with a wrong flag detonates the real mine");
    Expect(board.exploded().has_value() &&
               *board.exploded() == minesweeper::GridPoint{0, 0},
           "exploded() reports the mine the chord hit");
  }
}

/// @brief Verifies losing ends the game and freezes further input.
void TestLoss() {
  minesweeper::Board board({4, 4, 2}, 1);
  board.SetMineLayout({{0, 0}, {3, 3}});
  board.Reveal(0, 0);
  Expect(board.status() == minesweeper::GameStatus::Lost,
         "Revealing a mine loses");
  Expect(board.Reveal(1, 1) == 0, "No reveals after the game is lost");
  Expect(!board.ToggleFlag(2, 2), "No flags after the game is lost");

  board.Reset({4, 4, 2}, 2);
  Expect(board.status() == minesweeper::GameStatus::Ready &&
             !board.exploded().has_value() && board.revealed_count() == 0,
         "Reset returns to a fresh Ready board");
}

/// @brief Verifies out-of-range configs are clamped to a playable board.
void TestConfigClamping() {
  minesweeper::Board board({0, 500, 100000}, 1);
  Expect(board.width() == 2 && board.height() == 99, "Edges clamp to [2, 99]");
  Expect(board.mine_count() == 2 * 99 - 1, "Mines clamp to cells - 1");
  Expect(board.At(-1, 0).state == minesweeper::CellState::Hidden &&
             !board.At(-1, 0).mine,
         "Out-of-bounds reads return a covered empty cell");
}

/// @brief Verifies best times and win/loss counts survive a save round trip.
void TestStorageRoundTrip() {
  const std::filesystem::path temp_path =
      std::filesystem::temp_directory_path() / "minesweeper_test_save.cfg";

  minesweeper::SaveData out;
  out.difficulty = minesweeper::Difficulty::Expert;
  Expect(minesweeper::RecordResult(out, minesweeper::Difficulty::Expert, true,
                                   123.5f),
         "First Expert win sets a best time");
  Expect(!minesweeper::RecordResult(out, minesweeper::Difficulty::Expert, true,
                                    200.0f),
         "Slower win does not beat the best time");
  Expect(minesweeper::RecordResult(out, minesweeper::Difficulty::Expert, true,
                                   99.25f),
         "Faster win beats the best time");
  Expect(!minesweeper::RecordResult(out, minesweeper::Difficulty::Beginner,
                                    false, 0.0f),
         "Losses never set a best time");
  Expect(minesweeper::WriteSaveData(temp_path, out), "WriteSaveData succeeds");

  const minesweeper::SaveData in = minesweeper::LoadSaveData(temp_path);
  Expect(in.difficulty == minesweeper::Difficulty::Expert,
         "difficulty round-trips");
  Expect(std::abs(in.records[2].best_time - 99.25f) < 0.01f,
         "expert_best_time round-trips");
  Expect(in.records[2].wins == 3, "expert_wins round-trips");
  Expect(in.records[0].losses == 1, "beginner_losses round-trips");

  // `std::error_code` overload: reports failure through `ec` instead of
  // throwing, so cleanup can never abort the test run.
  std::error_code ec;
  std::filesystem::remove(temp_path, ec);
}

}  // namespace

/// @brief Entry point for the CTest unit test binary.
/// @return 0 if all tests pass, non-zero otherwise.
int main() {
  TestDifficultyPresets();
  TestFirstClickIsSafe();
  TestFloodFillAndNumbers();
  TestFlags();
  TestChord();
  TestLoss();
  TestConfigClamping();
  TestStorageRoundTrip();

  if (g_failures > 0) {
    std::cerr << g_failures << " test assertion(s) failed.\n";
    return 1;
  }
  std::cout << "All minesweeper_core tests passed!\n";
  return 0;
}
