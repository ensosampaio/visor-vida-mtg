// ============================================================================
//  ETAPA 3 — Primeiro programa: acender a tela e escrever um texto
//  Visor de Vida MTG
//
//  Todo programa de Arduino tem duas funções obrigatórias:
//    setup()  -> roda UMA vez, quando a placa liga (ou depois do botão RESET);
//    loop()   -> roda de novo e de novo, para sempre, depois do setup().
//
//  Na IDE do Arduino (menu Ferramentas): Placa "ESP32S3 Dev Module",
//  Flash Size "16MB", PSRAM "OPI PSRAM".
// ============================================================================

#include <LovyanGFX.hpp>   // biblioteca gráfica (desenho, textos, touch)
#include "placa.h"         // cria o objeto "lcd": a placa de verdade ou o simulador do PC

// Cores no formato 0xRRGGBB (vermelho, verde, azul), as mesmas do visor do vídeo.
const uint32_t COR_FUNDO  = 0x061055;   // azul-escuro
const uint32_t COR_PAINEL = 0x1636D8;   // azul
const uint32_t COR_LIMAO  = 0xA6FF3C;   // verde-limão
const uint32_t COR_BRANCO = 0xF2F7FF;

uint32_t segundos = 0;          // quantos segundos o programa está rodando
uint32_t ultimoSegundo = 0;     // em que momento (em ms) contamos o último segundo

void setup() {
  // O "Monitor Serial" da IDE mostra mensagens enviadas pela placa (ótimo para depurar).
  Serial.begin(115200);
  Serial.println("Etapa 3: ligando a tela...");

  lcd.init();                   // liga a tela e o touch
  lcd.setBrightness(200);       // brilho de 0 (apagada) a 255 (máximo)
  lcd.fillScreen(COR_FUNDO);    // pinta a tela inteira

  int meioX = lcd.width() / 2;  // 800 / 2 = 400

  // Um painel com borda, no estilo do visor do vídeo
  lcd.fillRoundRect(100, 60, 600, 360, 16, COR_PAINEL);   // x, y, largura, altura, raio do canto, cor
  lcd.drawRoundRect(100, 60, 600, 360, 16, COR_LIMAO);
  lcd.drawRoundRect(101, 61, 598, 358, 15, COR_LIMAO);

  // "middle_center": o ponto (x, y) passado ao drawString é o CENTRO do texto
  lcd.setTextDatum(middle_center);

  // As fontes da biblioteca não têm acentos, por isso "Ola" e não "Olá".
  lcd.setFont(&fonts::FreeSansBold24pt7b);
  lcd.setTextColor(COR_BRANCO);
  lcd.drawString("Ola, Enzo!", meioX, 120);

  lcd.setFont(&fonts::FreeSansBold12pt7b);
  lcd.setTextColor(COR_LIMAO);
  lcd.drawString("Visor de Vida MTG - etapa 3", meioX, 175);

  // Um "40" em estilo display de 7 segmentos: um gostinho do contador
  lcd.setFont(&fonts::Font7);
  lcd.setTextSize(2);           // dobra o tamanho da fonte
  lcd.setTextColor(COR_BRANCO);
  lcd.drawString("40", meioX, 290);
  lcd.setTextSize(1);           // volta ao tamanho normal

  // Mostra o tamanho da tela (confirma que a configuração está certa)
  char texto[40];
  snprintf(texto, sizeof texto, "Tela: %d x %d pixels", (int)lcd.width(), (int)lcd.height());
  lcd.setFont(&fonts::FreeSans9pt7b);
  lcd.setTextColor(COR_BRANCO);
  lcd.drawString(texto, meioX, 390);

  Serial.println("Pronto! Se a tela mostrou o texto, a etapa 3 funcionou.");
}

void loop() {
  // millis() = quantos milissegundos se passaram desde que a placa ligou.
  // A cada 1000 ms (1 segundo) atualizamos o contador no canto da tela.
  if (millis() - ultimoSegundo >= 1000) {
    ultimoSegundo = millis();
    segundos++;

    char texto[24];
    snprintf(texto, sizeof texto, "ligada ha %lu s", (unsigned long)segundos);

    // Apaga o texto anterior pintando um retângulo por cima, e escreve o novo.
    lcd.fillRect(560, 440, 240, 40, COR_FUNDO);
    lcd.setTextDatum(middle_right);
    lcd.setFont(&fonts::FreeSans9pt7b);
    lcd.setTextColor(COR_LIMAO);
    lcd.drawString(texto, 785, 460);

    Serial.println(texto);
  }
  delay(10);   // pequena pausa: o loop não precisa rodar milhares de vezes por segundo
}
