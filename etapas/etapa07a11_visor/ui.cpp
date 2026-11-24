// ui.cpp — desenho das telas e tratamento dos toques.
#include "ui.h"
#include <stdio.h>
#include <string.h>
#include <algorithm>

using namespace mtg;

// ---------------------------------------------------------------- cores (RGB 0xRRGGBB)
static constexpr uint32_t C_BG        = 0x061055;  // fundo azul-escuro
static constexpr uint32_t C_PANEL     = 0x1636D8;  // painel azul
static constexpr uint32_t C_PANEL_OUT = 0x2A2F5C;  // painel de jogador eliminado
static constexpr uint32_t C_DARK      = 0x0B1E8C;  // botões
static constexpr uint32_t C_BOX       = 0x3A66FF;  // caixa do número de vida
static constexpr uint32_t C_LIME      = 0xA6FF3C;  // contornos verde-limão
static constexpr uint32_t C_CYAN      = 0x3FD2FF;  // selecionado
static constexpr uint32_t C_WHITE     = 0xF2F7FF;
static constexpr uint32_t C_DIM       = 0x8C9AD0;
static constexpr uint32_t C_NAVY      = 0x041040;
static constexpr uint32_t C_RED       = 0xFF4040;
static constexpr uint32_t C_ORANGE    = 0xFFB030;
static constexpr uint32_t C_POISON    = 0x6CFF6C;
static constexpr uint32_t C_ENERGY    = 0xFFE040;
static constexpr uint32_t C_PLAYER[4] = {0x5EC8FF, 0x3FE0C8, 0xA6FF3C, 0xC8B4FF};

static constexpr int W = 800, H = 480;
static const uint8_t BRIGHT[] = {50, 110, 180, 255};

// fontes (já vêm na biblioteca LovyanGFX)
#define F_S9   (&fonts::FreeSansBold9pt7b)
#define F_S12  (&fonts::FreeSansBold12pt7b)
#define F_S18  (&fonts::FreeSansBold18pt7b)
#define F_S24  (&fonts::FreeSansBold24pt7b)
#define F_MONO (&fonts::FreeMonoBold9pt7b)
#define F_SEG  (&fonts::Font7)   // números estilo display de 7 segmentos

enum { ICON_CMDR, ICON_ENERGY, ICON_POISON };

// ---------------------------------------------------------------- utilitários de desenho
static void text(lgfx::LovyanGFX& g, const char* s, int x, int y, const lgfx::IFont* f,
                 uint32_t color, textdatum_t datum = textdatum_t::middle_center, float size = 1) {
  g.setFont(f);
  g.setTextSize(size);
  g.setTextDatum(datum);
  g.setTextColor(color);
  g.drawString(s, x, y);
}

static void frame(lgfx::LovyanGFX& g, int x, int y, int w, int h, int r, uint32_t fill, uint32_t border) {
  g.fillRoundRect(x, y, w, h, r, fill);
  g.drawRoundRect(x, y, w, h, r, border);
  g.drawRoundRect(x + 1, y + 1, w - 2, h - 2, r > 1 ? r - 1 : r, border);
}

static void button(lgfx::LovyanGFX& g, int x, int y, int w, int h, const char* label,
                   const lgfx::IFont* f, bool selected = false, bool enabled = true) {
  frame(g, x, y, w, h, 6, selected ? C_CYAN : C_DARK, enabled ? C_LIME : C_DIM);
  text(g, label, x + w / 2, y + h / 2 + 1, f, selected ? C_NAVY : (enabled ? C_WHITE : C_DIM));
}

static void drawMinus(lgfx::LovyanGFX& g, int cx, int cy, int s, uint32_t c) {
  g.fillRect(cx - s, cy - s / 5, 2 * s, 2 * (s / 5) + 1, c);
}
static void drawPlus(lgfx::LovyanGFX& g, int cx, int cy, int s, uint32_t c) {
  drawMinus(g, cx, cy, s, c);
  g.fillRect(cx - s / 5, cy - s, 2 * (s / 5) + 1, 2 * s, c);
}

