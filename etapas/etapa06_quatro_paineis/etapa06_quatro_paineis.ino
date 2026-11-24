// ============================================================================
//  ETAPA 6 — Quatro painéis, com os dois de cima girados 180°
//  Visor de Vida MTG
//
//  Conceitos novos:
//    - struct + array: cada jogador tem sua vida, variação e posição na tela;
//    - girar 180° um desenho: inverter a ordem dos pixels do sprite;
//    - converter o toque da tela para as coordenadas de um painel girado.
// ============================================================================

#include <LovyanGFX.hpp>
#include <algorithm>       // std::reverse
#include "placa.h"

const uint32_t COR_FUNDO  = 0x061055;
const uint32_t COR_PAINEL = 0x1636D8;
const uint32_t COR_CAIXA  = 0x3A66FF;
const uint32_t COR_LIMAO  = 0xA6FF3C;
const uint32_t COR_BRANCO = 0xF2F7FF;
const uint32_t COR_VERMELHO = 0xFF4040;
const uint32_t COR_JOGADOR[4] = {0x5EC8FF, 0x3FE0C8, 0xA6FF3C, 0xC8B4FF};
const uint32_t TEMPO_VARIACAO = 3000;

struct Jogador {
  int x, y, w, h;          // retângulo do painel na tela
  bool girado;             // true = de cabeça para baixo (lado de lá da mesa)
  int vida;
  int variacao;
  uint32_t ultimaMudanca;
};

// P1 e P2 em cima (girados), P3 e P4 embaixo — igual ao vídeo
Jogador jog[4] = {
  {  6,  48, 390, 194, true,  40, 0, 0},
  {404,  48, 390, 194, true,  40, 0, 0},
  {  6, 286, 390, 194, false, 40, 0, 0},
  {404, 286, 390, 194, false, 40, 0, 0},
};

LGFX_Sprite tela(&lcd);     // quadro inteiro (800x480)
LGFX_Sprite painel(&tela);  // um painel por vez (390x194), depois copiado para "tela"

bool precisaDesenhar = true;
bool dedoNaTela = false;
int  segJogador = -1, segBotao = 0;   // qual jogador/botão está sendo segurado
uint32_t proximaRepeticao = 0;
int  repeticoes = 0;

void mudaVida(int j, int delta) {
  Jogador& p = jog[j];
  p.vida += delta;
  if (millis() - p.ultimaMudanca > TEMPO_VARIACAO) p.variacao = 0;
  p.variacao += delta;
  p.ultimaMudanca = millis();
  precisaDesenhar = true;
}

// Desenha o painel do jogador j no sprite "painel", sempre "em pé".
void desenhaPainel(int j) {
  Jogador& p = jog[j];
  int w = p.w, h = p.h, cx = w / 2, cy = h / 2 - 8, zona = w / 3;

  painel.fillScreen(COR_FUNDO);
  painel.fillRoundRect(0, 0, w, h, 10, COR_PAINEL);
  painel.drawRoundRect(0, 0, w, h, 10, COR_LIMAO);
  painel.drawRoundRect(1, 1, w - 2, h - 2, 9, COR_LIMAO);

  painel.fillRect(zona / 2 - 18, cy - 3, 36, 7, COR_LIMAO);              // −
  painel.fillRect(w - zona / 2 - 18, cy - 3, 36, 7, COR_LIMAO);          // +
  painel.fillRect(w - zona / 2 - 3, cy - 18, 7, 36, COR_LIMAO);

  char texto[16];
  snprintf(texto, sizeof texto, "%d", p.vida);
  painel.setFont(&fonts::Font7);
  painel.setTextSize(2);
  int tw = painel.textWidth(texto), th = painel.fontHeight();
  painel.fillRoundRect(cx - tw / 2 - 12, cy - th / 2 - 8, tw + 24, th + 16, 8, COR_CAIXA);
  painel.setTextDatum(middle_center);
  painel.setTextColor(p.vida > 0 ? COR_BRANCO : COR_VERMELHO);
  painel.drawString(texto, cx, cy);
  painel.setTextSize(1);

  int tagY = h - 34;
  painel.fillRoundRect(cx - 27, tagY, 54, 26, 5, COR_JOGADOR[j]);
  snprintf(texto, sizeof texto, "P%d", j + 1);
  painel.setFont(&fonts::FreeSansBold9pt7b);
  painel.setTextColor(COR_FUNDO);
  painel.drawString(texto, cx, tagY + 14);

  if (p.variacao != 0 && millis() - p.ultimaMudanca <= TEMPO_VARIACAO) {
    snprintf(texto, sizeof texto, "%+d", p.variacao);
    painel.setFont(&fonts::FreeSansBold12pt7b);
    painel.setTextDatum(middle_right);
    painel.setTextColor(p.variacao < 0 ? COR_VERMELHO : COR_LIMAO);
    painel.drawString(texto, cx - 40, tagY + 13);
  }

  // O truque do 180°: inverter a ordem de TODOS os pixels do sprite.
  // O primeiro pixel (canto superior esquerdo) vira o último (canto inferior direito), e assim por diante.
  if (p.girado) {
    uint16_t* pixels = (uint16_t*)painel.getBuffer();
    std::reverse(pixels, pixels + w * h);
  }
  painel.pushSprite(p.x, p.y);   // copia o painel para a tela invisível, na posição dele
}

