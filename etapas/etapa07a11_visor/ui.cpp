// ui.cpp — desenho das telas (tema 8-bit) e tratamento dos toques.
//
// O desenho usa o motor gfx8 (gfx8.h): tudo em "pixels 8-bit", numa tela lógica de
// 400x240 em que cada pixel vira um quadradinho de 2x2 na tela de 800x480.
// Os toques (addHit) continuam em pixels reais da tela; addHitL converte.
#include "ui.h"
#include "gfx8.h"
#include <stdio.h>
#include <string.h>
#include <algorithm>

using namespace mtg;

static const uint8_t BRIGHT[] = {50, 110, 180, 255};
static const char* NOME_P[4] = {"P1", "P2", "P3", "P4"};

// ---------------------------------------------------------------- ligação com a tela
// O motor gfx8 só sabe pintar retângulos; aqui ele pinta no sprite (a "tela invisível").
static lgfx::LovyanGFX* g_alvo = nullptr;
void portaPreencher(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t cor) {
  g_alvo->fillRect(x, y, w, h, cor);   // uint16_t = cor RGB565 para a LovyanGFX
}

static constexpr uint32_t LONG_PRESS_MS = 700;   // segurar o meio do painel = "é a vez deste jogador"

// Painel de quem está no turno: fundo mais claro (a borda fica mais grossa no desenho).
static const Estilo EST_TURNO[4] = {
  {C_CIANO,   C_AZUL_M,   C_AZUL},     // P1
  {C_LIMA,    C_VERDE_E2, C_VERDE_M},  // P2
  {C_LARANJA, C_VERM_M,   C_VERMELHO}, // P3
  {C_MAGENTA, C_VIOLETA,  C_MAGENTA},  // P4
};
static const Estilo EST_PASSAR = {C_OURO, C_MARROM, C_OURO_E};

// estilos usados só nesta tela
static const Estilo EST_NEUTRO  = {C_CINZA_C, C_PRETO, SEM_TEXTURA};
static const Estilo EST_CAIXA   = {C_OURO, C_PRETO, SEM_TEXTURA};
static const Estilo EST_LETAL   = {C_VERMELHO, C_VERM_M, SEM_TEXTURA};
static const Estilo EST_LISTA   = {C_CINZA_C, C_PRETO, SEM_TEXTURA};

// ---------------------------------------------------------------- ciclo principal
App::App(LGFX_Sprite& canvas, PlatformHooks hooks)
    : canvas_(canvas), hooks_(hooks) {}