static void drawIcon(lgfx::LovyanGFX& g, int icon, int cx, int cy, int s, uint32_t c) {
  switch (icon) {
    case ICON_CMDR:  // coroa
      g.fillRect(cx - s, cy + s / 3, 2 * s, s / 2, c);
      g.fillTriangle(cx - s, cy + s / 3, cx - s, cy - s, cx - s / 3, cy + s / 3, c);
      g.fillTriangle(cx - s / 2, cy + s / 3, cx, cy - s, cx + s / 2, cy + s / 3, c);
      g.fillTriangle(cx + s / 3, cy + s / 3, cx + s, cy - s, cx + s, cy + s / 3, c);
      break;
    case ICON_ENERGY:  // raio
      g.fillTriangle(cx + s / 3, cy - s, cx - s * 2 / 3, cy + s / 6, cx + s / 8, cy + s / 6, c);
      g.fillTriangle(cx - s / 3, cy + s, cx + s * 2 / 3, cy - s / 6, cx - s / 8, cy - s / 6, c);
      break;
    default:  // gota (veneno)
      g.fillCircle(cx, cy + s / 3, s * 2 / 3, c);
      g.fillTriangle(cx, cy - s, cx - s * 2 / 3 + 1, cy + s / 4, cx + s * 2 / 3 - 1, cy + s / 4, c);
      break;
  }
}

// ---------------------------------------------------------------- ciclo principal
App::App(LGFX_Sprite& canvas, PlatformHooks hooks)
    : canvas_(canvas), panel_(&canvas), hooks_(hooks) {}

void App::begin() {
  panel_.setColorDepth(16);
  panel_.setPsram(true);
  if (hooks_.setBrightness) hooks_.setBrightness(BRIGHT[brightIdx_]);
  dirty_ = true;
}

bool App::animating(uint32_t now) const {
  if (screen_ != Screen::Play) return false;
  if (firstPick_ >= 0 && now - firstStart_ < 4000) return true;
  for (int i = 0; i < game_.numPlayers(); i++)
    if (game_.recentDelta(i, now) != 0) return true;
  return false;
}

void App::update(bool touched, int x, int y, uint32_t now) {
  game_.tick(now);

  if (touched && !down_) {                  // dedo acabou de encostar
    down_ = true;
    const Hit* h = hitAt(x, y);
    if (h) {
      held_ = *h;
      holding_ = h->repeat;
      repeatCount_ = 0;
      nextRepeat_ = now + 450;
      perform(*h, 1, now);
      dirty_ = true;
    }
  } else if (touched && down_ && holding_) { // segurando um botão de +/-
    bool inside = x >= held_.r.x && x < held_.r.x + held_.r.w && y >= held_.r.y && y < held_.r.y + held_.r.h;
    if (!inside) {
      holding_ = false;
    } else if ((int32_t)(now - nextRepeat_) >= 0) {
      repeatCount_++;
      perform(held_, repeatCount_ >= 10 ? 5 : 1, now);  // segurando muito tempo: de 5 em 5
      nextRepeat_ = now + 140;
      dirty_ = true;
    }
  } else if (!touched && down_) {           // dedo saiu
    down_ = false;
    holding_ = false;
  }

  static bool wasAnimating = false;
  bool anim = animating(now);
  if (anim || wasAnimating) {
    if (now - lastFrame_ > 120) dirty_ = true;
  }
  wasAnimating = anim;

  if (dirty_) {
    dirty_ = false;
    lastFrame_ = now;
    render(now);
    if (hooks_.present) hooks_.present();
  }
}

// ---------------------------------------------------------------- toques
void App::addHit(Rect r, Action act, int a, int b, bool repeat) {
  if (hitCount_ >= (int)(sizeof(hits_) / sizeof(hits_[0]))) return;
  hits_[hitCount_++] = Hit{r, (uint8_t)act, (int8_t)a, (int8_t)b, repeat};
}

// Converte uma área do painel (coordenadas do próprio painel) para coordenadas da tela,
// levando em conta se o painel está de cabeça para baixo.
void App::addPanelHit(Rect p, bool rotated, Rect l, Action act, int a, int b, bool repeat) {
  Rect s;
  if (rotated) s = Rect{(int16_t)(p.x + p.w - (l.x + l.w)), (int16_t)(p.y + p.h - (l.y + l.h)), l.w, l.h};
  else         s = Rect{(int16_t)(p.x + l.x), (int16_t)(p.y + l.y), l.w, l.h};
  addHit(s, act, a, b, repeat);
}

const App::Hit* App::hitAt(int x, int y) const {
  for (int i = hitCount_ - 1; i >= 0; i--) {   // os últimos desenhados ficam por cima
    const Rect& r = hits_[i].r;
    if (x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h) return &hits_[i];
  }
  return nullptr;
}

