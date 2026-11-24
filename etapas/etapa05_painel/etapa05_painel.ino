// ============================================================================
//  ETAPA 5 — Um painel de jogador com a vida e os botões + e −
//  Visor de Vida MTG
//
//  Conceitos novos:
//    - SPRITE: uma "tela invisível" na memória. Desenhamos o quadro inteiro nela
//      e copiamos de uma vez para o display -> a tela não pisca;
//    - redesenhar só quando algo muda (variável "precisaDesenhar");
//    - segurar o botão repete a ação (e acelera depois de um tempo);
//    - mostrar a variação recente (ex.: "-3") por alguns segundos.
// ============================================================================

#include <LovyanGFX.hpp>
#include "placa.h"

const uint32_t COR_FUNDO  = 0x061055;
const uint32_t COR_PAINEL = 0x1636D8;
const uint32_t COR_CAIXA  = 0x3A66FF;
const uint32_t COR_LIMAO  = 0xA6FF3C;
const uint32_t COR_BRANCO = 0xF2F7FF;
const uint32_t COR_VERMELHO = 0xFF4040;
const uint32_t COR_P1     = 0x5EC8FF;

LGFX_Sprite tela(&lcd);     // a tela invisível, do tamanho do display

int vida = 40;
int variacao = 0;            // soma dos toques recentes (o "-3")
uint32_t ultimaMudanca = 0;  // quando a vida mudou pela última vez (ms)
const uint32_t TEMPO_VARIACAO = 3000;   // a variação some depois de 3 s

// Área do painel na tela e as zonas de toque (terço esquerdo = −, terço direito = +)
const int PX = 100, PY = 60, PW = 600, PH = 360;
const int ZONA = PW / 3;

bool precisaDesenhar = true;
bool dedoNaTela = false;
int  botaoSegurado = 0;      // -1 = segurando o −, +1 = segurando o +, 0 = nenhum
uint32_t proximaRepeticao = 0;
int  repeticoes = 0;

void mudaVida(int delta) {
  vida += delta;
  if (millis() - ultimaMudanca > TEMPO_VARIACAO) variacao = 0;   // começa uma nova contagem
  variacao += delta;
  ultimaMudanca = millis();
  precisaDesenhar = true;
  Serial.printf("vida %+d = %d\n", delta, vida);
}

void desenha() {
  tela.fillScreen(COR_FUNDO);

  // painel com borda dupla
  tela.fillRoundRect(PX, PY, PW, PH, 14, COR_PAINEL);
  tela.drawRoundRect(PX, PY, PW, PH, 14, COR_LIMAO);
  tela.drawRoundRect(PX + 1, PY + 1, PW - 2, PH - 2, 13, COR_LIMAO);

  int cx = PX + PW / 2, cy = PY + PH / 2 - 10;

  // sinais de − e + (retângulos, mais nítidos que um caractere)
  tela.fillRect(PX + ZONA / 2 - 26, cy - 5, 52, 11, COR_LIMAO);                 // −
  tela.fillRect(PX + PW - ZONA / 2 - 26, cy - 5, 52, 11, COR_LIMAO);            // + (barra deitada)
  tela.fillRect(PX + PW - ZONA / 2 - 5, cy - 26, 11, 52, COR_LIMAO);            // + (barra em pé)

  // vida num quadro, com fonte de 7 segmentos em tamanho 3
  char texto[16];
  snprintf(texto, sizeof texto, "%d", vida);
  tela.setFont(&fonts::Font7);
  tela.setTextSize(3);
  int tw = tela.textWidth(texto), th = tela.fontHeight();
  tela.fillRoundRect(cx - tw / 2 - 16, cy - th / 2 - 12, tw + 32, th + 24, 10, COR_CAIXA);
  tela.setTextDatum(middle_center);
  tela.setTextColor(vida > 0 ? COR_BRANCO : COR_VERMELHO);
  tela.drawString(texto, cx, cy);
  tela.setTextSize(1);

  // etiqueta do jogador
  int tagY = PY + PH - 40;
  tela.fillRoundRect(cx - 30, tagY, 60, 28, 6, COR_P1);
  tela.setFont(&fonts::FreeSansBold12pt7b);
  tela.setTextColor(COR_FUNDO);
  tela.drawString("P1", cx, tagY + 15);

  // variação recente, ao lado da etiqueta
  if (variacao != 0 && millis() - ultimaMudanca <= TEMPO_VARIACAO) {
    snprintf(texto, sizeof texto, "%+d", variacao);
    tela.setTextDatum(middle_right);
    tela.setTextColor(variacao < 0 ? COR_VERMELHO : COR_LIMAO);
    tela.drawString(texto, cx - 44, tagY + 14);
  }

  // dica no rodapé
  tela.setTextDatum(middle_center);
  tela.setFont(&fonts::FreeSans9pt7b);
  tela.setTextColor(COR_BRANCO);
  tela.drawString("Toque nas laterais do painel. Segure para repetir.", 400, 455);

  tela.pushSprite(0, 0);   // copia a tela invisível inteira para o display
}

void setup() {
  Serial.begin(115200);
  lcd.init();
  lcd.setBrightness(200);

  // 800 x 480 pixels x 2 bytes = 768 KB: na placa isso vai para a PSRAM
  tela.setColorDepth(16);
  tela.setPsram(true);
  tela.createSprite(lcd.width(), lcd.height());
}

void loop() {
  int32_t x, y;
  bool tocando = lcd.getTouch(&x, &y);

  // Em qual botão o dedo está? (−1, +1 ou 0 = fora dos botões)
  int botao = 0;
  if (tocando && y >= PY && y < PY + PH) {
    if (x >= PX && x < PX + ZONA) botao = -1;
    else if (x >= PX + PW - ZONA && x < PX + PW) botao = +1;
  }

  if (tocando && !dedoNaTela) {                 // acabou de encostar
    if (botao != 0) {
      mudaVida(botao);
      botaoSegurado = botao;
      repeticoes = 0;
      proximaRepeticao = millis() + 450;        // espera um pouco antes de repetir
    }
  } else if (tocando && botaoSegurado != 0) {   // continua segurando
    if (botao != botaoSegurado) {
      botaoSegurado = 0;                        // escorregou para fora do botão: para
    } else if (millis() >= proximaRepeticao) {
      repeticoes++;
      mudaVida(botaoSegurado * (repeticoes >= 10 ? 5 : 1));   // depois de 10 repetições, de 5 em 5
      proximaRepeticao = millis() + 140;
    }
  } else if (!tocando) {
    botaoSegurado = 0;
  }
  dedoNaTela = tocando;

  // a variação some sozinha depois de 3 s: precisa redesenhar quando o tempo acaba
  static bool mostrandoVariacao = false;
  bool deveMostrar = variacao != 0 && millis() - ultimaMudanca <= TEMPO_VARIACAO;
  if (deveMostrar != mostrandoVariacao) { mostrandoVariacao = deveMostrar; precisaDesenhar = true; }

  if (precisaDesenhar) {
    desenha();
    precisaDesenhar = false;
  }
  delay(10);
}
