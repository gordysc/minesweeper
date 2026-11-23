#include "game.h"

#include <raylib.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>

/// @brief Everything that belongs to the Minesweeper game.
namespace minesweeper {

/// @brief Constants and helpers internal to `game.cpp`.
namespace {

/// @brief How long the board shakes after a loss, in seconds.
constexpr float kShakeSeconds = 0.4f;
/// @brief Largest shake offset in pixels.
constexpr float kShakePixels = 7.0f;

/// @brief Offsets from a cell to its 8 neighbours, for the chord preview.
constexpr std::array<GridPoint, 8> kNeighbours{{
    {-1, -1},
    {0, -1},
    {1, -1},
    {-1, 0},
    {1, 0},
    {-1, 1},
    {0, 1},
    {1, 1},
}};

/// @brief Returns true if the mouse pointer actually moved this frame.
///
/// Gating "hide the keyboard cursor" behind real motion stops a resting
/// mouse from fighting keyboard play.
/// @return True when horizontal or vertical mouse delta exceeds half a pixel.
bool MouseMoved() {
  const Vector2 delta = GetMouseDelta();
  return std::abs(delta.x) > 0.5f || std::abs(delta.y) > 0.5f;
}

/// @brief Returns true on the first frame `key` is pressed and on the
/// operating system's key-repeat frames while it is held.
/// @param key raylib key code.
/// @return True if the key should act this frame.
bool KeyPressedOrRepeated(int key) {
  return IsKeyPressed(key) || IsKeyPressedRepeat(key);
}

}  // namespace

Game::Game() : save_path_(DefaultSavePath()), save_(LoadSaveData(save_path_)) {
  // Mix wall-clock time into the seed counter so every launch deals fresh
  // boards. `time_since_epoch().count()` is the raw tick count.
  const auto now_ticks = static_cast<std::uint64_t>(
      std::chrono::steady_clock::now().time_since_epoch().count());
  seed_counter_ = static_cast<std::uint32_t>(now_ticks ^ (now_ticks >> 32U));

  difficulty_ = save_.difficulty;
  StartNewGame();
}

Game::~Game() { PersistSaveData(); }

void Game::Update(float dt) {
  // Clamp so a long stall (window drag, breakpoint) can't add a huge jump.
  dt = std::clamp(dt, 0.0f, 0.1f);
  now_ += dt;
  if (board_.status() == GameStatus::Playing) elapsed_ += dt;
  shake_timer_ = std::max(0.0f, shake_timer_ - dt);

  HandleKeyboard();
  HandleMouse();
}

void Game::Draw() const {
  FrameView view;
  view.difficulty = difficulty_;
  view.elapsed = elapsed_;
  view.now = now_;
  view.record = CurrentRecord();
  view.new_record = new_record_;
  view.hover = keyboard_active_ ? std::nullopt : hover_;
  // `std::optional<T>(value)` wraps a value; the ternary picks either a
  // cursor or "no cursor".
  view.cursor =
      keyboard_active_ ? std::optional<GridPoint>(cursor_) : std::nullopt;
  // A span made from a vector points at the vector's buffer. That is safe
  // here because `view` dies at the end of this function, before
  // `pressed_` can next change.
  view.pressed = pressed_;
  view.face_pressed = face_armed_ && CheckCollisionPointRec(GetMousePosition(),
                                                            FaceButtonRect());
  view.board_pressed = !pressed_.empty();
  view.mouse = GetMousePosition();
  if (shake_timer_ > 0.0f) {
    // Two sine waves at different speeds give a rattle that dies down as
    // the timer runs out.
    const float strength = kShakePixels * (shake_timer_ / kShakeSeconds);
    view.shake = {strength * std::sin(now_ * 83.0f),
                  strength * std::cos(now_ * 67.0f)};
  }
  renderer_.Draw(board_, layout_, animator_, view);
}

void Game::StartNewGame() {
  board_.Reset(ConfigFor(difficulty_), NextSeed());
  layout_ = ComputeBoardLayout(board_.width(), board_.height());
  animator_.Reset(board_.width(), board_.height());
  elapsed_ = 0.0f;
  shake_timer_ = 0.0f;
  result_recorded_ = false;
  new_record_ = false;
  pressed_.clear();
  board_armed_ = false;
  face_armed_ = false;
  cursor_ = {board_.width() / 2, board_.height() / 2};
}

void Game::SetDifficulty(Difficulty difficulty) {
  difficulty_ = difficulty;
  save_.difficulty = difficulty;
  PersistSaveData();
  StartNewGame();
}

void Game::HandleMouse() {
  const Vector2 mouse = GetMousePosition();
  if (MouseMoved()) keyboard_active_ = false;
  hover_ = layout_.CellAt(mouse);
  const bool over_face = CheckCollisionPointRec(mouse, FaceButtonRect());

  if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
    for (int i = 0; i < kDifficultyCount; ++i) {
      const auto difficulty = static_cast<Difficulty>(i);
      if (CheckCollisionPointRec(mouse, DifficultyTabRect(difficulty))) {
        SetDifficulty(difficulty);
        return;
      }
    }
    // Like the classic, a click acts on *release*, so the player can press,
    // see the preview, and slide off to cancel. These flags remember where
    // the press began.
    face_armed_ = over_face;
    board_armed_ = hover_.has_value();
  }

