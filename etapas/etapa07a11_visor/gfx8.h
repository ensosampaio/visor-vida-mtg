// gfx8.h — motor de desenho do tema 8-bit (paleta do NES)
//
// Tudo e desenhado em "pixels 8-bit": 1 pixel logico = 2x2 pixels reais.
// Tela logica: 400 x 240  (tela real 800 x 480).
#pragma once
#include "plataforma.h"

#define ESCALA 2
#define TELA_W 400
#define TELA_H 240

#define RGB565(r, g, b) (uint16_t)((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | ((b) >> 3))

// ---- Paleta NES (subconjunto usado) ----
#define C_PRETO     RGB565(0x00, 0x00, 0x00)
#define C_BRANCO    RGB565(0xFC, 0xFC, 0xFC)
#define C_CINZA     RGB565(0x78, 0x78, 0x78)
#define C_CINZA_C   RGB565(0xBC, 0xBC, 0xBC)
#define C_OURO      RGB565(0xF8, 0xB8, 0x00)
#define C_OURO_E    RGB565(0xAC, 0x7C, 0x00)
#define C_MARROM    RGB565(0x50, 0x30, 0x00)
#define C_CREME     RGB565(0xFC, 0xE0, 0xA8)
#define C_CIANO     RGB565(0x3C, 0xBC, 0xFC)
#define C_TURQ      RGB565(0x00, 0xE8, 0xD8)
#define C_TEAL      RGB565(0x00, 0x88, 0x88)
#define C_TEAL_E    RGB565(0x00, 0x40, 0x58)
#define C_LIMA      RGB565(0xB8, 0xF8, 0x18)
#define C_VERDE     RGB565(0x00, 0xB8, 0x00)
#define C_VERDE_M   RGB565(0x00, 0xA8, 0x00)
#define C_VERDE_E2  RGB565(0x00, 0x78, 0x00)
#define C_VERDE_E   RGB565(0x00, 0x58, 0x00)
#define C_VERMELHO  RGB565(0xF8, 0x38, 0x00)
#define C_VERM_M    RGB565(0xA8, 0x10, 0x00)
#define C_VERM_E    RGB565(0x88, 0x14, 0x00)
#define C_AZUL      RGB565(0x00, 0x78, 0xF8)
#define C_AZUL_M    RGB565(0x00, 0x58, 0xF8)
#define C_AZUL_V    RGB565(0x00, 0x00, 0xFC)
#define C_AZUL_E    RGB565(0x00, 0x00, 0xBC)
#define C_MAGENTA   RGB565(0xD8, 0x00, 0xCC)
#define C_ANIL      RGB565(0x44, 0x28, 0xBC)
#define C_VIOLETA   RGB565(0x68, 0x44, 0xFC)
#define C_LARANJA   RGB565(0xFC, 0xA0, 0x44)

#define SEM_SOMBRA  0x0001   // cor "magica" (quase preto) = nao desenhar sombra
#define SEM_TEXTURA 0x0001

// Esquema de cores de um componente
struct Estilo {
  uint16_t borda;
  uint16_t fundo;
  uint16_t textura;   // pontinhos a cada 8 px (SEM_TEXTURA = nenhum)
};

// Estilos prontos (equivalentes as pecas PNG da versao 8-bit)
extern const Estilo EST_JOGADOR[4];
extern const Estilo EST_ELIMINADO;
extern const Estilo EST_BTN_NORMAL, EST_BTN_AZUL, EST_BTN_ROXO, EST_BTN_VERDE, EST_BTN_VERMELHO,
                    EST_BTN_SELECIONADO, EST_BTN_DESATIVADO, EST_BTN_OURO, EST_BTN_CIANO;
extern const Estilo EST_TOPO_NORMAL, EST_TOPO_ATIVO, EST_TOPO_DESATIVADO;
extern const Estilo EST_ICO_COMANDANTE, EST_ICO_ENERGIA, EST_ICO_VENENO;
extern const uint16_t COR_JOGADOR[4];

// ---- Transformacao: permite desenhar um painel "de cabeca para baixo" (180 graus) ----
void gOrigem(int16_t ox, int16_t oy, int16_t w, int16_t h, bool girar180);
void gOrigemReset();
void gEscurecer(bool on);   // desenha tudo com metade do brilho (fundo de caixa de dialogo)

// ---- Primitivas (coordenadas logicas) ----
void gRet(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t c);
void gChanfrado(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t chanfro, uint16_t c);
void gContorno(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t chanfro, uint16_t c);

// ---- Texto (fonte 5x7 negrito) ----
enum Alinha { ESQ = 0, CENTRO = 1, DIR = 2 };
int16_t gLarguraTexto(const char *s, uint8_t k);
int16_t gTexto(const char *s, int16_t x, int16_t y, uint8_t k, uint16_t cor,
               uint16_t sombra = C_PRETO, Alinha al = ESQ);
int16_t gNumero(int16_t n, int16_t x, int16_t y, uint8_t k, uint16_t cor,
                uint16_t sombra = C_PRETO, Alinha al = ESQ, bool sinal = false);

int16_t gNumeroGrande(int16_t n, int16_t x, int16_t y, uint8_t k, uint16_t cor,
                      Alinha al = CENTRO, bool sinal = false);   // algarismos 8x10

// ---- Componentes ----
// Caixa no estilo NES: contorno preto, borda colorida, linha preta, fundo + textura
void gCaixa(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t chanfro, const Estilo &e,
            uint8_t espessura = 2, bool textura = true, bool pressionado = false);
void gBotao(int16_t x, int16_t y, int16_t w, int16_t h, const Estilo &e, const char *rotulo,
            uint8_t k = 1, uint16_t corTexto = C_BRANCO, bool pressionado = false);
void gBotaoTopo(int16_t x, int16_t y, const char *rotulo, uint8_t estado); // 0 normal 1 ativo 2 desativado
void gEtiqueta(int16_t x, int16_t y, uint16_t cor, const char *rotulo);       // "P1".. 27x13
void gSinal(char tipo, int16_t x, int16_t y, bool apagado);                   // '+' ou '-' 16x16
void gBarra(int16_t x, int16_t y, int16_t w, uint16_t c);                     // barra-corrente
void gFundoEstrelado();
void gXadrez(int16_t x, int16_t y, int16_t w, int16_t h);    // escurece: quadriculado preto

enum Icone { ICO_COROA = 0, ICO_RAIO = 1, ICO_GOTA = 2 };
void gIcone(Icone ic, bool grande, int16_t x, int16_t y);  // grande 20x12, pequeno 9x9
void gBotaoIcone(int16_t x, int16_t y, Icone ic);           // 31x17 com icone
