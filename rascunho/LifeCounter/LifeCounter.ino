// ============================================================================
//  LIFE COUNTER — Magic: The Gathering
//  Placa: Sunton ESP32-8048S043C (ESP32-S3 + tela 4.3" 800x480 + touch capacitivo GT911)
//
//  Configuração na IDE do Arduino (menu Ferramentas / Tools):
//    Placa ............ ESP32S3 Dev Module
//    Flash Size ....... 16MB (128Mb)
//    PSRAM ............ OPI PSRAM          <- obrigatório (a tela usa a PSRAM)
//    Partition Scheme . 16M Flash (3MB APP/9.9MB FATFS)  (ou qualquer uma de 16MB)
//  Biblioteca necessária: LovyanGFX (instale pelo Gerenciador de Bibliotecas)
// ============================================================================

// A LovyanGFX já traz a configuração pronta desta placa (pinos da tela e do touch).
#include <LovyanGFX.hpp>
#include <lgfx_user/LGFX_ESP32S3_RGB_ESP32-8048S043.h>
#include "ui.h"

static LGFX lcd;                    // o display de verdade
static LGFX_Sprite canvas(&lcd);    // "tela invisível" onde desenhamos cada quadro
static App* app = nullptr;

static void present()                  { canvas.pushSprite(0, 0); }
static void setBrightness(uint8_t v)   { lcd.setBrightness(v); }
static uint32_t randomN(uint32_t n)    { return esp_random() % n; }

void setup() {
  Serial.begin(115200);
  lcd.init();
  lcd.setRotation(0);   // paisagem, conector USB à esquerda. Teste 2 se quiser a tela "de ponta-cabeça".

  canvas.setColorDepth(16);
  canvas.setPsram(true);
  if (!canvas.createSprite(lcd.width(), lcd.height())) {
    Serial.println("ERRO: sem memoria para o sprite. Confira se PSRAM = OPI PSRAM no menu Ferramentas.");
    lcd.fillScreen(TFT_RED);
    while (true) delay(1000);
  }

  app = new App(canvas, PlatformHooks{present, setBrightness, randomN});
  app->begin();
}

void loop() {
  int32_t x = 0, y = 0;
  bool touched = lcd.getTouch(&x, &y);
  app->update(touched, x, y, millis());
  delay(10);
}
