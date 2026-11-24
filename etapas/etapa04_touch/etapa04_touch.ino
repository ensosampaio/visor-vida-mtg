// ============================================================================
//  ETAPA 4 — Testar o touch: desenhar um ponto onde o dedo encosta
//  Visor de Vida MTG
//
//  Conceitos novos:
//    - lcd.getTouch(&x, &y): diz se há um dedo na tela e onde ele está;
//    - "acabou de encostar" x "continua encostado" (detecção de borda);
//    - um botão simples: um retângulo + um teste "o toque caiu dentro dele?".
//  No simulador, o clique do mouse faz o papel do dedo.
// ============================================================================

#include <LovyanGFX.hpp>
#include "placa.h"

const uint32_t COR_FUNDO  = 0x061055;
const uint32_t COR_PAINEL = 0x1636D8;
const uint32_t COR_LIMAO  = 0xA6FF3C;
const uint32_t COR_BRANCO = 0xF2F7FF;
const uint32_t COR_CIANO  = 0x3FD2FF;

// Um botão é só um retângulo na tela
struct Botao { int x, y, w, h; };
const Botao BOTAO_LIMPAR = {640, 8, 150, 44};

bool dedoNaTela = false;   // o dedo estava na tela na volta anterior do loop?
int  toques = 0;           // quantas vezes o dedo encostou

// O ponto (px, py) está dentro do botão?
bool dentroDe(const Botao& b, int px, int py) {
  return px >= b.x && px < b.x + b.w && py >= b.y && py < b.y + b.h;
}

void desenhaBarraDeCima(const char* mensagem) {
  lcd.fillRect(0, 0, 800, 60, COR_PAINEL);
  lcd.drawFastHLine(0, 60, 800, COR_LIMAO);
  lcd.setTextDatum(middle_left);
  lcd.setFont(&fonts::FreeSansBold12pt7b);
  lcd.setTextColor(COR_BRANCO);
  lcd.drawString(mensagem, 16, 30);

  // botão LIMPAR
  lcd.fillRoundRect(BOTAO_LIMPAR.x, BOTAO_LIMPAR.y, BOTAO_LIMPAR.w, BOTAO_LIMPAR.h, 8, COR_FUNDO);
  lcd.drawRoundRect(BOTAO_LIMPAR.x, BOTAO_LIMPAR.y, BOTAO_LIMPAR.w, BOTAO_LIMPAR.h, 8, COR_LIMAO);
  lcd.setTextDatum(middle_center);
  lcd.setTextColor(COR_LIMAO);
  lcd.drawString("LIMPAR", BOTAO_LIMPAR.x + BOTAO_LIMPAR.w / 2, BOTAO_LIMPAR.y + BOTAO_LIMPAR.h / 2);
}

void limpaTela() {
  lcd.fillScreen(COR_FUNDO);
  desenhaBarraDeCima("Toque em qualquer lugar da tela");
  toques = 0;
}

void setup() {
  Serial.begin(115200);
  lcd.init();
  lcd.setBrightness(200);
  limpaTela();
}

void loop() {
  int32_t x, y;
  bool tocando = lcd.getTouch(&x, &y);   // true se há um dedo na tela agora

  if (tocando && !dedoNaTela) {
    // ---- o dedo ACABOU de encostar (só acontece uma vez por toque) ----
    if (dentroDe(BOTAO_LIMPAR, x, y)) {
      limpaTela();
    } else {
      toques++;
      lcd.fillCircle(x, y, 14, COR_CIANO);        // marca o ponto do toque
      lcd.drawCircle(x, y, 22, COR_LIMAO);

      char texto[48];
      snprintf(texto, sizeof texto, "Toque %d:  x = %d   y = %d", toques, (int)x, (int)y);
      desenhaBarraDeCima(texto);
      Serial.println(texto);
    }
  } else if (tocando && dedoNaTela) {
    // ---- o dedo CONTINUA na tela (arrastando): desenha um rastro fino ----
    if (!dentroDe(BOTAO_LIMPAR, x, y) && y > 62) lcd.fillCircle(x, y, 3, COR_LIMAO);
  }

  dedoNaTela = tocando;   // lembra para a próxima volta
  delay(10);
}
