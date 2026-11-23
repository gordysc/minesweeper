#include "renderer.h"

#include <raylib.h>

#include <algorithm>
#include <cmath>

#include "screen.h"
#include "theme.h"

/// @brief Everything that belongs to the Minesweeper game.
namespace minesweeper {

/// @brief Drawing helpers internal to `renderer.cpp`.
///
/// They are free functions rather than private members because they need no
/// Renderer state; keeping them out of the header also means changing one
/// never forces other files to recompile.
namespace {

/// @brief Corner roundness of a tile (fraction of its shorter side).
constexpr float kTileRoundness = 0.22f;
/// @brief Number of segments used to draw each rounded corner.
constexpr int kCornerSegments = 6;

/// @brief Returns `color` with its alpha scaled by `factor`.
/// @param color Source colour.
/// @param factor `0.0` (transparent) to `1.0` (unchanged).
/// @return Faded copy (the parameter is a copy, so editing it is safe).
Color WithAlpha(Color color, float factor) {
  color.a = static_cast<unsigned char>(static_cast<float>(color.a) *
                                       std::clamp(factor, 0.0f, 1.0f));
  return color;
}

/// @brief Shrinks `rect` by `amount` on every side.
/// @param rect Rectangle to shrink.
/// @param amount Pixels to remove from each edge.
/// @return The smaller rectangle, sharing the same centre.
Rectangle Inset(Rectangle rect, float amount) {
  return {rect.x + amount, rect.y + amount, rect.width - 2.0f * amount,
          rect.height - 2.0f * amount};
}

/// @brief Scales `rect` about its centre.
/// @param rect Rectangle to scale.
/// @param scale `1.0` keeps the size; `0.0` collapses it to a point.
/// @return The scaled rectangle.
Rectangle ScaleAboutCentre(Rectangle rect, float scale) {
  const float w = rect.width * scale;
  const float h = rect.height * scale;
  return {rect.x + (rect.width - w) / 2.0f, rect.y + (rect.height - h) / 2.0f,
          w, h};
}

/// @brief Returns the centre point of `rect`.
/// @param rect Any rectangle.
/// @return Its centre.
Vector2 Centre(Rectangle rect) {
  return {rect.x + rect.width / 2.0f, rect.y + rect.height / 2.0f};
}

/// @brief "Ease out back" curve: rushes past 1.0 slightly, then settles,
/// which gives pop-ins a little bounce.
/// @param t Linear progress in `[0, 1]`.
/// @return Eased progress (peaks around 1.1).
float EaseOutBack(float t) {
  constexpr float kOvershoot = 1.70158f;
  const float u = t - 1.0f;
  return 1.0f + (kOvershoot + 1.0f) * u * u * u + kOvershoot * u * u;
}

/// @brief Draws `text` centred on `centre` with raylib's default font.
/// @param text Null-terminated string (raylib is a C API, so it takes
///        `const char*` rather than `std::string`).
/// @param centre Point to centre the text on.
/// @param size Font size in pixels.
/// @param color Text colour.
void DrawTextCentred(const char* text, Vector2 centre, int size, Color color) {
  const int width = MeasureText(text, size);
  DrawText(text, static_cast<int>(centre.x) - width / 2,
           static_cast<int>(centre.y) - size / 2, size, color);
}

/// @brief Draws a raised, covered tile: a darker lip with a lighter face on
/// top, offset upwards so the tile looks like it sticks out.
/// @param tile Tile bounds.
/// @param face Face colour.
/// @param alpha Overall opacity (`0..1`), used while a cover fades away.
void DrawCoveredTile(Rectangle tile, Color face, float alpha) {
  const float lip = std::max(2.0f, tile.height * 0.09f);
  DrawRectangleRounded(tile, kTileRoundness, kCornerSegments,
                       WithAlpha(kTileShadow, alpha));
  const Rectangle top{tile.x, tile.y, tile.width, tile.height - lip};
  DrawRectangleRounded(top, kTileRoundness, kCornerSegments,
                       WithAlpha(face, alpha));
}

/// @brief Draws a mine: a round body with eight spikes and a glint.
/// @param centre Mine centre.
/// @param radius Body radius in pixels.
void DrawMine(Vector2 centre, float radius) {
  if (radius <= 0.5f) return;
  const float spike = radius * 1.45f;
  const float thick = std::max(1.5f, radius * 0.3f);
  // Four lines through the centre make the eight spikes. Diagonals are a
  // little shorter so the outline reads as round.
  DrawLineEx({centre.x - spike, centre.y}, {centre.x + spike, centre.y}, thick,
             kMine);
  DrawLineEx({centre.x, centre.y - spike}, {centre.x, centre.y + spike}, thick,
             kMine);
  const float diag = spike * 0.78f;
  DrawLineEx({centre.x - diag, centre.y - diag},
             {centre.x + diag, centre.y + diag}, thick, kMine);
  DrawLineEx({centre.x - diag, centre.y + diag},
             {centre.x + diag, centre.y - diag}, thick, kMine);
  DrawCircleV(centre, radius, kMine);
  DrawCircleV({centre.x - radius * 0.35f, centre.y - radius * 0.35f},
              radius * 0.28f, Color{255, 255, 255, 170});
}

/// @brief Draws a flag on a pole with a small base.
/// @param centre Tile centre.
/// @param size Tile size in pixels; the flag is scaled from it.
void DrawFlag(Vector2 centre, float size) {
  if (size <= 1.0f) return;
  const float pole_x = centre.x + size * 0.1f;
  const float top = centre.y - size * 0.3f;
  const float bottom = centre.y + size * 0.22f;
  DrawLineEx({pole_x, top}, {pole_x, bottom}, std::max(1.5f, size * 0.07f),
             kFlagPole);
  DrawRectangleRounded({centre.x - size * 0.2f, bottom - size * 0.02f,
                        size * 0.44f, size * 0.1f},
                       0.5f, 4, kFlagPole);
  // raylib only fills triangles whose vertices are listed counter-clockwise
  // as seen on screen (the others are culled as "back-facing"), so the order
  // is top of pole -> flag tip -> bottom of cloth.
  DrawTriangle({pole_x, top}, {pole_x - size * 0.36f, top + size * 0.15f},
               {pole_x, top + size * 0.3f}, kFlag);
}

/// @brief Draws one cell of the board.
/// @param board Board being drawn.
/// @param x Column index.
/// @param y Row index.
/// @param rect Cell bounds on screen.
/// @param progress Pop-in progress from the RevealAnimator (`0..1`).
/// @param pressed True if the cell is held down by the mouse.
/// @param hovered True if the mouse is over the cell.
void DrawCell(const Board& board, int x, int y, Rectangle rect, float progress,
              bool pressed, bool hovered) {
  const Cell& cell = board.At(x, y);
  const float gap = std::max(1.0f, std::round(rect.width * 0.045f));
  const Rectangle tile = Inset(rect, gap);
  const Vector2 centre = Centre(tile);
  const float size = tile.width;

  const bool lost = board.status() == GameStatus::Lost;
  const bool shows_mine = lost && cell.mine && cell.state == CellState::Hidden;
  const bool wrong_flag =
      lost && !cell.mine && cell.state == CellState::Flagged;
  // `exploded()` returns a `std::optional`; comparing an optional with a
  // plain value is true only when it holds a value equal to it.
  const bool exploded = board.exploded() == GridPoint{x, y};

  if (cell.state == CellState::Revealed || shows_mine) {
    Color background = kTileRevealed;
    if (shows_mine) background = exploded ? kTileExploded : kTileMine;
    DrawRectangleRounded(tile, kTileRoundness, kCornerSegments, background);
    if (shows_mine) {
      DrawMine(centre, size * 0.22f * EaseOutBack(progress));
    } else if (cell.adjacent > 0) {
      const char* digit = TextFormat("%d", cell.adjacent);
      const int font = static_cast<int>(size * 0.62f);
      const Color color = WithAlpha(kNumberColors[cell.adjacent], progress);
      // Drawing twice, one pixel apart, fakes a bold weight with raylib's
      // thin default font.
      DrawTextCentred(digit, centre, font, color);
      DrawTextCentred(digit, {centre.x + 1.0f, centre.y}, font, color);
    }
    // While the pop-in runs, the old cover shrinks and fades away on top.
    if (progress < 1.0f) {
      DrawCoveredTile(ScaleAboutCentre(tile, 1.0f - progress), kTileFace,
                      1.0f - progress);
    }
    return;
  }

  if (pressed) {
    DrawRectangleRounded(tile, kTileRoundness, kCornerSegments, kTilePressed);
  } else {
    DrawCoveredTile(
        tile, hovered && !board.is_over() ? kTileFaceHover : kTileFace, 1.0f);
  }
  if (cell.state == CellState::Flagged) {
    // Flags auto-placed on a win pop in like everything else.
    const float scale = progress < 1.0f ? EaseOutBack(progress) : 1.0f;
    // Nudge the flag up so it sits on the raised face, not across the lip.
    DrawFlag({centre.x, centre.y - size * 0.04f}, size * scale);
    if (wrong_flag) {
      // A flag on a safe cell gets a cross once the game is lost.
      const float r = size * 0.32f;
      const float t = std::max(2.0f, size * 0.08f);
      DrawLineEx({centre.x - r, centre.y - r}, {centre.x + r, centre.y + r}, t,
                 kWrongFlag);
      DrawLineEx({centre.x - r, centre.y + r}, {centre.x + r, centre.y - r}, t,
                 kWrongFlag);
    }
  }
}

/// @brief Draws a counter box with a small label above it.
/// @param rect Box bounds.
/// @param label Caption drawn above the box (e.g. `"MINES"`).
/// @param value Number to show, clamped to `[-99, 999]` to fit three digits.
/// @param color Digit colour.
void DrawCounter(Rectangle rect, const char* label, int value, Color color) {
  DrawText(label, static_cast<int>(rect.x), static_cast<int>(rect.y) - 16, 12,
           kTextMuted);
  DrawRectangleRounded(rect, 0.25f, 8, kCounterBackground);
  // `%03d` pads with zeros to three digits, like a classic seven-segment
  // display. A negative value prints as e.g. `-05`.
  DrawTextCentred(TextFormat("%03d", std::clamp(value, -99, 999)), Centre(rect),
                  36, color);
}

/// @brief Draws the reset button and the face whose expression follows the
/// game: smiling, nervous while a cell is held, sunglasses on a win, and
/// crossed-out eyes on a loss.
/// @param board Board whose status picks the expression.
/// @param view Frame state (for press and hover).
void DrawFaceButton(const Board& board, const FrameView& view) {
  const Rectangle button = FaceButtonRect();
  const bool hovered = CheckCollisionPointRec(view.mouse, button);
  const Color fill =
      view.face_pressed ? kCounterBackground : (hovered ? kPanelHover : kPanel);
  DrawRectangleRounded(button, 0.3f, 8, fill);
  DrawRectangleRoundedLinesEx(button, 0.3f, 8, 2.0f, kPanelHover);

  // Push the face down a pixel while pressed so the button feels clicked.
  Vector2 c = Centre(button);
  if (view.face_pressed) c.y += 2.0f;
  const float r = button.width * 0.34f;
  DrawCircleV(c, r, kFace);

  const Vector2 left_eye{c.x - r * 0.36f, c.y - r * 0.22f};
  const Vector2 right_eye{c.x + r * 0.36f, c.y - r * 0.22f};
  const float eye = r * 0.13f;

  switch (board.status()) {
    case GameStatus::Lost: {
      // Braces give this `case` its own scope, which C++ requires when a
      // case declares local variables.
      const float t = std::max(1.5f, r * 0.1f);
      for (const Vector2& e : {left_eye, right_eye}) {
        DrawLineEx({e.x - eye, e.y - eye}, {e.x + eye, e.y + eye}, t,
                   kFaceFeatures);
        DrawLineEx({e.x - eye, e.y + eye}, {e.x + eye, e.y - eye}, t,
                   kFaceFeatures);
      }
      // raylib angles are in degrees, with 0 pointing right and 90 pointing
      // *down* (screen y grows downwards). 200..340 is the top of a circle,
      // i.e. a frown when the circle sits below the eyes.
      DrawRing({c.x, c.y + r * 0.72f}, r * 0.34f, r * 0.45f, 205.0f, 335.0f, 16,
               kFaceFeatures);
      break;
    }
    case GameStatus::Won: {
      const float lens_w = r * 0.42f;
      const float lens_h = r * 0.28f;
      DrawRectangleRounded({left_eye.x - lens_w / 2.0f,
                            left_eye.y - lens_h / 2.0f, lens_w, lens_h},
                           0.5f, 4, kFaceFeatures);
      DrawRectangleRounded({right_eye.x - lens_w / 2.0f,
                            right_eye.y - lens_h / 2.0f, lens_w, lens_h},
                           0.5f, 4, kFaceFeatures);
      DrawLineEx(left_eye, right_eye, std::max(1.5f, r * 0.08f), kFaceFeatures);
      DrawRing(c, r * 0.46f, r * 0.58f, 25.0f, 155.0f, 16, kFaceFeatures);
      break;
    }
    case GameStatus::Ready:
    case GameStatus::Playing:
      DrawCircleV(left_eye, eye, kFaceFeatures);
      DrawCircleV(right_eye, eye, kFaceFeatures);
      if (view.board_pressed) {
        // A small "o" mouth while the player is about to commit to a cell.
        DrawCircleV({c.x, c.y + r * 0.4f}, r * 0.17f, kFaceFeatures);
      } else {
        DrawRing(c, r * 0.46f, r * 0.58f, 25.0f, 155.0f, 16, kFaceFeatures);
      }
      break;
  }
}

/// @brief Draws the header panel: mine counter, record lines, face, timer.
/// @param board Board being played.
/// @param view Frame state.
void DrawHeader(const Board& board, const FrameView& view) {
  DrawRectangleRounded(HeaderRect(), 0.18f, 8, kPanel);
  const Rectangle mines = MineCounterRect();
  const Rectangle timer = TimerRect();
  DrawCounter(mines, "MINES", board.mines_remaining(), kMineDigits);
  // The visible clock shows whole seconds, like the classic, but the exact
  // time is kept for best times.
  DrawCounter(timer, "TIME", static_cast<int>(view.elapsed), kTimerDigits);

  const int played = view.record.wins + view.record.losses;
  const char* wins = TextFormat("WON %d OF %d", view.record.wins, played);
  DrawText(wins, static_cast<int>(mines.x + mines.width + 20.0f),
           static_cast<int>(mines.y + 16.0f), 16, kTextMuted);

  const char* best = view.record.best_time > 0.0f
                         ? TextFormat("BEST %.2f s", view.record.best_time)
                         : "BEST --";
  const int best_width = MeasureText(best, 16);
  DrawText(best, static_cast<int>(timer.x - 20.0f) - best_width,
           static_cast<int>(timer.y + 16.0f), 16,
           view.new_record ? kSuccess : kTextMuted);

  DrawFaceButton(board, view);
}

/// @brief Draws the three difficulty tabs, highlighting the active one.
/// @param view Frame state (active difficulty and mouse position).
void DrawTabs(const FrameView& view) {
  for (int i = 0; i < kDifficultyCount; ++i) {
    const auto difficulty = static_cast<Difficulty>(i);
    const Rectangle tab = DifficultyTabRect(difficulty);
    const bool active = difficulty == view.difficulty;
    const bool hovered = CheckCollisionPointRec(view.mouse, tab);
    const Color fill = active ? kAccent : (hovered ? kPanelHover : kPanel);
    DrawRectangleRounded(tab, 0.35f, 8, fill);

    const BoardConfig config = ConfigFor(difficulty);
    const char* label =
        TextFormat("%d  %s  %dx%d", i + 1, DifficultyName(difficulty),
                   config.width, config.height);
    DrawTextCentred(label, Centre(tab), 14, active ? kBackground : kText);
  }
}

/// @brief Draws the line under the board: controls while playing, the result
/// once the game ends.
/// @param board Board being played.
/// @param view Frame state.
void DrawFooter(const Board& board, const FrameView& view) {
  const Vector2 centre{static_cast<float>(kScreenWidth) / 2.0f,
                       FooterY() + 10.0f};
  switch (board.status()) {
    case GameStatus::Won: {
      const char* message =
          view.new_record
              ? TextFormat("Cleared in %.2f s  -  new best time!", view.elapsed)
              : TextFormat("Cleared in %.2f s.  Press R to play again.",
                           view.elapsed);
      DrawTextCentred(message, centre, 22, kSuccess);
      break;
    }
    case GameStatus::Lost:
      DrawTextCentred("Boom.  Press R or click the face to try again.", centre,
                      22, kDanger);
      break;
    case GameStatus::Ready:
    case GameStatus::Playing:
      DrawTextCentred(
          "LEFT CLICK reveal    RIGHT CLICK flag    CLICK A NUMBER chord    "
          "R new game    ARROWS + SPACE / F keyboard play",
          centre, 14, kTextMuted);
      break;
  }
}

}  // namespace

void Renderer::Draw(const Board& board, const BoardLayout& layout,
                    const RevealAnimator& animator,
                    const FrameView& view) const {
  ClearBackground(kBackground);
  DrawHeader(board, view);
  DrawTabs(view);

  // Copy the layout and nudge the copy for the loss shake. `layout` itself is
  // a `const&`, so it can't (and shouldn't) be modified here.
  BoardLayout shaken = layout;
  shaken.origin_x += view.shake.x;
  shaken.origin_y += view.shake.y;

  DrawRectangleRounded(Inset(shaken.Bounds(), -10.0f), 0.03f, 8, kPanel);
  for (int y = 0; y < board.height(); ++y) {
    for (int x = 0; x < board.width(); ++x) {
      const GridPoint p{x, y};
      // `std::find` walks the span and returns `end()` when nothing matches.
      // At most 9 cells are pressed, so a linear scan is cheapest.
      const bool pressed = std::find(view.pressed.begin(), view.pressed.end(),
                                     p) != view.pressed.end();
      DrawCell(board, x, y, shaken.CellRect(x, y),
               animator.Progress(x, y, view.now), pressed, view.hover == p);
    }
  }

  if (view.cursor.has_value()) {
    // `->` on an optional reaches the stored value's members, like a pointer.
    const Rectangle r =
        Inset(shaken.CellRect(view.cursor->x, view.cursor->y), -1.0f);
    // A gentle pulse makes the cursor easy to find on a big board.
    const float pulse = 0.65f + 0.35f * std::sin(view.now * 6.0f);
    DrawRectangleRoundedLinesEx(r, kTileRoundness, kCornerSegments, 3.0f,
                                WithAlpha(kCursor, pulse));
  }

  DrawFooter(board, view);
}

}  // namespace minesweeper
