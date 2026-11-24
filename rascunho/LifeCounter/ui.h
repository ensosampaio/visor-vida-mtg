// ui.h — telas, desenho e toques.
// Tudo é desenhado numa "tela invisível" (sprite) de 800x480 e depois
// copiado de uma vez para o display, o que evita a tela piscar.
#pragma once
#include <LovyanGFX.hpp>
#include "game.h"

// Funções que dependem do hardware. O .ino fornece as versões da placa;
// o simulador do PC fornece as dele.
struct PlatformHooks {
  void (*present)();                 // copia a tela invisível para o display
  void (*setBrightness)(uint8_t);    // 0..255
  uint32_t (*randomN)(uint32_t n);   // número aleatório de 0 a n-1
};

class App {
public:
  App(LGFX_Sprite& canvas, PlatformHooks hooks);
  void begin();
  // Chamar a cada volta do loop(): touched = dedo na tela, x/y = posição.
  void update(bool touched, int x, int y, uint32_t now);
  mtg::Game& game() { return game_; }

private:
  enum class Screen : uint8_t { Menu, Setup, Play, Counters, Cmdr, Log };
  enum class Overlay : uint8_t { None, ConfirmReset, ConfirmMenu };
  enum Action : uint8_t {
    A_NONE, A_MENU_CMDR, A_MENU_STD, A_MENU_RESUME, A_MENU_BRIGHT,
    A_SETUP_LIFE, A_SETUP_PLAYERS, A_SETUP_BACK,
    A_LIFE, A_TOP_LOG, A_TOP_LOCK, A_TOP_UNDO, A_TOP_FIRST, A_TOP_RESET, A_TOP_MENU,
    A_ICON_CMDR, A_ICON_POISON, A_ICON_ENERGY,
    A_COUNTER, A_COUNTER_BACK,
    A_CMD_TARGET, A_CMD_FROM, A_CMD_DELTA, A_CMD_APPLY, A_CMD_BACK,
    A_LOG_NEWER, A_LOG_OLDER, A_LOG_BACK,
    A_CONFIRM_YES, A_CONFIRM_NO,
  };
  struct Rect { int16_t x, y, w, h; };
  struct Hit { Rect r; uint8_t action; int8_t a, b; bool repeat; };

  // desenho
  void render(uint32_t now);
  void renderMenu();
  void renderSetup();
  void renderPlay(uint32_t now);
  void renderPanel(int i, Rect r, bool rotated, uint32_t now);
  void renderCounters();
  void renderCmdr();
  void renderLog();
  void renderConfirm();
  void topBar();
  void iconButton(int x, int y, int w, int h, int icon, Action act);
  void ensurePanelSprite(int w, int h);

  // toques
  void addHit(Rect r, Action act, int a = 0, int b = 0, bool repeat = false);
  void addPanelHit(Rect panel, bool rotated, Rect local, Action act, int a, int b, bool repeat);
  const Hit* hitAt(int x, int y) const;
  void perform(const Hit& h, int step, uint32_t now);

  void layoutPanels(Rect* rects, bool* rotated, int n) const;
  bool animating(uint32_t now) const;

  LGFX_Sprite& canvas_;
  LGFX_Sprite panel_;
  PlatformHooks hooks_;
  mtg::Game game_;

  Screen screen_ = Screen::Menu;
  Overlay overlay_ = Overlay::None;
  Hit hits_[64];
  int hitCount_ = 0;

  // toque atual
  bool down_ = false;
  Hit held_{};
  bool holding_ = false;
  uint32_t nextRepeat_ = 0;
  int repeatCount_ = 0;

  bool dirty_ = true;
  uint32_t lastFrame_ = 0;

  // estado das telas
  int setupLife_ = 40;
  bool locked_ = false;
  mtg::Counter counterKind_ = mtg::Counter::Poison;
  int cmdTarget_ = 1, cmdFrom_ = 0, cmdDmg_ = 0;
  int logPage_ = 0;
  int brightIdx_ = 3;
  int firstPick_ = -1;
  uint32_t firstStart_ = 0;
};