void App::begin() {
  g_alvo = &canvas_;
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
    if (h && h->longPress) {                 // toque longo: só conta depois de segurar
      held_ = *h;
      holding_ = false;
      longArmed_ = true;
      longAt_ = now + LONG_PRESS_MS;
    } else if (h) {
      held_ = *h;
      holding_ = h->repeat;
      repeatCount_ = 0;
      nextRepeat_ = now + 450;
      perform(*h, 1, now);
      dirty_ = true;
    }
  } else if (touched && down_ && longArmed_) { // esperando o toque longo completar
    bool inside = x >= held_.r.x && x < held_.r.x + held_.r.w && y >= held_.r.y && y < held_.r.y + held_.r.h;
    if (!inside) longArmed_ = false;
    else if ((int32_t)(now - longAt_) >= 0) {
      longArmed_ = false;
      perform(held_, 1, now);
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
    longArmed_ = false;
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
void App::addHit(Rect r, Action act, int a, int b, bool repeat, bool longPress) {
  if (hitCount_ >= (int)(sizeof(hits_) / sizeof(hits_[0]))) return;
  hits_[hitCount_++] = Hit{r, (uint8_t)act, (int8_t)a, (int8_t)b, repeat, longPress};
}

// Converte uma área do painel (coordenadas do próprio painel) para coordenadas da tela,
// levando em conta se o painel está de cabeça para baixo.
void App::addPanelHit(Rect p, bool rotated, Rect l, Action act, int a, int b, bool repeat, bool longPress) {
  Rect s;
  if (rotated) s = Rect{(int16_t)(p.x + p.w - (l.x + l.w)), (int16_t)(p.y + p.h - (l.y + l.h)), l.w, l.h};
  else         s = Rect{(int16_t)(p.x + l.x), (int16_t)(p.y + l.y), l.w, l.h};
  addHit(s, act, a, b, repeat, longPress);
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
      game_.startTurns(firstPick_);   // registra "P2 GOES FIRST" e começa a rodada 1
      break;
    }
    case A_PASS_TURN:   game_.passTurn(); firstPick_ = -1; break;
    case A_SET_TURN:    game_.setTurn(h.a); firstPick_ = -1; break;
    case A_MENU_TURNS:  turnsOn_ = !turnsOn_; break;
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

void App::addHitL(int x, int y, int w, int h, Action act, int a, int b, bool repeat) {
  addHit(Rect{(int16_t)(x * ESCALA), (int16_t)(y * ESCALA), (int16_t)(w * ESCALA), (int16_t)(h * ESCALA)},
         act, a, b, repeat);
}

// ---------------------------------------------------------------- desenho
void App::render(uint32_t now) {
  g_alvo = &canvas_;
  gOrigemReset();
  hitCount_ = 0;
  gFundoEstrelado();
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
  gTexto("LIFE COUNTER", 200, 10, 3, C_OURO, C_PRETO, CENTRO);
  gTexto("MAGIC: THE GATHERING", 200, 36, 1, C_CIANO, C_PRETO, CENTRO);

  gCaixa(75, 52, 250, 62, 4, EST_BTN_OURO, 2, true);
  gTexto("COMMANDER", 200, 62, 3, C_BRANCO, C_PRETO, CENTRO);
  gTexto("20/30/40 LIFE - 1 TO 4 PLAYERS", 200, 94, 1, C_OURO, C_PRETO, CENTRO);
  addHitL(75, 52, 250, 62, A_MENU_CMDR);

  gCaixa(75, 124, 250, 62, 4, EST_BTN_CIANO, 2, true);
  gTexto("STANDARD", 200, 134, 3, C_BRANCO, C_PRETO, CENTRO);
  gTexto("20 LIFE - 2 PLAYERS", 200, 166, 1, C_CIANO, C_PRETO, CENTRO);
  addHitL(75, 124, 250, 62, A_MENU_STD);

  if (game_.started()) {
    gBotao(12, 200, 100, 26, EST_BTN_VERDE, "RESUME GAME");
    addHitL(12, 200, 100, 26, A_MENU_RESUME);
  }
  gBotao(150, 200, 100, 26, turnsOn_ ? EST_BTN_SELECIONADO : EST_BTN_NORMAL,
         turnsOn_ ? "TURNS: ON" : "TURNS: OFF", 1, turnsOn_ ? C_PRETO : C_BRANCO);
  addHitL(150, 200, 100, 26, A_MENU_TURNS);
  char buf[24];
  snprintf(buf, sizeof buf, "LIGHT %d/%d", brightIdx_ + 1, (int)sizeof(BRIGHT));
  gBotao(288, 200, 100, 26, EST_BTN_ROXO, buf);
  addHitL(288, 200, 100, 26, A_MENU_BRIGHT);
}

void App::renderSetup() {
  gTexto("COMMANDER", 200, 8, 3, C_OURO, C_PRETO, CENTRO);
  gTexto("STARTING LIFE", 200, 36, 1, C_CIANO, C_PRETO, CENTRO);
  const int lifes[3] = {20, 30, 40};
  char buf[16];
  for (int i = 0; i < 3; i++) {
    snprintf(buf, sizeof buf, "%d", lifes[i]);
    int x = 70 + i * 90;
    gBotao(x, 46, 80, 32, setupLife_ == lifes[i] ? EST_BTN_SELECIONADO : EST_BTN_NORMAL, buf, 2);
    addHitL(x, 46, 80, 32, A_SETUP_LIFE, lifes[i]);
  }
  gTexto("HOW MANY PLAYERS?", 200, 90, 1, C_CIANO, C_PRETO, CENTRO);
  for (int i = 0; i < 4; i++) {
    snprintf(buf, sizeof buf, i == 0 ? "%d PLAYER" : "%d PLAYERS", i + 1);
    int x = 45 + (i % 2) * 160, y = 102 + (i / 2) * 40;
    gBotao(x, y, 150, 32, EST_BTN_NORMAL, buf, 2);
    addHitL(x, y, 150, 32, A_SETUP_PLAYERS, i + 1);
  }
  gBotao(150, 200, 100, 26, EST_BTN_NORMAL, "BACK");
  addHitL(150, 200, 100, 26, A_SETUP_BACK);
}

// Posição de cada painel (em pixels 8-bit). Os de cima ficam de cabeça para baixo
// (para quem está sentado do outro lado da mesa).
void App::layoutPanels(Rect* r, bool* rot, int n) const {
  const int16_t L = 3, TOP_Y = 24, BOT_Y = 147, PH = 90;   // faixa do meio: y 114..146
  const int16_t LW = 195, RX = 202, RW = 195, FW = 394;
  switch (n) {
    case 1:
      r[0] = {L, 45, FW, 192}; rot[0] = false; break;
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
  gTexto(game_.state().format == Format::Commander ? "COMMANDER" : "STANDARD", 4, 3, 2, C_OURO);
  static const char* labels[6] = {"LOG", "LOCK", "UNDO", "FIRST", "RESET", "MENU"};
  static const Action acts[6] = {A_TOP_LOG, A_TOP_LOCK, A_TOP_UNDO, A_TOP_FIRST, A_TOP_RESET, A_TOP_MENU};
  for (int i = 0; i < 6; i++) {
    int x = 136 + 44 * i;
    uint8_t estado = 0;                                // 0 normal, 1 ativo, 2 desativado
    if (i == 1 && locked_) estado = 1;
    if (i == 2 && !game_.canUndo()) estado = 2;
    gBotaoTopo(x, 3, labels[i], estado);
    addHitL(x - 1, 0, 43, 22, acts[i]);                // área de toque um pouco maior que o botão
  }
}

void App::iconButton(int x, int y, uint8_t icon, Action act) {
  gBotaoIcone(x, y, (Icone)icon);
  addHitL(x - 2, y - 2, 35, 21, act);
}

void App::renderPlay(uint32_t now) {
  int n = game_.numPlayers();
  Rect rects[MAX_PLAYERS];
  bool rot[MAX_PLAYERS];
  layoutPanels(rects, rot, n);
  for (int i = 0; i < n; i++) renderPanel(i, rects[i], rot[i], now);

  topBar();

  // faixa central: botões de passar o turno (um de cada lado), rodada e os 3 botões de ícone
  bool turns = turnsOn_ && n > 1;
  int sy = (n == 1) ? 25 : 126;
  if (turns) {
    passButton(4, 116, false);     // lido por quem está embaixo
    passButton(296, 116, true);    // de cabeça para baixo, para quem está em cima
    char buf[24];
    if (game_.turnPlayer() >= 0) snprintf(buf, sizeof buf, "ROUND %d", game_.round());
    else snprintf(buf, sizeof buf, "NO TURN YET");
    gTexto(buf, 200, 116, 1, C_OURO, C_PRETO, CENTRO);
    gBarra(108, sy + 7, 38, C_OURO);
    gBarra(254, sy + 7, 38, C_LIMA);
  } else {
    gBarra(4, sy + 7, 140, C_OURO);
    gBarra(257, sy + 7, 140, C_LIMA);
  }
  iconButton(149, sy, ICO_COROA, A_ICON_CMDR);
  iconButton(184, sy, ICO_RAIO, A_ICON_ENERGY);
  iconButton(219, sy, ICO_GOTA, A_ICON_POISON);
}

// Botão "PASS TURN" (100x28 em pixels 8-bit = 200x56 na tela).
void App::passButton(int x, int y, bool rotated) {
  const int w = 100, h = 28;
  bool en = game_.winner() < 0;
  gOrigem(x, y, w, h, rotated);
  gBotao(0, 0, w, h, en ? EST_PASSAR : EST_BTN_DESATIVADO, "PASS TURN >", 1, en ? C_OURO : C_CINZA);
  gOrigemReset();
  addHitL(x, y - 1, w, h + 2, A_PASS_TURN);
}

// Um painel de jogador. Tudo é desenhado nas coordenadas do jogador "de baixo";
// para os de cima, gOrigem(..., true) gira o desenho 180 graus.
void App::renderPanel(int i, Rect r, bool rotated, uint32_t now) {
  const Player& pl = game_.state().p[i];
  int w = r.w, h = r.h;
  bool out = game_.isOut(i);
  bool big = h > 150;

  // destaque do sorteio "FIRST": a borda pisca de painel em painel e para no sorteado
  bool hl = false;
  if (firstPick_ >= 0) {
    uint32_t t = now - firstStart_;
    if (t < 1500) hl = ((t / 110) % game_.numPlayers()) == (uint32_t)i;
    else if (t < 4000) hl = (i == firstPick_);
  }

  // turno: só aparece com a opção ligada e fora da animação do sorteio
  bool sorteando = firstPick_ >= 0 && now - firstStart_ < 1500;
  bool turn = turnsOn_ && game_.numPlayers() > 1 && !out && !sorteando && game_.turnPlayer() == i;

  gOrigem(r.x, r.y, w, h, rotated);
  Estilo e = out ? EST_ELIMINADO : (turn ? EST_TURNO[i] : EST_JOGADOR[i]);
  if (hl) e.borda = C_BRANCO;
  gCaixa(0, 0, w, h, 3, e, turn ? 4 : 2, true);       // no turno: borda com o dobro da grossura
  if (out) gRet(6, 5, w - 12, 3, C_VERMELHO);         // faixa vermelha do eliminado

  int cy = h / 2 + (big ? 0 : 3);   // um pouco abaixo do meio: deixa espaço para "YOUR TURN"
  bool largo = w > 200;
  bool apagado = out || locked_;
  gSinal('-', largo ? 58 : 25, cy - 2, apagado);
  gSinal('+', w - (largo ? 74 : 41), cy - 8, apagado);

  // vida (algarismos grandes 8x10)
  bool tresDig = pl.life >= 100 || pl.life <= -10;
  uint8_t k = big ? (tresDig ? 6 : 7) : (tresDig ? 3 : 4);
  gNumeroGrande(pl.life, w / 2, cy - 5 * k - 2, k, out ? C_CINZA : C_BRANCO);

  // etiqueta do jogador e variação recente (ex.: -3)
  gEtiqueta(w / 2 - 13, h - 17, out ? C_CINZA : COR_JOGADOR[i], NOME_P[i]);
  int16_t d = game_.recentDelta(i, now);
  if (d) gNumero(d, w / 2 - 19, h - 14, 1, d < 0 ? C_VERMELHO : C_LIMA, C_PRETO, DIR, true);

  // veneno e energia no canto inferior direito
  int xr = w - 9;
  if (pl.poison > 0) {
    xr -= gNumero(pl.poison, xr, h - 14, 1, C_LIMA, C_PRETO, DIR) + 11;
    gIcone(ICO_GOTA, false, xr, h - 15);
    xr -= 6;
  }
  if (pl.energy > 0) {
    xr -= gNumero(pl.energy, xr, h - 14, 1, C_CIANO, C_PRETO, DIR) + 11;
    gIcone(ICO_RAIO, false, xr, h - 15);
  }
  // maior dano de comandante recebido, no canto inferior esquerdo (laranja a partir de 15)
  int mc = game_.maxCmdrTaken(i);
  if (mc > 0) {
    gIcone(ICO_COROA, false, 9, h - 15);
    gNumero(mc, 21, h - 14, 1, mc >= 15 ? C_LARANJA : C_OURO);
  }

  // mensagem no alto do painel
  if (out) {
    static const char* why[] = {"", "OUT", "OUT - POISON", "OUT - COMMANDER"};
    const char* t = why[(int)game_.outReason(i)];
    uint8_t kt = gLarguraTexto(t, 2) <= w - 16 ? 2 : 1;   // texto longo num painel estreito: letra menor
    gTexto(t, w / 2, kt == 2 ? 11 : 13, kt, C_VERMELHO, C_PRETO, CENTRO);
  } else if (game_.winner() == i) {
    gTexto("WINNER!", w / 2, 6, 2, C_LIMA, C_PRETO, CENTRO);
  } else if (firstPick_ == i && now - firstStart_ >= 1500 && now - firstStart_ < 4000) {
    gTexto("GOES FIRST!", w / 2, 7, 2, C_CIANO, C_PRETO, CENTRO);
  } else if (turn) {
    gTexto("YOUR TURN", w / 2, 7, 2, C_OURO, C_PRETO, CENTRO);
  }
  gOrigemReset();

  // toques: terço esquerdo = -, terço direito = + (segurar repete)
  if (!locked_) {
    Rect p{(int16_t)(r.x * ESCALA), (int16_t)(r.y * ESCALA), (int16_t)(w * ESCALA), (int16_t)(h * ESCALA)};
    int16_t zone = std::min(w / 3, 85) * ESCALA, ph = h * ESCALA;
    addPanelHit(p, rotated, Rect{0, 0, zone, ph}, A_LIFE, i, -1, true);
    addPanelHit(p, rotated, Rect{(int16_t)(p.w - zone), 0, zone, ph}, A_LIFE, i, +1, true);
    // meio do painel: segurar = "agora é a vez deste jogador"
    if (turnsOn_ && game_.numPlayers() > 1)
      addPanelHit(p, rotated, Rect{zone, 0, (int16_t)(p.w - 2 * zone), ph}, A_SET_TURN, i, 0, false, true);
  }
}

void App::renderCounters() {
  bool poison = counterKind_ == Counter::Poison;
  gTexto(poison ? "POISON COUNTERS" : "ENERGY COUNTERS", 200, 5, 2, poison ? C_LIMA : C_CIANO, C_PRETO, CENTRO);
  int n = game_.numPlayers();
  int rowH = 38, gap = 7, y0 = 23;
  if (n <= 2) { rowH = 55; gap = 12; }
  char buf[16];
  for (int i = 0; i < n; i++) {
    int y = y0 + i * (rowH + gap);
    const Player& pl = game_.state().p[i];
    int v = poison ? pl.poison : pl.energy;
    bool out = game_.isOut(i);

    gCaixa(10, y, 45, rowH, 3, out ? EST_ELIMINADO : EST_JOGADOR[i], 2, false);
    gTexto(NOME_P[i], 33, y + rowH / 2 - 7, 2, C_BRANCO, C_PRETO, CENTRO);

    gCaixa(63, y, 100, rowH, 3, EST_BTN_NORMAL, 2, false);
    gSinal('-', 105, y + rowH / 2 - 2, false);
    addHitL(63, y, 100, rowH, A_COUNTER, i, -1, true);

    bool lethal = poison && v >= POISON_LETHAL;
    gCaixa(170, y, 100, rowH, 3, lethal ? EST_LETAL : EST_NEUTRO, 2, false);
    snprintf(buf, sizeof buf, "%d", v);
    if (poison) {
      gTexto(buf, 220, y + (rowH - 31) / 2, 3, C_BRANCO, C_PRETO, CENTRO);
      for (int s = 0; s < POISON_LETHAL; s++) {          // medidor de 10 gotas
        int sx = 190 + s * 6, sy = y + rowH - 14;
        gRet(sx, sy, 5, 8, C_PRETO);
        gRet(sx + 1, sy + 1, 3, 6, s < v ? C_LIMA : C_VERDE_E);
        if (s < v) gRet(sx + 1, sy + 1, 1, 2, C_BRANCO);
      }
    } else {
      gTexto(buf, 220, y + rowH / 2 - 10, 3, C_BRANCO, C_PRETO, CENTRO);
    }

    gCaixa(277, y, 100, rowH, 3, EST_BTN_NORMAL, 2, false);
    gSinal('+', 319, y + rowH / 2 - 8, false);
    addHitL(277, y, 100, rowH, A_COUNTER, i, +1, true);
  }
  gBotao(150, 209, 100, 26, EST_BTN_NORMAL, "BACK");
  addHitL(150, 209, 100, 26, A_COUNTER_BACK);
}

void App::renderCmdr() {
  const State& s = game_.state();
  int n = s.numPlayers;
  char buf[80];
  gTexto("COMMANDER DAMAGE", 200, 4, 2, C_OURO, C_PRETO, CENTRO);

  // alvo
  gTexto("TARGET", 8, 28, 1, C_CIANO);
  for (int i = 0; i < n; i++) {
    int x = 55 + i * 60;
    Estilo sel = {COR_JOGADOR[i], EST_JOGADOR[i].fundo, SEM_TEXTURA};
    gBotao(x, 22, 54, 20, cmdTarget_ == i ? sel : EST_NEUTRO, NOME_P[i], 1,
           cmdTarget_ == i ? C_BRANCO : C_CINZA_C);
    addHitL(x, 22, 54, 20, A_CMD_TARGET, i);
  }

  // origem: quanto o alvo já levou de cada comandante
  int bw = (400 - 16 - (n - 1) * 5) / n;
  for (int i = 0; i < n; i++) {
    int x = 8 + i * (bw + 5), y = 48;
    bool sel = cmdFrom_ == i;
    int taken = s.p[cmdTarget_].cmdr[i];
    gCaixa(x, y, bw, 50, 3, sel ? EST_BTN_SELECIONADO : EST_NEUTRO, 2, false);
    snprintf(buf, sizeof buf, i == cmdTarget_ ? "P%d (SELF)" : "FROM P%d", i + 1);
    gTexto(buf, x + bw / 2, y + 7, 1, sel ? C_BRANCO : C_CINZA_C, C_PRETO, CENTRO);
    snprintf(buf, sizeof buf, "%d", taken);
    gTexto(buf, x + bw / 2, y + 20, 3, taken >= CMDR_LETHAL ? C_VERMELHO : C_BRANCO, C_PRETO, CENTRO);
    addHitL(x, y, bw, 50, A_CMD_FROM, i);
  }

  // resumo: "P1 > P2" com as cores dos jogadores
  int before = s.p[cmdTarget_].cmdr[cmdFrom_];
  int16_t wp = gLarguraTexto("P1", 2), wg = gLarguraTexto(">", 2), esp = 10;
  int16_t x0 = 200 - (2 * wp + wg + 2 * esp) / 2;
  gTexto(NOME_P[cmdFrom_], x0, 106, 2, COR_JOGADOR[cmdFrom_]);
  gTexto(">", x0 + wp + esp, 106, 2, C_BRANCO);
  gTexto(NOME_P[cmdTarget_], x0 + wp + wg + 2 * esp, 106, 2, COR_JOGADOR[cmdTarget_]);
  snprintf(buf, sizeof buf, "DAMAGE: %+d    FROM P%d: %d > %d", cmdDmg_, cmdFrom_ + 1, before, before + cmdDmg_);
  gTexto(buf, 200, 128, 1, cmdDmg_ ? C_CIANO : C_BRANCO, C_PRETO, CENTRO);
  snprintf(buf, sizeof buf, "P%d HAS TAKEN %d IN TOTAL (21 FROM ONE = OUT)",
           cmdTarget_ + 1, game_.totalCmdrTaken(cmdTarget_));
  gTexto(buf, 200, 142, 1, C_CINZA_C, C_PRETO, CENTRO);
  if (before + cmdDmg_ >= CMDR_LETHAL && before < CMDR_LETHAL)
    gTexto("LETHAL!", 200, 160, 2, C_VERMELHO, C_PRETO, CENTRO);

  // botões
  gCaixa(8, 192, 60, 40, 3, EST_BTN_NORMAL, 2, false);
  gSinal('-', 30, 210, false);
  addHitL(8, 192, 60, 40, A_CMD_DELTA, 0, -1, true);
  gCaixa(73, 192, 60, 40, 3, EST_BTN_NORMAL, 2, false);
  gSinal('+', 95, 204, false);
  addHitL(73, 192, 60, 40, A_CMD_DELTA, 0, +1, true);
  bool pode = cmdDmg_ != 0;
  gBotao(138, 192, 165, 40, pode ? EST_BTN_VERDE : EST_BTN_DESATIVADO, "APPLY", 3, pode ? C_BRANCO : C_CINZA);
  addHitL(138, 192, 165, 40, A_CMD_APPLY);
  gBotao(308, 192, 84, 40, EST_BTN_NORMAL, "BACK", 2);
  addHitL(308, 192, 84, 40, A_CMD_BACK);
}

// cor de cada linha do histórico, pelo tipo de evento
static uint16_t corDoEvento(const char* t) {
  if (strstr(t, "OUT"))      return C_VERMELHO;
  if (strstr(t, "WINS"))     return C_LIMA;
  if (strstr(t, "NEW GAME")) return C_CIANO;
  if (strstr(t, "CMDR"))     return C_OURO;
  if (strstr(t, "POISON"))   return C_LIMA;
  if (strstr(t, "ENERGY"))   return C_CIANO;
  if (strstr(t, "FIRST"))    return C_CREME;
  if (strstr(t, " TURN"))    return C_LARANJA;
  return C_BRANCO;
}

void App::renderLog() {
  gTexto("HISTORY - NEWEST FIRST", 200, 3, 2, C_CIANO, C_PRETO, CENTRO);
  gCaixa(4, 19, 392, 180, 3, EST_LISTA, 1, false);
  int total = game_.logCount();
  int first = logPage_ * 10;
  for (int k = 0; k < 10 && first + k < total; k++) {
    int i = first + k;
    int y = 24 + k * 17;
    char buf[80];
    snprintf(buf, sizeof buf, "#%-3d %s", game_.logNumber(i), game_.logLine(i));
    buf[53] = 0;                                        // cabe numa linha (53 letras)
    gTexto(buf, 12, y + 5, 1, corDoEvento(game_.logLine(i)));
    if (k < 9 && first + k + 1 < total)
      for (int x = 10; x < 390; x += 4) gRet(x, y + 16, 2, 1, C_CINZA);
  }
  bool novo = logPage_ > 0, velho = first + 10 < total;
  gBotao(8, 204, 75, 28, novo ? EST_BTN_NORMAL : EST_BTN_DESATIVADO, "NEWER", 1, novo ? C_BRANCO : C_CINZA);
  addHitL(8, 204, 75, 28, A_LOG_NEWER);
  gBotao(88, 204, 75, 28, velho ? EST_BTN_NORMAL : EST_BTN_DESATIVADO, "OLDER", 1, velho ? C_BRANCO : C_CINZA);
  addHitL(88, 204, 75, 28, A_LOG_OLDER);
  char buf[32];
  if (total) snprintf(buf, sizeof buf, "%d-%d OF %d", first + 1, std::min(first + 10, total), total);
  else snprintf(buf, sizeof buf, "EMPTY");
  gTexto(buf, 235, 214, 1, C_BRANCO, C_PRETO, CENTRO);
  gBotao(307, 204, 85, 28, EST_BTN_NORMAL, "BACK");
  addHitL(307, 204, 85, 28, A_LOG_BACK);
}

void App::renderConfirm() {
  gXadrez(0, 0, TELA_W, TELA_H);                       // escurece o que está atrás
  bool reset = overlay_ == Overlay::ConfirmReset;
  gCaixa(85, 65, 230, 110, 3, EST_CAIXA, 2, false);
  gContorno(91, 71, 218, 98, 2, C_BRANCO);
  gTexto(reset ? "RESET GAME?" : "BACK TO MENU?", 200, 80, 2, C_BRANCO, C_PRETO, CENTRO);
  gTexto(reset ? "EVERYONE BACK TO START LIFE" : "THE GAME IS KEPT: USE RESUME", 200, 102, 1, C_CIANO,
         C_PRETO, CENTRO);
  gBotao(100, 128, 95, 32, EST_BTN_VERMELHO, "YES", 2);
  addHitL(100, 128, 95, 32, A_CONFIRM_YES);
  gBotao(205, 128, 95, 32, EST_BTN_AZUL, "NO", 2);
  addHitL(205, 128, 95, 32, A_CONFIRM_NO);
}