void desenha() {
  tela.fillScreen(COR_FUNDO);
  tela.setTextDatum(middle_left);
  tela.setFont(&fonts::FreeSansBold12pt7b);
  tela.setTextColor(COR_LIMAO);
  tela.drawString("COMMANDER - 4 JOGADORES", 14, 24);
  tela.fillRect(10, 263, 780, 3, COR_LIMAO);          // faixa do meio
  for (int j = 0; j < 4; j++) desenhaPainel(j);
  tela.pushSprite(0, 0);
}

// Em qual jogador e botão caiu o toque (x, y)? Devolve false se fora dos botões.
bool achaBotao(int x, int y, int& j, int& botao) {
  for (j = 0; j < 4; j++) {
    Jogador& p = jog[j];
    if (x < p.x || x >= p.x + p.w || y < p.y || y >= p.y + p.h) continue;
    int lx = x - p.x;                       // posição dentro do painel
    if (p.girado) lx = p.w - 1 - lx;        // painel girado: esquerda e direita trocam
    int zona = p.w / 3;
    if (lx < zona) { botao = -1; return true; }
    if (lx >= p.w - zona) { botao = +1; return true; }
    return false;
  }
  return false;
}

void setup() {
  Serial.begin(115200);
  lcd.init();
  lcd.setBrightness(200);
  tela.setColorDepth(16);
  tela.setPsram(true);
  tela.createSprite(lcd.width(), lcd.height());
  painel.setColorDepth(16);
  painel.setPsram(true);
  painel.createSprite(390, 194);
}

void loop() {
  int32_t x, y;
  bool tocando = lcd.getTouch(&x, &y);
  int j = -1, botao = 0;
  bool emBotao = tocando && achaBotao(x, y, j, botao);

  if (tocando && !dedoNaTela) {
    if (emBotao) {
      mudaVida(j, botao);
      segJogador = j; segBotao = botao;
      repeticoes = 0;
      proximaRepeticao = millis() + 450;
    }
  } else if (tocando && segJogador >= 0) {
    if (!emBotao || j != segJogador || botao != segBotao) {
      segJogador = -1;
    } else if (millis() >= proximaRepeticao) {
      repeticoes++;
      mudaVida(segJogador, segBotao * (repeticoes >= 10 ? 5 : 1));
      proximaRepeticao = millis() + 140;
    }
  } else if (!tocando) {
    segJogador = -1;
  }
  dedoNaTela = tocando;

  // a variação ("-3") de cada jogador some sozinha depois de 3 s
  for (int k = 0; k < 4; k++)
    if (jog[k].variacao != 0 && millis() - jog[k].ultimaMudanca > TEMPO_VARIACAO) {
      jog[k].variacao = 0;
      precisaDesenhar = true;
    }

  if (precisaDesenhar) {
    desenha();
    precisaDesenhar = false;
  }
  delay(10);
}
