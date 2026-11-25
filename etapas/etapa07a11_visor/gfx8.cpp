// gfx8.cpp — motor de desenho 8-bit: primitivas, fontes, ícones e componentes.
#include "gfx8.h"
#include "fonte8.h"
#include "fonte_grande.h"

// ------------------------------------------------------------------ estilos
//                                   borda        fundo        textura
const Estilo EST_JOGADOR[4] = {
  {C_AZUL,     C_AZUL_E,   C_AZUL_V},    // P1
  {C_VERDE,    C_VERDE_E,  C_VERDE_E2},  // P2
  {C_VERMELHO, C_VERM_E,   C_VERM_M},    // P3
  {C_MAGENTA,  C_ANIL,     C_VIOLETA},   // P4
};
const uint16_t COR_JOGADOR[4] = {C_AZUL, C_VERDE, C_VERMELHO, C_MAGENTA};
const Estilo EST_ELIMINADO       = {C_CINZA,    C_PRETO,   C_CINZA};
const Estilo EST_BTN_NORMAL      = {C_CIANO,    C_PRETO,   C_CINZA};
const Estilo EST_BTN_AZUL        = {C_AZUL,     C_AZUL_E,  C_AZUL_V};
const Estilo EST_BTN_ROXO        = {C_MAGENTA,  C_PRETO,   C_CINZA};
const Estilo EST_BTN_VERDE       = {C_LIMA,     C_PRETO,   C_CINZA};
const Estilo EST_BTN_VERMELHO    = {C_VERMELHO, C_VERM_M,  C_VERMELHO};
const Estilo EST_BTN_SELECIONADO = {C_OURO,     C_OURO_E,  C_OURO};
const Estilo EST_BTN_DESATIVADO  = {C_CINZA,    C_PRETO,   SEM_TEXTURA};
const Estilo EST_BTN_OURO        = {C_OURO,     C_PRETO,   C_CINZA};
const Estilo EST_BTN_CIANO       = {C_CIANO,    C_PRETO,   C_CINZA};
const Estilo EST_TOPO_NORMAL     = {C_CINZA_C,  C_PRETO,   SEM_TEXTURA};
const Estilo EST_TOPO_ATIVO      = {C_CREME,    C_OURO,    SEM_TEXTURA};
const Estilo EST_TOPO_DESATIVADO = {C_CINZA,    C_PRETO,   SEM_TEXTURA};
const Estilo EST_ICO_COMANDANTE  = {C_OURO,     C_MARROM,  SEM_TEXTURA};
const Estilo EST_ICO_ENERGIA     = {C_CIANO,    C_TEAL_E,  SEM_TEXTURA};
const Estilo EST_ICO_VENENO      = {C_LIMA,     C_VERDE_E, SEM_TEXTURA};

// ------------------------------------------------------------------ transformacao
static int16_t T_ox = 0, T_oy = 0, T_w = TELA_W, T_h = TELA_H;
static bool T_gira = false;
static bool T_escuro = false;

void gOrigem(int16_t ox, int16_t oy, int16_t w, int16_t h, bool girar180) {
  T_ox = ox; T_oy = oy; T_w = w; T_h = h; T_gira = girar180;
}
void gOrigemReset() { gOrigem(0, 0, TELA_W, TELA_H, false); }
void gEscurecer(bool on) { T_escuro = on; }

void gRet(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t c) {
  if (w <= 0 || h <= 0) return;
  if (T_gira) { x = T_w - x - w; y = T_h - y - h; }
  if (T_escuro) c = (c >> 1) & 0x7BEF;          // metade do brilho em RGB565
  portaPreencher((T_ox + x) * ESCALA, (T_oy + y) * ESCALA, w * ESCALA, h * ESCALA, c);
}

// recuo horizontal da linha j de um retangulo chanfrado (cantos em "escadinha")
static inline int16_t recuo(int16_t j, int16_t h, uint8_t n) {
  if (j < n) return n - j;
  if (j >= h - n) return n - (h - 1 - j);
  return 0;
}

