// main.cpp — roda um sketch (.ino) do projeto numa janela do computador.
// Qual sketch? Definido na hora de compilar (veja o README.md):
//   cmake -B build -DSKETCH=../etapas/etapa03_ola/etapa03_ola.ino
#include "arduino_shim.h"
#include SKETCH_FILE                  // o .ino escolhido entra aqui, como se fosse código C++

// A LovyanGFX cuida da janela; nós só chamamos setup() uma vez e loop() para sempre.
static int roda_sketch(bool* rodando) {
  setup();
  while (*rodando) loop();
  return 0;
}

int main(int, char**) {
  return lgfx::Panel_sdl::main(roda_sketch);
}