void App::perform(const Hit& h, int step, uint32_t now) {
  switch (h.action) {
    case A_MENU_CMDR:   screen_ = Screen::Setup; break;
    case A_MENU_STD:    game_.newGame(Format::Standard, 2, 20); locked_ = false; firstPick_ = -1; screen_ = Screen::Play; break;
    case A_MENU_RESUME: screen_ = Screen::Play; break;
    case A_MENU_BRIGHT:
      brightIdx_ = (brightIdx_ + 1) % (int)sizeof(BRIGHT);
      if (hooks_.setBrightness) hooks_.setBrightness(BRIGHT[brightIdx_]);
      break;
    case A_SETUP_LIFE:  setupLife_ = h.a; break;
    case A_SETUP_PLAYERS:
      game_.newGame(Format::Commander, h.a, setupLife_);
      locked_ = false; firstPick_ = -1; screen_ = Screen::Play;
      break;
    case A_SETUP_BACK:  screen_ = Screen::Menu; break;

    case A_LIFE:        game_.change(h.a, Counter::Life, h.b * step, now); break;
    case A_TOP_LOG:     game_.flush(); logPage_ = 0; screen_ = Screen::Log; break;
    case A_TOP_LOCK:    locked_ = !locked_; break;
    case A_TOP_UNDO:    game_.undo(); break;
    case A_TOP_FIRST: {
      int n = game_.numPlayers();
      firstPick_ = (int)(hooks_.randomN ? hooks_.randomN(n) : 0);
      firstStart_ = now;
      char buf[32];
      snprintf(buf, sizeof buf, "P%d GOES FIRST", firstPick_ + 1);
      game_.event(buf);
      break;
    }
    case A_TOP_RESET:   overlay_ = Overlay::ConfirmReset; break;
    case A_TOP_MENU:    overlay_ = Overlay::ConfirmMenu; break;
    case A_ICON_CMDR: {
      game_.flush();
      int n = game_.numPlayers();
      if (cmdTarget_ >= n) cmdTarget_ = n > 1 ? 1 : 0;
      if (cmdFrom_ >= n || cmdFrom_ == cmdTarget_) cmdFrom_ = (cmdTarget_ + n - 1) % n;
      cmdDmg_ = 0;
      screen_ = Screen::Cmdr;
      break;
    }
    case A_ICON_POISON: game_.flush(); counterKind_ = Counter::Poison; screen_ = Screen::Counters; break;
    case A_ICON_ENERGY: game_.flush(); counterKind_ = Counter::Energy; screen_ = Screen::Counters; break;

    case A_COUNTER:      game_.change(h.a, counterKind_, h.b * step, now); break;
    case A_COUNTER_BACK: game_.flush(); screen_ = Screen::Play; break;

    case A_CMD_TARGET:  cmdTarget_ = h.a; cmdDmg_ = 0; break;
    case A_CMD_FROM:    cmdFrom_ = h.a; cmdDmg_ = 0; break;
    case A_CMD_DELTA: {
      int taken = game_.state().p[cmdTarget_].cmdr[cmdFrom_];
      cmdDmg_ += h.b * step;
      if (taken + cmdDmg_ < 0) cmdDmg_ = -taken;
      if (cmdDmg_ > 99) cmdDmg_ = 99;
      break;
    }
    case A_CMD_APPLY:
      game_.applyCommander(cmdTarget_, cmdFrom_, cmdDmg_, now);
      cmdDmg_ = 0;
      break;
    case A_CMD_BACK:    screen_ = Screen::Play; break;

    case A_LOG_NEWER:   if (logPage_ > 0) logPage_--; break;
    case A_LOG_OLDER:   if ((logPage_ + 1) * 10 < game_.logCount()) logPage_++; break;
    case A_LOG_BACK:    screen_ = Screen::Play; break;

    case A_CONFIRM_YES:
      if (overlay_ == Overlay::ConfirmReset) { game_.restart(); locked_ = false; firstPick_ = -1; }
      else { game_.flush(); screen_ = Screen::Menu; }
      overlay_ = Overlay::None;
      break;
    case A_CONFIRM_NO:  overlay_ = Overlay::None; break;
  }
}