void gChanfrado(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t n, uint16_t c) {
  for (uint8_t i = 0; i < n && i < h / 2; i++) {
    int16_t r = n - i;
    gRet(x + r, y + i, w - 2 * r, 1, c);
    gRet(x + r, y + h - 1 - i, w - 2 * r, 1, c);
  }
  gRet(x, y + n, w, h - 2 * n, c);
}

// so a linha de 1 px na borda do chanfrado (interior nao e tocado)
void gContorno(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t n, uint16_t c) {
  for (int16_t j = 0; j < h; j++) {
    int16_t a = recuo(j, h, n), b = w - 1 - a;             // span externo
    if (j == 0 || j == h - 1) { gRet(x + a, y + j, b - a + 1, 1, c); continue; }
    int16_t ri = recuo(j - 1, h - 2, n);
    int16_t ia = 1 + ri, ib = w - 2 - ri;                  // span interno
    if (ia > b || ib < a) { gRet(x + a, y + j, b - a + 1, 1, c); continue; }
    gRet(x + a, y + j, ia - a, 1, c);
    gRet(x + ib + 1, y + j, b - ib, 1, c);
  }
}

// ------------------------------------------------------------------ texto
static void glifo(uint8_t ch, int16_t x, int16_t y, uint8_t k, uint16_t c) {
  if (ch >= 'a' && ch <= 'z') ch -= 32;
  if (ch < FONTE_PRIMEIRO || ch > FONTE_ULTIMO) ch = ' ';
  const uint8_t *g = FONTE8[ch - FONTE_PRIMEIRO];
  uint8_t w = pgm_read_byte(g);
  for (uint8_t r = 0; r < FONTE_ALTURA; r++) {
    uint8_t bits = pgm_read_byte(g + 1 + r);
    int8_t ini = -1;
    for (uint8_t col = 0; col <= w; col++) {        // junta pixels vizinhos num so retangulo
      bool on = col < w && (bits & (0x80 >> col));
      if (on && ini < 0) ini = col;
      if (!on && ini >= 0) { gRet(x + ini * k, y + r * k, (col - ini) * k, k, c); ini = -1; }
    }
  }
}

static uint8_t larguraGlifo(uint8_t ch) {
  if (ch >= 'a' && ch <= 'z') ch -= 32;
  if (ch < FONTE_PRIMEIRO || ch > FONTE_ULTIMO) ch = ' ';
  return pgm_read_byte(FONTE8[ch - FONTE_PRIMEIRO]);
}

int16_t gLarguraTexto(const char *s, uint8_t k) {
  int16_t w = 0; uint8_t n = 0;
  for (; *s; s++, n++) w += larguraGlifo(*s);
  return n ? (w + (n - 1)) * k : 0;
}

int16_t gTexto(const char *s, int16_t x, int16_t y, uint8_t k, uint16_t cor, uint16_t sombra, Alinha al) {
  int16_t w = gLarguraTexto(s, k);
  if (al == CENTRO) x -= w / 2; else if (al == DIR) x -= w;
  for (uint8_t pass = 0; pass < 2; pass++) {
    if (pass == 0 && sombra == SEM_SOMBRA) continue;
    int16_t cx = x + (pass == 0 ? 1 : 0), cy = y + (pass == 0 ? 1 : 0);
    for (const char *p = s; *p; p++) {
      glifo(*p, cx, cy, k, pass == 0 ? sombra : cor);
      cx += (larguraGlifo(*p) + 1) * k;
    }
  }
  return w;
}

int16_t gNumero(int16_t n, int16_t x, int16_t y, uint8_t k, uint16_t cor, uint16_t sombra, Alinha al, bool sinal) {
  char buf[8]; uint8_t i = 7; buf[7] = 0;
  bool neg = n < 0; uint16_t v = neg ? -n : n;
  do { buf[--i] = '0' + v % 10; v /= 10; } while (v && i > 1);
  if (neg) buf[--i] = '-'; else if (sinal) buf[--i] = '+';
  return gTexto(buf + i, x, y, k, cor, sombra, al);
}

