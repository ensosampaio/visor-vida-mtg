// placa.h — escolhe qual "tela" o programa vai usar.
//
// O mesmo código roda em dois lugares:
//   - na placa de verdade (Sunton ESP32-8048S043C, tela 800x480);
//   - no simulador do computador (uma janela de 800x480; o mouse faz o papel do dedo).
// Quem decide é o símbolo LGFX_SDL, que só o simulador define.
#pragma once

#if defined(LGFX_SDL)
  // ---------- simulador no PC ----------
  #include <LGFX_AUTODETECT.hpp>
  static LGFX lcd(800, 480);      // janela do mesmo tamanho da tela da placa
#else
  // ---------- placa de verdade ----------
  // A LovyanGFX já traz a configuração desta placa (pinos da tela, do touch e do brilho).
  #include <lgfx_user/LGFX_ESP32S3_RGB_ESP32-8048S043.h>
  static LGFX lcd;
#endif