// ---------------------------------------------------------------- desenho
void App::render(uint32_t now) {
  hitCount_ = 0;
  canvas_.fillScreen(C_BG);
  switch (screen_) {
    case Screen::Menu:     renderMenu(); break;
    case Screen::Setup:    renderSetup(); break;
    case Screen::Play:     renderPlay(now); break;
    case Screen::Counters: renderCounters(); break;
    case Screen::Cmdr:     renderCmdr(); break;
    case Screen::Log:      renderLog(); break;
  }
  if (overlay_ != Overlay::None) {
    hitCount_ = 0;   // com a caixa de confirmação aberta, só ela responde
    renderConfirm();
  }
}

void App::renderMenu() {
  auto& g = canvas_;
  frame(g, 4, 4, W - 8, H - 8, 12, C_PANEL, C_LIME);
  text(g, "LIFE COUNTER", W / 2, 58, F_S24, C_LIME);
  text(g, "MAGIC: THE GATHERING", W / 2, 100, F_S9, C_WHITE);

  frame(g, 150, 130, 500, 110, 10, C_DARK, C_LIME);
  text(g, "COMMANDER", W / 2, 172, F_S18, C_WHITE);
  text(g, "20 / 30 / 40 LIFE  -  1 TO 4 PLAYERS", W / 2, 212, F_S9, C_CYAN);
  addHit({150, 130, 500, 110}, A_MENU_CMDR);

  frame(g, 150, 260, 500, 110, 10, C_DARK, C_LIME);
  text(g, "STANDARD", W / 2, 302, F_S18, C_WHITE);
  text(g, "20 LIFE  -  2 PLAYERS", W / 2, 342, F_S9, C_CYAN);
  addHit({150, 260, 500, 110}, A_MENU_STD);

  if (game_.started()) {
    button(g, 24, 400, 200, 52, "RESUME GAME", F_S12);
    addHit({24, 400, 200, 52}, A_MENU_RESUME);
  }
  char buf[24];
  snprintf(buf, sizeof buf, "LIGHT %d/%d", brightIdx_ + 1, (int)sizeof(BRIGHT));
  button(g, W - 224, 400, 200, 52, buf, F_S12);
  addHit({W - 224, 400, 200, 52}, A_MENU_BRIGHT);
}

void App::renderSetup() {
  auto& g = canvas_;
  frame(g, 4, 4, W - 8, H - 8, 12, C_PANEL, C_LIME);
  text(g, "COMMANDER", W / 2, 48, F_S24, C_LIME);
  text(g, "STARTING LIFE", W / 2, 92, F_S9, C_WHITE);
  const int lifes[3] = {20, 30, 40};
  for (int i = 0; i < 3; i++) {
    char buf[8]; snprintf(buf, sizeof buf, "%d", lifes[i]);
    Rect r{(int16_t)(160 + i * 165), 108, 150, 52};
    button(g, r.x, r.y, r.w, r.h, buf, F_S18, setupLife_ == lifes[i]);
    addHit(r, A_SETUP_LIFE, lifes[i]);
  }
  text(g, "HOW MANY PLAYERS?", W / 2, 188, F_S9, C_WHITE);
  for (int i = 0; i < 4; i++) {
    char buf[16]; snprintf(buf, sizeof buf, i == 0 ? "%d PLAYER" : "%d PLAYERS", i + 1);
    Rect r{(int16_t)(130 + (i % 2) * 280), (int16_t)(208 + (i / 2) * 82), 260, 68};
    button(g, r.x, r.y, r.w, r.h, buf, F_S18);
    addHit(r, A_SETUP_PLAYERS, i + 1);
  }
  button(g, W / 2 - 90, 392, 180, 56, "BACK", F_S12);
  addHit({W / 2 - 90, 392, 180, 56}, A_SETUP_BACK);
}

// Posição de cada painel. Os de cima ficam de cabeça para baixo
// (para quem está sentado do outro lado da mesa).
void App::layoutPanels(Rect* r, bool* rot, int n) const {
  const int16_t L = 6, R = 794, MID = 400, TOP_Y = 48, BOT_Y = 280, PH = 194;
  const int16_t LW = MID - 4 - L, RX = MID + 4, RW = R - RX, FW = R - L;
  switch (n) {
    case 1:
      r[0] = {L, 90, FW, 384}; rot[0] = false; break;
    case 2:
      r[0] = {L, TOP_Y, FW, PH}; rot[0] = true;
      r[1] = {L, BOT_Y, FW, PH}; rot[1] = false; break;
    case 3:
      r[0] = {L, TOP_Y, FW, PH}; rot[0] = true;
      r[1] = {L, BOT_Y, LW, PH}; rot[1] = false;
      r[2] = {RX, BOT_Y, RW, PH}; rot[2] = false; break;
    default:
      r[0] = {L, TOP_Y, LW, PH};  rot[0] = true;
      r[1] = {RX, TOP_Y, RW, PH}; rot[1] = true;
      r[2] = {L, BOT_Y, LW, PH};  rot[2] = false;
      r[3] = {RX, BOT_Y, RW, PH}; rot[3] = false; break;
  }
}