// numeros grandes (vida): fonte 8x10 ampliada k vezes, com sombra preta deslocada
int16_t gNumeroGrande(int16_t n, int16_t x, int16_t y, uint8_t k, uint16_t cor, Alinha al, bool sinal) {
  char buf[8]; uint8_t i = 7; buf[7] = 0;
  bool neg = n < 0; uint16_t v = neg ? -n : n;
  do { buf[--i] = '0' + v % 10; v /= 10; } while (v && i > 1);
  if (neg) buf[--i] = '-'; else if (sinal) buf[--i] = '+';
  const char *s = buf + i;
  uint8_t nc = 7 - i;
  int16_t w = (nc * (GRANDE_W + 1) - 1) * k;
  if (al == CENTRO) x -= w / 2; else if (al == DIR) x -= w;
  int8_t d = k > 2 ? 2 : 1;                         // deslocamento da sombra
  for (uint8_t pass = 0; pass < 2; pass++) {
    int16_t cx = x + (pass ? 0 : d), cy = y + (pass ? 0 : d);
    for (const char *p = s; *p; p++, cx += (GRANDE_W + 1) * k) {
      uint8_t idx = *p == '-' ? 10 : *p == '+' ? 11 : *p - '0';
      for (uint8_t r = 0; r < GRANDE_H; r++) {
        uint8_t bits = pgm_read_byte(&FONTE_GRANDE[idx][r]);
        int8_t ini = -1;
        for (uint8_t c = 0; c <= GRANDE_W; c++) {
          bool on = c < GRANDE_W && (bits & (0x80 >> c));
          if (on && ini < 0) ini = c;
          if (!on && ini >= 0) {
            gRet(cx + ini * k, cy + r * k, (c - ini) * k, k, pass ? cor : C_PRETO);
            ini = -1;
          }
        }
      }
    }
  }
  return w;
}

// ------------------------------------------------------------------ componentes
void gCaixa(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t n, const Estilo &e,
            uint8_t esp, bool textura, bool pressionado) {
  gChanfrado(x, y, w, h, n, C_PRETO);
  gChanfrado(x + 1, y + 1, w - 2, h - 2, n, e.borda);
  gChanfrado(x + 1 + esp, y + 1 + esp, w - 2 - 2 * esp, h - 2 - 2 * esp, n, C_PRETO);
  int16_t ix = x + 2 + esp, iy = y + 2 + esp, iw = w - 4 - 2 * esp, ih = h - 4 - 2 * esp;
  gChanfrado(ix, iy, iw, ih, n, e.fundo);
  if (textura && e.textura != SEM_TEXTURA && !pressionado) {
    for (int16_t yy = 3; yy < h; yy += 8)
      for (int16_t xx = 3; xx < w; xx += 8)
        if (xx >= ix - x + 1 && xx < ix - x + iw - 1 && yy >= iy - y + 1 && yy < iy - y + ih - 1)
          gRet(x + xx, y + yy, 1, 1, e.textura);
  }
  if (pressionado) {   // "afundado": xadrez preto nas bordas de cima e da esquerda
    for (int16_t yy = 0; yy < 3; yy++)
      for (int16_t xx = n; xx < iw - n; xx++)
        if (((ix + xx) + (iy + yy)) % 2 == 0) gRet(ix + xx, iy + yy, 1, 1, C_PRETO);
    for (int16_t yy = 3; yy < ih - n; yy++)
      for (int16_t xx = 0; xx < 3; xx++)
        if (((ix + xx) + (iy + yy)) % 2 == 0) gRet(ix + xx, iy + yy, 1, 1, C_PRETO);
  }
}

void gBotao(int16_t x, int16_t y, int16_t w, int16_t h, const Estilo &e, const char *rotulo,
            uint8_t k, uint16_t corTexto, bool pressionado) {
  uint8_t n = h >= 40 ? 4 : (h >= 20 ? 3 : 2);
  uint8_t esp = h >= 20 ? 2 : 1;
  gCaixa(x, y, w, h, n, e, esp, h >= 40, pressionado);
  if (rotulo) {
    int16_t dy = pressionado ? 1 : 0;
    gTexto(rotulo, x + w / 2 + dy, y + (h - 7 * k) / 2 + dy, k, corTexto,
           corTexto == C_PRETO ? SEM_SOMBRA : C_PRETO, CENTRO);
  }
}

