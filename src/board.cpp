#include "board.h"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <random>
#include <vector>

/// @brief Everything that belongs to the Minesweeper game.
namespace minesweeper {

/// @brief Constants and helpers internal to `board.cpp`.
///
/// An unnamed namespace gives everything inside it internal linkage: these
/// names are visible only in this .cpp file, so they can never clash with a
/// same-named helper in another file at link time.
namespace {

/// @brief Smallest board edge length the clamp in `Reset()` allows.
constexpr int kMinEdge = 2;
/// @brief Largest board edge length the clamp in `Reset()` allows.
constexpr int kMaxEdge = 99;

/// @brief Offsets from a cell to its 8 neighbours (Moore neighbourhood).
///
/// `std::array` is the modern replacement for a C array like
/// `GridPoint kNeighbours[8]`: it knows its own size, can be copied, and
/// works with range-based `for`.
constexpr std::array<GridPoint, 8> kNeighbours{{
    {-1, -1},
    {0, -1},
    {1, -1},  //
    {-1, 0},
    {1, 0},  //
    {-1, 1},
    {0, 1},
    {1, 1},  //
}};

/// @brief A covered, mine-free cell returned by `Board::At()` for
/// out-of-bounds reads.
///
/// `At()` returns a `const Cell&`, so it must refer to an object that
/// outlives the call. A namespace-scope object lives for the whole program,
/// so the reference can never dangle (returning a reference to a local
/// variable would be undefined behaviour).
const Cell kOutOfBoundsCell{};

}  // namespace

// Delegating constructor (C++11): reuses the seeded constructor instead of
// duplicating its setup. `std::random_device{}()` creates a temporary
// hardware-entropy source and calls it once for a fresh seed.
Board::Board()
    : Board(ConfigFor(Difficulty::Beginner), std::random_device{}()) {}

Board::Board(BoardConfig config, std::uint32_t seed) { Reset(config, seed); }

void Board::Reset(BoardConfig config, std::uint32_t seed) {
  // `std::clamp(v, lo, hi)` (C++17) pins `v` into `[lo, hi]`. Keeping at
  // least one safe cell guarantees the first click can always succeed.
  config.width = std::clamp(config.width, kMinEdge, kMaxEdge);
  config.height = std::clamp(config.height, kMinEdge, kMaxEdge);
  config.mines = std::clamp(config.mines, 1, config.width * config.height - 1);
  config_ = config;

  // `assign(n, value)` resizes the vector to `n` copies of `value`, reusing
  // the existing heap buffer when it is already big enough.
  cells_.assign(static_cast<std::size_t>(config_.width * config_.height),
                Cell{});
  status_ = GameStatus::Ready;
  flags_placed_ = 0;
  revealed_count_ = 0;
  // Assigning `std::nullopt` empties a `std::optional`. An optional is the
  // modern alternative to a sentinel like `{-1, -1}` for "no value".
  exploded_ = std::nullopt;
  rng_.seed(seed);
}

void Board::SetMineLayout(const std::vector<GridPoint>& mines) {
  cells_.assign(cells_.size(), Cell{});
  int placed = 0;
  // Range-based `for` with `const GridPoint&` reads each element in place
  // instead of copying it.
  for (const GridPoint& p : mines) {
    if (!InBounds(p.x, p.y)) continue;
    Cell& cell = cells_[Index(p.x, p.y)];
    if (cell.mine) continue;  // Ignore duplicate points.
    cell.mine = true;
    ++placed;
  }
  config_.mines = placed;
  status_ = GameStatus::Playing;
  flags_placed_ = 0;
  revealed_count_ = 0;
  exploded_ = std::nullopt;
  ComputeAdjacency();
  // A layout with no mines is already solved: there is nothing left to find.
  CheckForWin();
}

int Board::Reveal(int x, int y) {
  if (!InBounds(x, y) || is_over()) return 0;
  if (cells_[Index(x, y)].state != CellState::Hidden) return 0;

  if (status_ == GameStatus::Ready) {
    PlaceMines(x, y);
    status_ = GameStatus::Playing;
  }

  if (cells_[Index(x, y)].mine) {
    Detonate(x, y);
    return 0;
  }
  const int uncovered = FloodReveal(x, y);
  CheckForWin();
  return uncovered;
}

int Board::Chord(int x, int y) {
  if (!InBounds(x, y) || status_ != GameStatus::Playing) return 0;
  const Cell& centre = cells_[Index(x, y)];
  if (centre.state != CellState::Revealed || centre.adjacent == 0) return 0;
  if (CountAdjacentFlags(x, y) != centre.adjacent) return 0;

  // First pass: if any unflagged neighbour is a mine, a flag is wrong and the
  // chord detonates it, just as revealing that cell by hand would.
  for (const GridPoint& d : kNeighbours) {
    const int nx = x + d.x;
    const int ny = y + d.y;
    if (!InBounds(nx, ny)) continue;
    const Cell& n = cells_[Index(nx, ny)];
    if (n.state == CellState::Hidden && n.mine) {
      Detonate(nx, ny);
      return 0;
    }
  }

  // Second pass: every covered neighbour is safe, so flood-reveal each one.
  // FloodReveal skips cells an earlier flood already uncovered.
  int uncovered = 0;
  for (const GridPoint& d : kNeighbours) {
    const int nx = x + d.x;
    const int ny = y + d.y;
    if (InBounds(nx, ny) && cells_[Index(nx, ny)].state == CellState::Hidden) {
      uncovered += FloodReveal(nx, ny);
    }
  }
  CheckForWin();
  return uncovered;
}

bool Board::ToggleFlag(int x, int y) {
  if (!InBounds(x, y) || is_over()) return false;
  // `Cell&` is a non-const reference: writing through it changes the element
  // stored inside `cells_` rather than a copy.
  Cell& cell = cells_[Index(x, y)];
  switch (cell.state) {
    case CellState::Hidden:
      cell.state = CellState::Flagged;
      ++flags_placed_;
      return true;
    case CellState::Flagged:
      cell.state = CellState::Hidden;
      --flags_placed_;
      return true;
    case CellState::Revealed:
      return false;
  }
  return false;
}

bool Board::InBounds(int x, int y) const {
  return x >= 0 && x < config_.width && y >= 0 && y < config_.height;
}

const Cell& Board::At(int x, int y) const {
  if (!InBounds(x, y)) return kOutOfBoundsCell;
  return cells_[Index(x, y)];
}

int Board::CountAdjacentFlags(int x, int y) const {
  int flags = 0;
  for (const GridPoint& d : kNeighbours) {
    if (At(x + d.x, y + d.y).state == CellState::Flagged) ++flags;
  }
  return flags;
}

void Board::PlaceMines(int safe_x, int safe_y) {
  const int total = config_.width * config_.height;
  // Keep the whole 3x3 block around the first click clear when the board has
  // room for every mine outside it; otherwise keep just the clicked cell.
  const bool protect_neighbours = total - 9 >= config_.mines;

  std::vector<int> candidates;
  // `reserve()` allocates the full capacity up front so `push_back()` never
  // has to reallocate (and copy) the buffer mid-loop.
  candidates.reserve(static_cast<std::size_t>(total));
  for (int y = 0; y < config_.height; ++y) {
    for (int x = 0; x < config_.width; ++x) {
      // `std::abs` from <cstdlib> gives the distance from the click on each
      // axis; both <= 1 means "the click itself or one of its neighbours".
      const bool near_click =
          std::abs(x - safe_x) <= 1 && std::abs(y - safe_y) <= 1;
      const bool is_click = x == safe_x && y == safe_y;
      if (is_click || (protect_neighbours && near_click)) continue;
      candidates.push_back(Index(x, y));
    }
  }

  // `std::shuffle` applies a uniform Fisher-Yates shuffle driven by `rng_`;
  // the first `mines` entries are then a uniformly random choice of cells.
  // (The exact sequence for a seed can differ between standard libraries, but
  // is always the same within one build.)
  std::shuffle(candidates.begin(), candidates.end(), rng_);
  for (int i = 0; i < config_.mines; ++i) {
    cells_[static_cast<std::size_t>(candidates[static_cast<std::size_t>(i)])]
        .mine = true;
  }
  ComputeAdjacency();
}

void Board::ComputeAdjacency() {
  for (int y = 0; y < config_.height; ++y) {
    for (int x = 0; x < config_.width; ++x) {
      int count = 0;
      for (const GridPoint& d : kNeighbours) {
        // `At()` returns the empty out-of-bounds cell past the edges, so the
        // border needs no special case.
        if (At(x + d.x, y + d.y).mine) ++count;
      }
      cells_[Index(x, y)].adjacent = static_cast<std::uint8_t>(count);
    }
  }
}

int Board::FloodReveal(int x, int y) {
  int uncovered = 0;
  // A `std::vector` used as a LIFO stack (`push_back` / `back` / `pop_back`)
  // replaces recursion: the pending work lives on the heap, so even a 99x99
  // empty board cannot overflow the call stack.
  std::vector<GridPoint> pending{{x, y}};
  while (!pending.empty()) {
    // Copy the point out *before* `pop_back()`: a reference to `back()`
    // would dangle once the element is removed.
    const GridPoint p = pending.back();
    pending.pop_back();

    Cell& cell = cells_[Index(p.x, p.y)];
    // Flags stop the flood (the player marked them on purpose), and a cell
    // can be pushed twice before it is processed, so skip anything that is
    // no longer covered.
    if (cell.state != CellState::Hidden) continue;
    cell.state = CellState::Revealed;
    ++uncovered;
    ++revealed_count_;

    // Only a zero cell spreads. Its neighbours are all safe by definition,
    // because it has no adjacent mines.
    if (cell.adjacent != 0) continue;
    for (const GridPoint& d : kNeighbours) {
      const int nx = p.x + d.x;
      const int ny = p.y + d.y;
      if (InBounds(nx, ny) &&
          cells_[Index(nx, ny)].state == CellState::Hidden) {
        pending.push_back({nx, ny});
      }
    }
  }
  return uncovered;
}

void Board::Detonate(int x, int y) {
  status_ = GameStatus::Lost;
  // Brace-initialising a `std::optional<GridPoint>` stores a value in it.
  exploded_ = GridPoint{x, y};
}

void Board::CheckForWin() {
  const int safe_cells = config_.width * config_.height - config_.mines;
  if (revealed_count_ < safe_cells) return;
  status_ = GameStatus::Won;
  // Flag every mine left covered so the counter reaches 0, like the classic.
  for (Cell& cell : cells_) {
    if (cell.mine && cell.state == CellState::Hidden) {
      cell.state = CellState::Flagged;
      ++flags_placed_;
    }
  }
}

}  // namespace minesweeper
