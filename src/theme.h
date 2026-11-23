#ifndef MINESWEEPER_THEME_H_
#define MINESWEEPER_THEME_H_

#include <raylib.h>

#include <array>

/// @brief Everything that belongs to the Minesweeper game.
namespace minesweeper {

// raylib's `Color` is a plain C struct of four `unsigned char`s, which makes
// it a "literal type": it can be built at compile time, so each colour here
// is an `inline constexpr` constant with no runtime cost.

/// @brief Window background behind every panel.
inline constexpr Color kBackground{17, 19, 27, 255};
/// @brief Header, tab, and board-frame panel fill.
inline constexpr Color kPanel{27, 30, 42, 255};
/// @brief Panel fill under the mouse pointer.
inline constexpr Color kPanelHover{38, 42, 58, 255};
/// @brief Dark inset behind the mine counter and timer digits.
inline constexpr Color kCounterBackground{12, 13, 19, 255};

/// @brief Top face of a covered tile.
inline constexpr Color kTileFace{62, 69, 94, 255};
/// @brief Top face of a covered tile under the mouse pointer.
inline constexpr Color kTileFaceHover{78, 87, 117, 255};
/// @brief Lower lip that makes a covered tile look raised.
inline constexpr Color kTileShadow{40, 44, 61, 255};
/// @brief Uncovered tile.
inline constexpr Color kTileRevealed{33, 36, 49, 255};
/// @brief Covered tile held down by the mouse (press preview).
inline constexpr Color kTilePressed{46, 50, 68, 255};
/// @brief Tile behind a mine uncovered after a loss. Lighter than a revealed
/// tile so the dark mine body stands out.
inline constexpr Color kTileMine{92, 70, 86, 255};
/// @brief Tile behind the mine that ended the game.
inline constexpr Color kTileExploded{190, 40, 48, 255};

/// @brief Main text colour.
inline constexpr Color kText{226, 232, 240, 255};
/// @brief Secondary text colour for labels and hints.
inline constexpr Color kTextMuted{125, 134, 160, 255};
/// @brief Highlight for the selected difficulty tab.
inline constexpr Color kAccent{129, 140, 248, 255};
/// @brief Keyboard cursor outline.
inline constexpr Color kCursor{250, 204, 21, 255};
/// @brief Win message and "new best" text.
inline constexpr Color kSuccess{74, 222, 128, 255};
/// @brief Loss message text.
inline constexpr Color kDanger{248, 113, 113, 255};

/// @brief Mine counter digits.
inline constexpr Color kMineDigits{248, 113, 113, 255};
/// @brief Timer digits.
inline constexpr Color kTimerDigits{251, 191, 36, 255};

/// @brief Flag cloth.
inline constexpr Color kFlag{239, 68, 68, 255};
/// @brief Flag pole and base.
inline constexpr Color kFlagPole{203, 213, 225, 255};
/// @brief Mine body.
inline constexpr Color kMine{14, 15, 22, 255};
/// @brief Cross drawn over a flag that was placed on a safe cell.
inline constexpr Color kWrongFlag{250, 204, 21, 255};

/// @brief Reset-button face.
inline constexpr Color kFace{250, 204, 21, 255};
/// @brief Eyes and mouth on the reset-button face.
inline constexpr Color kFaceFeatures{30, 27, 20, 255};

/// @brief Colour of each adjacent-mine number, indexed by the number
/// (index 0 is unused because zero cells show no number).
///
/// `std::array<Color, 9>` instead of a C array `Color[9]`: it carries its
/// size and can be `constexpr` and passed around like any value.
inline constexpr std::array<Color, 9> kNumberColors{{
    {0, 0, 0, 0},          // 0: never drawn.
    {96, 165, 250, 255},   // 1: blue.
    {74, 222, 128, 255},   // 2: green.
    {248, 113, 113, 255},  // 3: red.
    {167, 139, 250, 255},  // 4: violet.
    {251, 146, 60, 255},   // 5: orange.
    {45, 212, 191, 255},   // 6: teal.
    {244, 244, 245, 255},  // 7: white.
    {148, 163, 184, 255},  // 8: grey.
}};

}  // namespace minesweeper

#endif  // MINESWEEPER_THEME_H_
