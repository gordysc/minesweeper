// Header guard: ensures this header's definitions are included at most once
// per translation unit (.cpp file), preventing duplicate-definition errors.
#ifndef MINESWEEPER_SCREEN_H_
#define MINESWEEPER_SCREEN_H_

/// @brief Everything that belongs to the Minesweeper game.
namespace minesweeper {

// `inline constexpr` (C++17) on a header variable gives it external linkage
// across every .cpp file that includes this header while still remaining a
// compile-time constant. Without `inline`, each translation unit would get its
// own separate internal copy.

/// @brief Window width in logical pixels.
inline constexpr int kScreenWidth = 1100;
/// @brief Window height in logical pixels.
inline constexpr int kScreenHeight = 780;

}  // namespace minesweeper

#endif  // MINESWEEPER_SCREEN_H_
