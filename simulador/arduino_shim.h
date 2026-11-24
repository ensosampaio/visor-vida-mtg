// arduino_shim.h — imita, no PC, as poucas funções do Arduino que os sketches usam
// (Serial, delay, millis). Assim o MESMO arquivo .ino roda na placa e no simulador.
#pragma once
#include <LovyanGFX.hpp>
#include <cstdio>
#include <cstdarg>
#include <cstdint>
#include <iostream>
#include <cstdlib>
#include <ctime>

using lgfx::delay;
using lgfx::millis;

// random(n): número aleatório de 0 a n-1 (na placa, o Arduino já tem essa função)
inline long random(long n) {
  static bool semeado = false;
  if (!semeado) { std::srand((unsigned)std::time(nullptr)); semeado = true; }
  return n > 0 ? std::rand() % n : 0;
}

// No PC, o "Monitor Serial" é o próprio terminal.
struct SerialDoPC {
  void begin(unsigned long) {}
  template <typename T> void print(const T& v)   { std::cout << v << std::flush; }
  template <typename T> void println(const T& v) { std::cout << v << std::endl; }
  void println() { std::cout << std::endl; }
  void printf(const char* fmt, ...) {
    va_list ap; va_start(ap, fmt); std::vprintf(fmt, ap); va_end(ap); std::fflush(stdout);
  }
};
static SerialDoPC Serial;
