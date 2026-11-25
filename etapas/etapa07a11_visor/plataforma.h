// plataforma.h — deixa o motor de desenho 8-bit (gfx8) compilar na placa e no simulador do PC.
#pragma once
#include <stdint.h>

#ifdef ARDUINO
  #include <Arduino.h>          // na placa: PROGMEM e pgm_read_byte já existem
#else
  #define PROGMEM
  #define pgm_read_byte(p) (*(const uint8_t *)(p))
#endif

// Única função que liga o motor de desenho à tela: pinta um retângulo em PIXELS REAIS
// (800x480), cor em RGB565. Quem implementa é o ui.cpp (pinta no sprite da LovyanGFX).
void portaPreencher(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t cor);