void App::topBar() {
  auto& g = canvas_;
  text(g, game_.state().format == Format::Commander ? "COMMANDER" : "STANDARD",
       14, 24, F_S12, C_LIME, textdatum_t::middle_left);
  static const char* labels[6] = {"LOG", "LOCK", "UNDO", "FIRST", "RESET", "MENU"};
  static const Action acts[6] = {A_TOP_LOG, A_TOP_LOCK, A_TOP_UNDO, A_TOP_FIRST, A_TOP_RESET, A_TOP_MENU};
  const int bw = 82, gap = 6, by = 6, bh = 34;
  int x = W - 6 - 6 * bw - 5 * gap;
  for (int i = 0; i < 6; i++) {
    bool sel = (i == 1 && locked_);
    bool en = (i != 2 || game_.canUndo());
    button(g, x, by, bw, bh, labels[i], F_S9, sel, en);
    addHit({(int16_t)(x - 2), 0, (int16_t)(bw + 4), (int16_t)(bh + 10)}, acts[i]);
    x += bw + gap;
  }
}

void App::iconButton(int x, int y, int w, int h, int icon, Action act) {
  frame(canvas_, x, y, w, h, 6, C_DARK, C_LIME);
  uint32_t c = icon == ICON_POISON ? C_POISON : icon == ICON_ENERGY ? C_ENERGY : C_LIME;
  drawIcon(canvas_, icon, x + w / 2, y + h / 2, h / 3, c);
  addHit({(int16_t)(x - 4), (int16_t)(y - 4), (int16_t)(w + 8), (int16_t)(h + 8)}, act);
}

void App::renderPlay(uint32_t now) {
  int n = game_.numPlayers();
  Rect rects[MAX_PLAYERS];
  bool rot[MAX_PLAYERS];
  layoutPanels(rects, rot, n);
  for (int i = 0; i < n; i++) renderPanel(i, rects[i], rot[i], now);

  topBar();

  // faixa central com os 3 botões de ícone
  int sy = (n == 1) ? 50 : 244;
  int iw = 62, ih = 34, gap = 8;
  int ix = W / 2 - (3 * iw + 2 * gap) / 2;
  int ly = sy + ih / 2;
  canvas_.fillRect(10, ly - 1, ix - 18, 3, C_LIME);
  canvas_.fillRect(ix + 3 * iw + 2 * gap + 8, ly - 1, W - 10 - (ix + 3 * iw + 2 * gap + 8), 3, C_LIME);
  iconButton(ix, sy, iw, ih, ICON_CMDR, A_ICON_CMDR);
  iconButton(ix + iw + gap, sy, iw, ih, ICON_ENERGY, A_ICON_ENERGY);
  iconButton(ix + 2 * (iw + gap), sy, iw, ih, ICON_POISON, A_ICON_POISON);
}

void App::ensurePanelSprite(int w, int h) {
  if (panel_.width() == w && panel_.height() == h) return;
  panel_.deleteSprite();
  panel_.createSprite(w, h);
}