void gBotaoTopo(int16_t x, int16_t y, const char *rotulo, uint8_t estado) {
  if (estado == 1)      gBotao(x, y, 41, 17, EST_TOPO_ATIVO, rotulo, 1, C_PRETO);
  else if (estado == 2) gBotao(x, y, 41, 17, EST_TOPO_DESATIVADO, rotulo, 1, C_CINZA);
  else                  gBotao(x, y, 41, 17, EST_TOPO_NORMAL, rotulo, 1, C_BRANCO);
}

void gEtiqueta(int16_t x, int16_t y, uint16_t cor, const char *rotulo) {
  gChanfrado(x, y, 27, 13, 2, C_PRETO);
  gChanfrado(x + 1, y + 1, 25, 11, 2, cor);
  if (rotulo) gTexto(rotulo, x + 13, y + 3, 1, C_BRANCO, C_PRETO, CENTRO);
}

void gSinal(char tipo, int16_t x, int16_t y, bool apagado) {
  // retangulos que formam o sinal (16x16, traco de 4)
  int16_t r[2][4]; uint8_t nr;
  if (tipo == '+') {
    int16_t a[2][4] = {{0, 6, 16, 4}, {6, 0, 4, 16}}; nr = 2;
    for (uint8_t i = 0; i < 2; i++) for (uint8_t j = 0; j < 4; j++) r[i][j] = a[i][j];
  } else {
    r[0][0] = 0; r[0][1] = 0; r[0][2] = 16; r[0][3] = 4; nr = 1;
  }
  for (uint8_t i = 0; i < nr; i++)   // sombra (2 px abaixo/direita) + contorno preto
    gRet(x + r[i][0] + 1, y + r[i][1] + 1, r[i][2] + 2, r[i][3] + 2, C_PRETO);
  for (uint8_t i = 0; i < nr; i++)
    gRet(x + r[i][0] - 1, y + r[i][1] - 1, r[i][2] + 2, r[i][3] + 2, C_PRETO);
  for (uint8_t i = 0; i < nr; i++)
    gRet(x + r[i][0], y + r[i][1], r[i][2], r[i][3], apagado ? C_CINZA : C_BRANCO);
}

void gBarra(int16_t x, int16_t y, int16_t w, uint16_t c) {
  gRet(x, y - 1, w, 5, C_PRETO);
  gRet(x + 1, y, w - 2, 3, c);
  for (int16_t ex = x + 4; ex < x + w - 4; ex += 8) gRet(ex, y + 1, 4, 1, C_PRETO);
}

void gFundoEstrelado() {
  static const uint8_t estrelas[6][2] PROGMEM = {{3, 5}, {19, 13}, {27, 26}, {10, 22}, {24, 3}, {14, 30}};
  const uint16_t cores[6] = {C_AZUL_M, C_CINZA_C, C_AZUL_M, C_TEAL_E, C_TEAL_E, C_AZUL_M};
  gRet(0, 0, TELA_W, TELA_H, C_PRETO);
  for (int16_t ty = 0; ty < TELA_H; ty += 32)
    for (int16_t tx = 0; tx < TELA_W; tx += 32)
      for (uint8_t i = 0; i < 6; i++) {
        int16_t sx = tx + pgm_read_byte(&estrelas[i][0]), sy = ty + pgm_read_byte(&estrelas[i][1]);
        if (sx < TELA_W && sy < TELA_H) gRet(sx, sy, 1, 1, cores[i]);
      }
}

void gXadrez(int16_t x, int16_t y, int16_t w, int16_t h) {
  for (int16_t yy = y; yy < y + h; yy++)
    for (int16_t xx = x + ((yy + x) & 1); xx < x + w; xx += 2) gRet(xx, yy, 1, 1, C_PRETO);
}

