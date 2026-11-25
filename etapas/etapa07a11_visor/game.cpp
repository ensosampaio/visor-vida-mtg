// game.cpp — implementação das regras do jogo.
#include "game.h"
#include <stdio.h>
#include <string.h>

namespace mtg {

static const char* counterName(Counter c) {
  switch (c) {
    case Counter::Life:   return "LIFE";
    case Counter::Poison: return "POISON";
    default:              return "ENERGY";
  }
}

void Game::newGame(Format f, uint8_t players, int16_t life) {
  if (players < 1) players = 1;
  if (players > MAX_PLAYERS) players = MAX_PLAYERS;
  s_ = State{};
  s_.format = f;
  s_.numPlayers = players;
  s_.startLife = life;
  for (int i = 0; i < MAX_PLAYERS; i++) s_.p[i].life = life;
  s_.turn = -1;
  s_.first = -1;
  s_.round = 0;
  pending_ = Pending{};
  undoHead_ = undoCount_ = 0;
  logTotal_ = 0;
  for (int i = 0; i < MAX_PLAYERS; i++) { recent_[i] = 0; recentAt_[i] = 0; }
  started_ = true;

  char buf[LOG_LEN];
  snprintf(buf, sizeof buf, "NEW GAME: %d PLAYER%s, %d LIFE",
           players, players > 1 ? "S" : "", life);
  addLog(buf);
}

int16_t* Game::counterRef(uint8_t p, Counter c) {
  switch (c) {
    case Counter::Life:   return &s_.p[p].life;
    case Counter::Poison: return &s_.p[p].poison;
    default:              return &s_.p[p].energy;
  }
}

void Game::change(uint8_t player, Counter c, int16_t delta, uint32_t now) {
  if (player >= s_.numPlayers || delta == 0) return;

  // Mudou de jogador ou de contador? Fecha o agrupamento anterior.
  if (pending_.active && (pending_.player != player || pending_.counter != c)) flush();

  int16_t* v = counterRef(player, c);
  int16_t before = *v;
  int32_t after = before + delta;
  if (c != Counter::Life && after < 0) after = 0;       // veneno/energia não ficam negativos
  if (after > 999) after = 999;
  if (after < -999) after = -999;
  if (after == before) return;

  if (!pending_.active) {
    pushUndo();
    pending_.active = true;
    pending_.player = player;
    pending_.counter = c;
    pending_.delta = 0;
    pending_.wasOut = isOut(player);
  }
  *v = (int16_t)after;
  pending_.delta += (int16_t)(after - before);
  pending_.last = now;

  if (c == Counter::Life) {
    if (now - recentAt_[player] > COALESCE_MS) recent_[player] = 0;
    recent_[player] += (int16_t)(after - before);
    recentAt_[player] = now;
  }
}

void Game::flush() {
  if (!pending_.active) return;
  Pending p = pending_;
  pending_.active = false;

  if (p.delta == 0) {          // subiu e desceu de volta: não registra nada
    if (undoCount_ > 0) { undoCount_--; undoHead_ = (undoHead_ + MAX_UNDO - 1) % MAX_UNDO; }
    return;
  }
  char buf[LOG_LEN];
  int value = *counterRef(p.player, p.counter);
  if (p.counter == Counter::Life) {
    snprintf(buf, sizeof buf, "P%d LIFE %+d = %d", p.player + 1, p.delta, value);
  } else {
    snprintf(buf, sizeof buf, "P%d %s %+d = %d  (LIFE %d)", p.player + 1,
             counterName(p.counter), p.delta, value, s_.p[p.player].life);
  }
  addLog(buf);
  logOutIfNeeded(p.player, p.wasOut);
}

void Game::tick(uint32_t now) {
  if (pending_.active && now - pending_.last > COALESCE_MS) flush();
}

void Game::applyCommander(uint8_t target, uint8_t from, int16_t dmg, uint32_t now) {
  if (target >= s_.numPlayers || from >= s_.numPlayers || dmg == 0) return;
  flush();
  Player& t = s_.p[target];
  if (t.cmdr[from] + dmg < 0) dmg = -t.cmdr[from];   // não deixa o total ficar negativo
  if (dmg == 0) return;

  bool wasOut = isOut(target);
  pushUndo();
  t.cmdr[from] += dmg;
  t.life -= dmg;

  if (now - recentAt_[target] > COALESCE_MS) recent_[target] = 0;
  recent_[target] -= dmg;
  recentAt_[target] = now;

  char buf[LOG_LEN];
  snprintf(buf, sizeof buf, "P%d CMDR FROM P%d %+d = %d  (LIFE %d)",
           target + 1, from + 1, dmg, t.cmdr[from], t.life);
  addLog(buf);
  logOutIfNeeded(target, wasOut);
}

void Game::event(const char* text) {
  flush();
  pushUndo();
  addLog(text);
}

void Game::startTurns(uint8_t first) {
  if (first >= s_.numPlayers) return;
  flush();
  pushUndo();
  s_.turn = first;
  s_.first = first;
  s_.round = 1;
  char buf[LOG_LEN];
  snprintf(buf, sizeof buf, "P%d GOES FIRST - ROUND 1", first + 1);
  addLog(buf);
}

void Game::passTurn() {
  if (s_.turn < 0) {                      // ninguém começou ainda: começa o primeiro vivo
    for (int i = 0; i < s_.numPlayers; i++) if (!isOut(i)) { startTurns(i); return; }
    return;
  }
  if (winner() >= 0 || alivePlayers() == 0) return;   // partida acabou
  int n = s_.numPlayers, next = -1;
  bool newRound = false;
  for (int k = 1; k <= n; k++) {
    int idx = (s_.turn + k) % n;
    if (idx == s_.first) newRound = true;  // deu a volta na mesa
    if (!isOut(idx)) { next = idx; break; }
  }
  if (next < 0) return;
  flush();
  pushUndo();
  s_.turn = (int8_t)next;
  if (newRound) s_.round++;
  char buf[LOG_LEN];
  snprintf(buf, sizeof buf, "P%d TURN - ROUND %d", next + 1, s_.round);
  addLog(buf);
}

void Game::setTurn(uint8_t p) {
  if (p >= s_.numPlayers) return;
  if (s_.turn < 0) { startTurns(p); return; }
  if (s_.turn == p) return;
  flush();
  pushUndo();
  s_.turn = (int8_t)p;
  char buf[LOG_LEN];
  snprintf(buf, sizeof buf, "P%d TURN (SET BY HAND)", p + 1);
  addLog(buf);
}

void Game::logOutIfNeeded(uint8_t p, bool wasOut) {
  OutReason r = outReason(p);
  if (wasOut || r == OutReason::None) return;
  char buf[LOG_LEN];
  switch (r) {
    case OutReason::Life:
      snprintf(buf, sizeof buf, "P%d OUT  (LIFE %d)", p + 1, s_.p[p].life); break;
    case OutReason::Poison:
      snprintf(buf, sizeof buf, "P%d OUT  (%d POISON)", p + 1, s_.p[p].poison); break;
    default:
      snprintf(buf, sizeof buf, "P%d OUT  (%d COMMANDER)", p + 1, maxCmdrTaken(p)); break;
  }
  addLog(buf);
  int w = winner();
  if (w >= 0) {
    snprintf(buf, sizeof buf, "P%d WINS!", w + 1);
    addLog(buf);
  }
}

void Game::pushUndo() {
  undo_[undoHead_].s = s_;
  undo_[undoHead_].logTotal = logTotal_;
  undoHead_ = (undoHead_ + 1) % MAX_UNDO;
  if (undoCount_ < MAX_UNDO) undoCount_++;
}

void Game::clearLog() {
  flush();
  logTotal_ = 0;
  undoHead_ = undoCount_ = 0;   // as "fotos" do UNDO apontam para linhas que deixaram de existir
  addLog("HISTORY CLEARED");
}

bool Game::undo() {
  flush();
  if (undoCount_ == 0) return false;
  undoHead_ = (undoHead_ + MAX_UNDO - 1) % MAX_UNDO;
  undoCount_--;
  s_ = undo_[undoHead_].s;
  logTotal_ = undo_[undoHead_].logTotal;
  for (int i = 0; i < MAX_PLAYERS; i++) recent_[i] = 0;
  return true;
}

OutReason Game::outReason(uint8_t p) const {
  if (p >= s_.numPlayers) return OutReason::None;
  const Player& pl = s_.p[p];
  if (pl.poison >= POISON_LETHAL) return OutReason::Poison;
  for (int i = 0; i < s_.numPlayers; i++)
    if (pl.cmdr[i] >= CMDR_LETHAL) return OutReason::Commander;
  if (pl.life <= 0) return OutReason::Life;
  return OutReason::None;
}

int Game::alivePlayers() const {
  int n = 0;
  for (int i = 0; i < s_.numPlayers; i++) if (!isOut(i)) n++;
  return n;
}

int Game::winner() const {
  if (s_.numPlayers < 2 || alivePlayers() != 1) return -1;
  for (int i = 0; i < s_.numPlayers; i++) if (!isOut(i)) return i;
  return -1;
}

int Game::maxCmdrTaken(uint8_t p) const {
  int m = 0;
  for (int i = 0; i < s_.numPlayers; i++) if (s_.p[p].cmdr[i] > m) m = s_.p[p].cmdr[i];
  return m;
}

int Game::totalCmdrTaken(uint8_t p) const {
  int t = 0;
  for (int i = 0; i < s_.numPlayers; i++) t += s_.p[p].cmdr[i];
  return t;
}

int16_t Game::recentDelta(uint8_t p, uint32_t now) const {
  if (p >= MAX_PLAYERS || now - recentAt_[p] > COALESCE_MS) return 0;
  return recent_[p];
}

void Game::addLog(const char* text) {
  strncpy(log_[logTotal_ % MAX_LOG], text, LOG_LEN - 1);
  log_[logTotal_ % MAX_LOG][LOG_LEN - 1] = 0;
  logTotal_++;
}

int Game::logCount() const { return logTotal_ < MAX_LOG ? logTotal_ : MAX_LOG; }

const char* Game::logLine(int i) const {
  if (i < 0 || i >= logCount()) return "";
  return log_[(logTotal_ - 1 - i) % MAX_LOG];
}

int Game::logNumber(int i) const { return logTotal_ - i; }

}  // namespace mtg