  // Press preview: a held left button pushes in the covered cell under it;
  // a held button on a revealed number pushes in its covered neighbours
  // (what a chord would open).
  pressed_.clear();
  const bool left_held = IsMouseButtonDown(MOUSE_BUTTON_LEFT) && board_armed_;
  const bool middle_held = IsMouseButtonDown(MOUSE_BUTTON_MIDDLE);
  if (hover_ && !board_.is_over() && (left_held || middle_held)) {
    // `*hover_` reads the value inside the optional; safe because the `if`
    // just checked that it holds one.
    const Cell& cell = board_.At(hover_->x, hover_->y);
    if (cell.state == CellState::Hidden && left_held) {
      pressed_.push_back(*hover_);
    } else if (cell.state == CellState::Revealed && cell.adjacent > 0) {
      for (const GridPoint& d : kNeighbours) {
        const GridPoint n{hover_->x + d.x, hover_->y + d.y};
        if (board_.InBounds(n.x, n.y) &&
            board_.At(n.x, n.y).state == CellState::Hidden) {
          pressed_.push_back(n);
        }
      }
    }
  }

  if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
    if (face_armed_ && over_face) {
      StartNewGame();
    } else if (board_armed_ && hover_) {
      OpenCell(*hover_);
    }
    face_armed_ = false;
    board_armed_ = false;
  }
  if (IsMouseButtonReleased(MOUSE_BUTTON_MIDDLE) && hover_) {
    OpenCell(*hover_);
  }
  // Flags act on press: there is nothing to preview or cancel.
  if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) && hover_) {
    FlagCell(*hover_);
  }
}

void Game::HandleKeyboard() {
  if (IsKeyPressed(KEY_R) || IsKeyPressed(KEY_F2)) StartNewGame();
  if (IsKeyPressed(KEY_ONE)) SetDifficulty(Difficulty::Beginner);
  if (IsKeyPressed(KEY_TWO)) SetDifficulty(Difficulty::Intermediate);
  if (IsKeyPressed(KEY_THREE)) SetDifficulty(Difficulty::Expert);

  int dx = 0;
  int dy = 0;
  if (KeyPressedOrRepeated(KEY_LEFT) || KeyPressedOrRepeated(KEY_A)) --dx;
  if (KeyPressedOrRepeated(KEY_RIGHT) || KeyPressedOrRepeated(KEY_D)) ++dx;
  if (KeyPressedOrRepeated(KEY_UP) || KeyPressedOrRepeated(KEY_W)) --dy;
  if (KeyPressedOrRepeated(KEY_DOWN) || KeyPressedOrRepeated(KEY_S)) ++dy;
  if (dx != 0 || dy != 0) {
    // The first key press only shows the cursor where it was; after that,
    // keys move it.
    if (keyboard_active_) {
      cursor_.x = std::clamp(cursor_.x + dx, 0, board_.width() - 1);
      cursor_.y = std::clamp(cursor_.y + dy, 0, board_.height() - 1);
    }
    keyboard_active_ = true;
  }

  if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER)) {
    keyboard_active_ = true;
    OpenCell(cursor_);
  }
  if (IsKeyPressed(KEY_F)) {
    keyboard_active_ = true;
    FlagCell(cursor_);
  }
}

void Game::OpenCell(GridPoint cell) {
  const CellState state = board_.At(cell.x, cell.y).state;
  if (state == CellState::Hidden) {
    board_.Reveal(cell.x, cell.y);
  } else if (state == CellState::Revealed) {
    board_.Chord(cell.x, cell.y);
  }
  AfterMove(cell);
}

void Game::FlagCell(GridPoint cell) {
  board_.ToggleFlag(cell.x, cell.y);
  AfterMove(cell);
}

void Game::AfterMove(GridPoint origin) {
  animator_.Sync(board_, origin, now_);
  if (!board_.is_over() || result_recorded_) return;

  result_recorded_ = true;
  const bool won = board_.status() == GameStatus::Won;
  new_record_ = RecordResult(save_, difficulty_, won, elapsed_);
  if (!won) shake_timer_ = kShakeSeconds;
  PersistSaveData();
}

const DifficultyRecord& Game::CurrentRecord() const {
  return save_.records[static_cast<std::size_t>(difficulty_)];
}

void Game::PersistSaveData() { WriteSaveData(save_path_, save_); }

std::uint32_t Game::NextSeed() {
  // Multiply by a large odd constant (Knuth's multiplicative hash) so
  // consecutive counters give very different seeds. Unsigned overflow wraps
  // around by definition, so this is well-defined.
  seed_counter_ += 1U;
  return seed_counter_ * 2654435761U;
}

}  // namespace minesweeper