void App::renderPanel(int i, Rect r, bool rotated, uint32_t now) {
  ensurePanelSprite(r.w, r.h);
  auto& g = panel_;
  const Player& pl = game_.state().p[i];
  int w = r.w, h = r.h;
  bool out = game_.isOut(i);
  bool big = h > 300;

  // destaque do sorteio "FIRST"
  bool hl = false;
  if (firstPick_ >= 0) {
    uint32_t t = now - firstStart_;
    if (t < 1500) hl = ((t / 110) % game_.numPlayers()) == (uint32_t)i;
    else if (t < 4000) hl = (i == firstPick_);
  }

  g.fillScreen(C_BG);
  frame(g, 0, 0, w, h, 10, out ? C_PANEL_OUT : C_PANEL, hl ? C_CYAN : C_LIME);
  if (hl) { g.drawRoundRect(2, 2, w - 4, h - 4, 9, C_CYAN); g.drawRoundRect(3, 3, w - 6, h - 6, 8, C_CYAN); }
  if (out) g.fillRoundRect(8, 8, w - 16, 10, 4, C_RED);

  int cy = h / 2 - (big ? 0 : 6);
  int zone = std::min(w / 3, 170);

  // + e -
  uint32_t signC = locked_ ? C_DIM : C_LIME;
  drawMinus(g, zone / 2, cy, big ? 26 : 18, signC);
  drawPlus(g, w - zone / 2, cy, big ? 26 : 18, signC);

  // vida
  char buf[24];
  snprintf(buf, sizeof buf, "%d", pl.life);
  g.setFont(F_SEG);
  g.setTextSize(big ? 3 : 2);
  int tw = g.textWidth(buf), th = g.fontHeight();
  g.fillRoundRect(w / 2 - tw / 2 - 14, cy - th / 2 - 10, tw + 28, th + 20, 8, out ? C_PANEL_OUT : C_BOX);
  text(g, buf, w / 2, cy, F_SEG, out ? C_DIM : C_WHITE, textdatum_t::middle_center, big ? 3 : 2);

  // etiqueta do jogador
  int tagW = 54, tagH = 26, tagX = w / 2 - tagW / 2, tagY = h - tagH - 8;
  g.fillRoundRect(tagX, tagY, tagW, tagH, 5, out ? C_DIM : C_PLAYER[i]);
  snprintf(buf, sizeof buf, "P%d", i + 1);
  text(g, buf, w / 2, tagY + tagH / 2 + 1, F_S9, C_NAVY);

  // variação recente (ex.: -3)
  int16_t d = game_.recentDelta(i, now);
  if (d) {
    snprintf(buf, sizeof buf, "%+d", d);
    text(g, buf, tagX - 12, tagY + tagH / 2, F_S12, d < 0 ? C_RED : C_LIME, textdatum_t::middle_right);
  }

  // veneno e energia ao lado da etiqueta
  int x = tagX + tagW + 14, my = tagY + tagH / 2;
  if (pl.poison > 0) {
    drawIcon(g, ICON_POISON, x + 7, my, 8, C_POISON);
    snprintf(buf, sizeof buf, "%d", pl.poison);
    text(g, buf, x + 18, my, F_S12, C_POISON, textdatum_t::middle_left);
    x += 50;
  }
  if (pl.energy > 0) {
    drawIcon(g, ICON_ENERGY, x + 7, my, 9, C_ENERGY);
    snprintf(buf, sizeof buf, "%d", pl.energy);
    text(g, buf, x + 18, my, F_S12, C_ENERGY, textdatum_t::middle_left);
  }

  // maior dano de comandante recebido
  int mc = game_.maxCmdrTaken(i);
  if (mc > 0) {
    drawIcon(g, ICON_CMDR, 26, 30, 9, mc >= 15 ? C_ORANGE : C_LIME);
    snprintf(buf, sizeof buf, "%d", mc);
    text(g, buf, 42, 31, F_S12, mc >= 15 ? C_ORANGE : C_WHITE, textdatum_t::middle_left);
  }

  // mensagens no topo do painel
  if (out) {
    static const char* why[] = {"", "OUT", "OUT - POISON", "OUT - COMMANDER"};
    text(g, why[(int)game_.outReason(i)], w / 2, 34, F_S12, C_RED);
  } else if (game_.winner() == i) {
    text(g, "WINNER!", w / 2, 32, F_S18, C_LIME);
  } else if (firstPick_ == i && now - firstStart_ >= 1500 && now - firstStart_ < 4000) {
    text(g, "GOES FIRST!", w / 2, 32, F_S18, C_CYAN);
  }

  // painel de cima: gira 180 graus invertendo a ordem dos pixels
  if (rotated) {
    uint16_t* px = (uint16_t*)g.getBuffer();
    std::reverse(px, px + (size_t)w * h);
  }
  g.pushSprite(&canvas_, r.x, r.y);

  if (!locked_) {
    addPanelHit(r, rotated, Rect{0, 0, (int16_t)zone, (int16_t)h}, A_LIFE, i, -1, true);
    addPanelHit(r, rotated, Rect{(int16_t)(w - zone), 0, (int16_t)zone, (int16_t)h}, A_LIFE, i, +1, true);
  }
}

