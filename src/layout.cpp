#include "layout.h"

#include <algorithm>
#include <cmath>

#include "screen.h"

/// @brief Everything that belongs to the Minesweeper game.
namespace minesweeper {

/// @brief Screen measurements internal to `layout.cpp`.
namespace {

/// @brief Gap between the window edge and the panels.
constexpr float kMargin = 40.0f;
/// @brief Top edge of the header panel.
constexpr float kHeaderTop = 20.0f;
/// @brief Height of the header panel.
constexpr float kHeaderHeight = 84.0f;
/// @brief Top edge of the difficulty tab row.
constexpr float kTabsTop = 118.0f;
/// @brief Height of a difficulty tab.
constexpr float kTabHeight = 36.0f;
/// @brief Width of a difficulty tab.
constexpr float kTabWidth = 200.0f;
/// @brief Gap between neighbouring tabs.
constexpr float kTabGap = 12.0f;
/// @brief Top edge of the area the board is fitted into.
constexpr float kBoardAreaTop = 176.0f;
/// @brief Bottom edge of the area the board is fitted into.
constexpr float kBoardAreaBottom = 726.0f;
/// @brief Largest cell size, so a 9x9 board doesn't fill the whole window.
constexpr float kMaxCellSize = 52.0f;
/// @brief Side of the square reset button.
constexpr float kFaceSize = 64.0f;
/// @brief Width of the mine counter and timer boxes.
constexpr float kCounterWidth = 132.0f;
/// @brief Height of the mine counter and timer boxes.
constexpr float kCounterHeight = 48.0f;

}  // namespace

Rectangle BoardLayout::Bounds() const {
  return {origin_x, origin_y, cell_size * static_cast<float>(columns),
          cell_size * static_cast<float>(rows)};
}

Rectangle BoardLayout::CellRect(int x, int y) const {
  return {origin_x + cell_size * static_cast<float>(x),
          origin_y + cell_size * static_cast<float>(y), cell_size, cell_size};
}

std::optional<GridPoint> BoardLayout::CellAt(Vector2 point) const {
  // `std::floor` rounds towards negative infinity, so points just left of or
  // above the board become -1 (rejected below) rather than truncating to 0.
  const int x = static_cast<int>(std::floor((point.x - origin_x) / cell_size));
  const int y = static_cast<int>(std::floor((point.y - origin_y) / cell_size));
  if (x < 0 || x >= columns || y < 0 || y >= rows) return std::nullopt;
  return GridPoint{x, y};
}

BoardLayout ComputeBoardLayout(int columns, int rows) {
  const float area_width = static_cast<float>(kScreenWidth) - 2.0f * kMargin;
  const float area_height = kBoardAreaBottom - kBoardAreaTop;
  // `std::min({a, b, c})` takes an initializer list and returns the
  // smallest value, avoiding nested `std::min(a, std::min(b, c))` calls.
  // Flooring keeps every cell edge on a whole pixel, so grid lines stay
  // crisp instead of blurring across two pixels.
  const float cell = std::floor(
      std::min({area_width / static_cast<float>(columns),
                area_height / static_cast<float>(rows), kMaxCellSize}));
  BoardLayout layout;
  layout.cell_size = cell;
  layout.columns = columns;
  layout.rows = rows;
  layout.origin_x = std::floor(
      (static_cast<float>(kScreenWidth) - cell * static_cast<float>(columns)) /
      2.0f);
  layout.origin_y = std::floor(
      kBoardAreaTop + (area_height - cell * static_cast<float>(rows)) / 2.0f);
  return layout;
}

Rectangle HeaderRect() {
  return {kMargin, kHeaderTop,
          static_cast<float>(kScreenWidth) - 2.0f * kMargin, kHeaderHeight};
}

Rectangle MineCounterRect() {
  const Rectangle header = HeaderRect();
  return {header.x + 20.0f, header.y + 26.0f, kCounterWidth, kCounterHeight};
}

Rectangle TimerRect() {
  const Rectangle header = HeaderRect();
  return {header.x + header.width - 20.0f - kCounterWidth, header.y + 26.0f,
          kCounterWidth, kCounterHeight};
}

Rectangle FaceButtonRect() {
  const Rectangle header = HeaderRect();
  return {(static_cast<float>(kScreenWidth) - kFaceSize) / 2.0f,
          header.y + (header.height - kFaceSize) / 2.0f, kFaceSize, kFaceSize};
}

Rectangle DifficultyTabRect(Difficulty difficulty) {
  const float total =
      kTabWidth * kDifficultyCount + kTabGap * (kDifficultyCount - 1);
  const float left = (static_cast<float>(kScreenWidth) - total) / 2.0f;
  const auto index = static_cast<float>(difficulty);
  return {left + index * (kTabWidth + kTabGap), kTabsTop, kTabWidth,
          kTabHeight};
}

float FooterY() { return kBoardAreaBottom + 16.0f; }

}  // namespace minesweeper
