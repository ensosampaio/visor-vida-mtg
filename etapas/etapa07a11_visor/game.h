// game.h — regras e estado do jogo (sem nada de tela).
// Esta parte é C++ puro: não depende do Arduino nem da tela,
// por isso dá para testar no computador.
#pragma once
#include <stdint.h>

namespace mtg {

constexpr int MAX_PLAYERS   = 4;
constexpr int POISON_LETHAL = 10;   // 10 marcadores de veneno = eliminado
constexpr int CMDR_LETHAL   = 21;   // 21 de dano do MESMO comandante = eliminado
constexpr int MAX_UNDO      = 64;   // quantas ações dá para desfazer
constexpr int MAX_LOG       = 200;  // quantas linhas o histórico guarda
constexpr int LOG_LEN       = 60;   // tamanho máximo de uma linha do histórico
constexpr uint32_t COALESCE_MS = 3000; // toques seguidos em até 3 s viram 1 linha no histórico

enum class Format  : uint8_t { Commander, Standard };
enum class Counter : uint8_t { Life, Poison, Energy };
enum class OutReason : uint8_t { None, Life, Poison, Commander };

struct Player {
  int16_t life;
  int16_t poison;
  int16_t energy;
  int16_t cmdr[MAX_PLAYERS];  // dano de comandante recebido de cada jogador
};

struct State {
  Format  format;
  uint8_t numPlayers;
  int16_t startLife;
  Player  p[MAX_PLAYERS];
  int8_t  turn;     // de quem é o turno (-1 = turnos ainda não começaram)
  int8_t  first;    // quem começou (a rodada sobe quando o turno volta para ele)
  int16_t round;    // rodada atual (1, 2, 3...)
};

class Game {
public:
  void newGame(Format f, uint8_t players, int16_t life);
  void restart() { newGame(s_.format, s_.numPlayers, s_.startLife); }
  bool started() const { return started_; }
  const State& state() const { return s_; }
  int numPlayers() const { return s_.numPlayers; }

  // Soma 'delta' a vida/veneno/energia de um jogador.
  // Toques seguidos no mesmo contador são agrupados numa linha do histórico.
  void change(uint8_t player, Counter c, int16_t delta, uint32_t now);
  // Aplica dano de comandante (também tira vida).
  void applyCommander(uint8_t target, uint8_t from, int16_t dmg, uint32_t now);
  // Registra um evento avulso no histórico (ex.: quem começa). Pode ser desfeito.
  void event(const char* text);

  // Turnos. Tudo entra no histórico e pode ser desfeito.
  void startTurns(uint8_t first);  // 'first' começa; rodada 1
  void passTurn();                 // passa para o próximo jogador vivo (pula eliminados)
  void setTurn(uint8_t p);         // corrige à mão de quem é o turno
  int  turnPlayer() const { return s_.turn; }
  int  round() const { return s_.round; }

  void tick(uint32_t now);  // fecha o agrupamento depois de COALESCE_MS
  void flush();             // fecha o agrupamento agora

  // Apaga o histórico (e o UNDO, que depende dele). A partida continua igual.
  void clearLog();

  bool canUndo() const { return undoCount_ > 0 || pending_.active; }
  bool undo();

  OutReason outReason(uint8_t p) const;
  bool isOut(uint8_t p) const { return outReason(p) != OutReason::None; }
  int  alivePlayers() const;
  int  winner() const;      // índice do vencedor, ou -1
  int  maxCmdrTaken(uint8_t p) const;
  int  totalCmdrTaken(uint8_t p) const;

  // Variação recente de vida (o "-3" que aparece ao lado do nome).
  int16_t recentDelta(uint8_t p, uint32_t now) const;

  // Histórico: i = 0 é a linha mais recente.
  int  logCount() const;
  const char* logLine(int i) const;
  int  logNumber(int i) const;  // número da linha (#1, #2, ...)

private:
  struct Snapshot { State s; int logTotal; };
  struct Pending {
    bool active = false;
    uint8_t player = 0;
    Counter counter = Counter::Life;
    int16_t delta = 0;
    uint32_t last = 0;
    bool wasOut = false;
  };

  void pushUndo();
  void addLog(const char* text);
  void logOutIfNeeded(uint8_t p, bool wasOut);
  int16_t* counterRef(uint8_t p, Counter c);

  State s_{};
  bool started_ = false;
  Pending pending_;

  Snapshot undo_[MAX_UNDO];
  int undoHead_ = 0, undoCount_ = 0;

  char log_[MAX_LOG][LOG_LEN];
  int logTotal_ = 0;  // linhas já escritas (a próxima é #logTotal_+1)

  int16_t recent_[MAX_PLAYERS] = {};
  uint32_t recentAt_[MAX_PLAYERS] = {};
};

}  // namespace mtg