void App::renderCounters() {
  auto& g = canvas_;
  bool poison = counterKind_ == Counter::Poison;
  frame(g, 4, 4, W - 8, H - 8, 12, C_PANEL, C_LIME);
  text(g, poison ? "POISON COUNTERS" : "ENERGY COUNTERS", W / 2, 24, F_S12, C_LIME);
  int n = game_.numPlayers();
  int rowH = 76, gap = 14, y0 = 46;
  if (n <= 2) { rowH = 110; gap = 24; }
  for (int i = 0; i < n; i++) {
    int y = y0 + i * (rowH + gap);
    const Player& pl = game_.state().p[i];
    int v = poison ? pl.poison : pl.energy;
    char buf[16];
    snprintf(buf, sizeof buf, "P%d", i + 1);
    g.fillRoundRect(20, y, 90, rowH, 8, game_.isOut(i) ? C_DIM : C_PLAYER[i]);
    text(g, buf, 65, y + rowH / 2, F_S18, C_NAVY);

    frame(g, 126, y, 200, rowH, 8, C_DARK, C_LIME);
    drawMinus(g, 226, y + rowH / 2, 20, C_LIME);
    addHit({126, (int16_t)y, 200, (int16_t)rowH}, A_COUNTER, i, -1, true);

    bool lethal = poison && v >= POISON_LETHAL;
    frame(g, 340, y, 200, rowH, 8, lethal ? C_RED : C_CYAN, C_LIME);
    snprintf(buf, sizeof buf, "%d", v);
    text(g, buf, 440, y + rowH / 2 - (poison ? 8 : 0), F_SEG, C_NAVY, textdatum_t::middle_center, 1);
    if (poison) text(g, "OF 10", 440, y + rowH - 12, F_S9, C_NAVY);

    frame(g, 554, y, 200, rowH, 8, C_DARK, C_LIME);
    drawPlus(g, 654, y + rowH / 2, 20, C_LIME);
    addHit({554, (int16_t)y, 200, (int16_t)rowH}, A_COUNTER, i, +1, true);
  }
  button(g, W / 2 - 100, 418, 200, 50, "BACK", F_S12);
  addHit({W / 2 - 100, 418, 200, 50}, A_COUNTER_BACK);
}

void App::renderCmdr() {
  auto& g = canvas_;
  const State& s = game_.state();
  int n = s.numPlayers;
  frame(g, 4, 4, W - 8, H - 8, 12, C_PANEL, C_LIME);
  text(g, "COMMANDER DAMAGE", W / 2, 22, F_S12, C_LIME);

  // alvo
  text(g, "TARGET", 20, 64, F_S9, C_WHITE, textdatum_t::middle_left);
  char buf[80];
  for (int i = 0; i < n; i++) {
    snprintf(buf, sizeof buf, "P%d", i + 1);
    Rect r{(int16_t)(110 + i * 120), 44, 108, 40};
    button(g, r.x, r.y, r.w, r.h, buf, F_S12, cmdTarget_ == i);
    addHit(r, A_CMD_TARGET, i);
  }

  // origem: quanto o alvo já levou de cada comandante
  int bw = (W - 32 - (n - 1) * 10) / n;
  for (int i = 0; i < n; i++) {
    Rect r{(int16_t)(16 + i * (bw + 10)), 96, (int16_t)bw, 100};
    bool sel = cmdFrom_ == i;
    int taken = s.p[cmdTarget_].cmdr[i];
    frame(g, r.x, r.y, r.w, r.h, 8, sel ? C_CYAN : C_DARK, C_LIME);
    snprintf(buf, sizeof buf, i == cmdTarget_ ? "FROM P%d (SELF)" : "FROM P%d", i + 1);
    text(g, buf, r.x + r.w / 2, r.y + 16, F_S9, sel ? C_NAVY : C_WHITE);
    snprintf(buf, sizeof buf, "%d", taken);
    uint32_t c = taken >= CMDR_LETHAL ? C_RED : (sel ? C_NAVY : C_WHITE);
    text(g, buf, r.x + r.w / 2, r.y + 62, F_SEG, c, textdatum_t::middle_center, 1);
    addHit(r, A_CMD_FROM, i);
  }

  // resumo
  int before = s.p[cmdTarget_].cmdr[cmdFrom_];
  snprintf(buf, sizeof buf, "P%d  >  P%d", cmdFrom_ + 1, cmdTarget_ + 1);
  text(g, buf, W / 2, 232, F_S24, C_WHITE);
  snprintf(buf, sizeof buf, "DAMAGE: %+d       FROM P%d: %d  ->  %d", cmdDmg_, cmdFrom_ + 1, before, before + cmdDmg_);
  text(g, buf, W / 2, 284, F_S12, cmdDmg_ ? C_CYAN : C_WHITE);
  snprintf(buf, sizeof buf, "P%d HAS TAKEN %d IN TOTAL   (21 FROM ONE = OUT)",
           cmdTarget_ + 1, game_.totalCmdrTaken(cmdTarget_));
  text(g, buf, W / 2, 322, F_S9, C_DIM);
  if (before + cmdDmg_ >= CMDR_LETHAL && before < CMDR_LETHAL)
    text(g, "LETHAL!", W / 2, 352, F_S12, C_RED);

  // botões
  frame(g, 16, 384, 120, 80, 8, C_DARK, C_LIME);
  drawMinus(g, 76, 424, 20, C_LIME);
  addHit({16, 384, 120, 80}, A_CMD_DELTA, 0, -1, true);
  frame(g, 146, 384, 120, 80, 8, C_DARK, C_LIME);
  drawPlus(g, 206, 424, 20, C_LIME);
  addHit({146, 384, 120, 80}, A_CMD_DELTA, 0, +1, true);
  button(g, 276, 384, 330, 80, "APPLY", F_S24, false, cmdDmg_ != 0);
  addHit({276, 384, 330, 80}, A_CMD_APPLY);
  button(g, 616, 384, 168, 80, "BACK", F_S12);
  addHit({616, 384, 168, 80}, A_CMD_BACK);
}

