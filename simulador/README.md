# Simulador do Visor de Vida MTG (Linux)

Roda os programas do projeto (os arquivos `.ino` da pasta `etapas/`) numa janela de 800×480 no seu computador,
igual à tela da placa. **O clique do mouse faz o papel do dedo** no touch.
O código das etapas é o mesmo que vai para a placa: só o arquivo `placa.h` de cada etapa escolhe entre "placa" e "simulador".

## 1. Instalar o que precisa (uma vez só)

No **Fedora**:

```bash
sudo dnf install gcc-c++ make cmake git sdl2-compat-devel
```

No Ubuntu/Debian/Mint:

```bash
sudo apt install build-essential cmake git libsdl2-dev
```

Depois, dentro desta pasta (`Projeto Arduíno/simulador`), baixe a biblioteca gráfica:

```bash
cd ~/Desktop/"Projeto Arduíno"/simulador
git clone --depth 1 https://github.com/lovyan03/LovyanGFX.git
```

## 2. Compilar e rodar uma etapa

```bash
cmake -B build -DSKETCH=../etapas/etapa03_ola/etapa03_ola.ino
cmake --build build -j
./build/simulador
```

- Abre uma janela com a tela do visor. O que o programa manda para o "Monitor Serial" aparece no terminal.
- Para fechar: feche a janela.
- Mudou o código do `.ino`? Rode de novo só os dois últimos comandos (`cmake --build ...` e `./build/simulador`).
- Para simular outra etapa, troque o caminho no `-DSKETCH=...` e rode os três comandos.

## Etapas disponíveis

| Etapa | `-DSKETCH=` |
|---|---|
| 3 — Olá | `../etapas/etapa03_ola/etapa03_ola.ino` |
| 4 — Touch | `../etapas/etapa04_touch/etapa04_touch.ino` |
| 5 — Um painel | `../etapas/etapa05_painel/etapa05_painel.ino` |
| 6 — Quatro painéis | `../etapas/etapa06_quatro_paineis/etapa06_quatro_paineis.ino` |
| 7 a 11 — Visor completo | `../etapas/etapa07a11_visor/etapa07a11_visor.ino` |

Os outros arquivos `.cpp` da pasta do sketch (ex.: `game.cpp`, `ui.cpp`) entram na compilação automaticamente, como na IDE do Arduino.

## Arquivos

| Arquivo | O que faz |
|---|---|
| `CMakeLists.txt` | Receita de compilação: junta a LovyanGFX, o SDL2 (janela) e o sketch escolhido. |
| `main.cpp` | Chama o `setup()` uma vez e o `loop()` para sempre, como a placa faz. |
| `arduino_shim.h` | Imita no PC as funções do Arduino que usamos (`Serial`, `delay`, `millis`, `random`). |

## Limitações

- O simulador mostra a tela e o toque; não simula Wi-Fi, cartão SD nem a velocidade real da placa.
- Cores e posições são as mesmas, mas sempre confira o resultado final na placa de verdade.
