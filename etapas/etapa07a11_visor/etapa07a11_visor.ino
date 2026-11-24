// ============================================================================
//  ETAPAS 7 a 11 — O visor completo
//  Visor de Vida MTG
//
//  Este é o programa inteiro do contador. Ele foi dividido em arquivos:
//    etapa07a11_visor.ino -> este arquivo: liga a tela e repassa os toques (bem curto)
//    game.h / game.cpp    -> AS REGRAS: vida, veneno, energia, comandante, eliminação,
//                            desfazer (UNDO) e histórico. Não sabe nada de tela.
//    ui.h / ui.cpp        -> A INTERFACE: desenha cada tela e decide o que cada toque faz.
//    placa.h              -> escolhe entre a placa de verdade e o simulador do PC.
//
//  O README.md desta pasta mostra qual trecho do código corresponde a cada etapa (7 a 11).
//  Na IDE: Placa "ESP32S3 Dev Module", Flash Size "16MB", PSRAM "OPI PSRAM".
// ============================================================================

#include <LovyanGFX.hpp>
#include "placa.h"
#include "ui.h"

static LGFX_Sprite canvas(&lcd);   // "tela invisível" onde cada quadro é desenhado
static App* app = nullptr;         // a interface (definida em ui.h / ui.cpp)

// Três funções que a interface usa e que dependem do hardware:
static void present()                { canvas.pushSprite(0, 0); }   // mostra o quadro pronto
static void setBrightness(uint8_t v) { lcd.setBrightness(v); }       // brilho da tela
static uint32_t randomN(uint32_t n)  { return random(n); }           // sorteio do FIRST

void setup() {
  Serial.begin(115200);
  lcd.init();
  lcd.setRotation(0);

  canvas.setColorDepth(16);
  canvas.setPsram(true);   // 800 x 480 x 2 bytes = 768 KB: precisa da PSRAM na placa
  if (!canvas.createSprite(lcd.width(), lcd.height())) {
    Serial.println("ERRO: sem memoria. Confira PSRAM = OPI PSRAM no menu Ferramentas.");
    lcd.fillScreen(TFT_RED);
    while (true) delay(1000);
  }

  app = new App(canvas, PlatformHooks{present, setBrightness, randomN});
  app->begin();
}

void loop() {
  int32_t x = 0, y = 0;
  bool touched = lcd.getTouch(&x, &y);
  app->update(touched, x, y, millis());   // a interface faz todo o resto
  delay(10);
}