// ------------------------------------------------------------------ icones (sprites)
// k = contorno preto, h/m = cor principal, s = sombra, w = branco, r = vermelho, '.' = transparente
static const char COROA_G[] PROGMEM =
  ".........kk........."
  ".kk.....khhk.....kk."
  ".khk...khhhhk...khk."
  ".khhk.khhwwhhk.khhk."
  ".khhhkhhhhhhhhkhhhk."
  ".khhhhhhhhhhhhhhhhk."
  ".kmmmmmmmmmmmmmmmmk."
  ".kmmrrmmmrrmmmrrmmk."
  ".kmmrrmmmrrmmmrrmmk."
  ".kmmmmmmmmmmmmmmmmk."
  ".kssssssssssssssssk."
  ".kkkkkkkkkkkkkkkkkk.";
static const char RAIO_G[] PROGMEM =
  "..........kkkkkk...."
  ".........khhhhk....."
  "........khwhhk......"
  ".......khwhhk......."
  "......khhhhkkkkkk..."
  ".....khhhhhhhhhhk..."
  "....kmmmmmmmmmmk...."
  "...kkkkkkmmmmmk....."
  "........kmmmmk......"
  ".......kmmssk......."
  "......kssssk........"
  "......kkkkk.........";
static const char GOTA_G[] PROGMEM =
  ".........kk........."
  "........khhk........"
  ".......khhhhk......."
  "......khwhhhhk......"
  ".....khwwhhhhhk....."
  ".....khwhhhhhhk....."
  "....khhhhhhhhhhk...."
  "....kmmmmmmmmmmk...."
  "....kmmmmmmmmsmk...."
  ".....kmmmmmmssk....."
  "......kssssssk......"
  ".......kkkkkk.......";
static const char COROA_P[] PROGMEM =
  "........." "h...h...h" "hh.hwh.hh" "hhhhhhhhh" "mmmmmmmmm"
  "mrmmrmmrm" "mmmmmmmmm" "sssssssss" ".........";
static const char RAIO_P[] PROGMEM =
  ".....hhh." "....hwh.." "...hhh..." "..hhhhhh." ".mmmmmm.."
  "....mmm.." "...mms..." "..sss...." ".ss......";
static const char GOTA_P[] PROGMEM =
  "....h...." "...hhh..." "..hwhhh.." "..hwhhh.." ".hhhhhhh."
  ".mmmmmmm." ".mmmmmsm." "..mmmss.." "...sss...";

void gIcone(Icone ic, bool grande, int16_t x, int16_t y) {
  const char *spr; uint16_t cor, esc;
  switch (ic) {
    case ICO_COROA: spr = grande ? COROA_G : COROA_P; cor = C_OURO; esc = C_OURO_E; break;
    case ICO_RAIO:  spr = grande ? RAIO_G : RAIO_P;   cor = C_CIANO; esc = C_AZUL; break;
    default:        spr = grande ? GOTA_G : GOTA_P;   cor = C_LIMA; esc = C_VERDE_M; break;
  }
  uint8_t w = grande ? 20 : 9, h = grande ? 12 : 9;
  for (uint8_t r = 0; r < h; r++) {
    uint8_t ini = 0; char atual = pgm_read_byte(spr + r * w);
    for (uint8_t c = 1; c <= w; c++) {
      char ch = c < w ? pgm_read_byte(spr + r * w + c) : 0;
      if (ch != atual) {
        if (atual != '.') {
          uint16_t k = atual == 'k' ? C_PRETO : atual == 's' ? esc : atual == 'w' ? C_BRANCO
                     : atual == 'r' ? C_VERMELHO : cor;
          gRet(x + ini, y + r, c - ini, 1, k);
        }
        ini = c; atual = ch;
      }
    }
  }
}

void gBotaoIcone(int16_t x, int16_t y, Icone ic) {
  const Estilo &e = ic == ICO_COROA ? EST_ICO_COMANDANTE : ic == ICO_RAIO ? EST_ICO_ENERGIA : EST_ICO_VENENO;
  gCaixa(x, y, 31, 17, 2, e, 1, false);
  gIcone(ic, true, x + 5, y + 2);
}