void App::renderLog() {
  auto& g = canvas_;
  frame(g, 4, 4, W - 8, H - 8, 12, C_PANEL, C_LIME);
  text(g, "HISTORY  (NEWEST FIRST)", W / 2, 22, F_S12, C_LIME);
  int total = game_.logCount();
  int first = logPage_ * 10;
  for (int k = 0; k < 10 && first + k < total; k++) {
    int i = first + k;
    int y = 42 + k * 34;
    g.fillRect(14, y, W - 28, 32, (k % 2) ? C_PANEL : C_DARK);
    char buf[80];
    snprintf(buf, sizeof buf, "#%-3d %s", game_.logNumber(i), game_.logLine(i));
    text(g, buf, 24, y + 16, F_MONO, C_WHITE, textdatum_t::middle_left);
  }
  button(g, 16, 408, 150, 56, "NEWER", F_S12, false, logPage_ > 0);
  addHit({16, 408, 150, 56}, A_LOG_NEWER);
  button(g, 176, 408, 150, 56, "OLDER", F_S12, false, first + 10 < total);
  addHit({176, 408, 150, 56}, A_LOG_OLDER);
  char buf[32];
  if (total) snprintf(buf, sizeof buf, "%d-%d OF %d", first + 1, std::min(first + 10, total), total);
  else snprintf(buf, sizeof buf, "EMPTY");
  text(g, buf, 470, 436, F_S12, C_WHITE);
  button(g, 614, 408, 170, 56, "BACK", F_S12);
  addHit({614, 408, 170, 56}, A_LOG_BACK);
}

void App::renderConfirm() {
  auto& g = canvas_;
  // escurece o fundo com um padrão quadriculado (simples e rápido)
  for (int y = 0; y < H; y += 2)
    for (int x = (y / 2) % 2; x < W; x += 2) g.drawPixel(x, y, (uint32_t)0x000000);
  frame(g, 170, 130, 460, 220, 14, C_PANEL, C_LIME);
  bool reset = overlay_ == Overlay::ConfirmReset;
  text(g, reset ? "RESET GAME?" : "BACK TO MENU?", W / 2, 180, F_S24, C_WHITE);
  text(g, reset ? "Everyone goes back to starting life." : "The game is kept: use RESUME.", W / 2, 222, F_S9, C_CYAN);
  button(g, 200, 260, 190, 64, "YES", F_S18);
  addHit({200, 260, 190, 64}, A_CONFIRM_YES);
  button(g, 410, 260, 190, 64, "NO", F_S18);
  addHit({410, 260, 190, 64}, A_CONFIRM_NO);
}
