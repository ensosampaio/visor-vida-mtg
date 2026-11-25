# Etapas 7 a 11 — O visor completo

Aqui as etapas 4 a 6 viram o programa inteiro do contador, organizado em arquivos.
Tudo foi testado no simulador e compila para a placa; **falta rodar na placa de verdade**.

O visual é o **tema 8-bit** (paleta do videogame NES): fontes, ícones, molduras e o fundo de estrelas
são desenhados só com retângulos pelo motor `gfx8`, sem nenhuma imagem. Os prints abaixo são do próprio código.

| Tela | |
|---|---|
| Menu e configuração (etapa 7) | ![](07_menu.png) ![](07_configuracao.png) |
| Jogo com turno, sorteio do FIRST e confirmação do RESET (etapa 8) | ![](08_jogo.png) ![](08_first.png) ![](08_confirmar.png) |
| Turno pulando o jogador eliminado | ![](08_turno_pulando.png) |
| Veneno (etapa 9) | ![](09_veneno.png) |
| Dano de comandante (etapa 10) | ![](10_comandante.png) ![](10_jogo_final.png) |
| Histórico (etapa 11) | ![](11_historico.png) |

## Como o código está dividido

| Arquivo | Papel |
|---|---|
| `etapa07a11_visor.ino` | Liga a tela, cria o sprite e repassa cada toque para a interface. Bem curto. |
| `game.h` / `game.cpp` | **As regras** (classe `mtg::Game`): vida, veneno, energia, dano de comandante, eliminação, UNDO e histórico. Não sabe nada de tela. |
| `ui.h` / `ui.cpp` | **A interface** (classe `App`): desenha cada tela e decide o que cada toque faz. |
| `gfx8.h` / `gfx8.cpp` | **O motor de desenho 8-bit**: paleta, cantos em escadinha, texto, números grandes, ícones, botões e painéis. |
| `fonte8.h` / `fonte_grande.h` | As duas fontes em pixel art: letras 5x7 em negrito e algarismos 8x10 para a vida. |
| `plataforma.h` | Deixa o motor compilar na placa e no PC. |
| `placa.h` | Escolhe entre a placa de verdade e o simulador do PC. |

Separar regras e interface é o mesmo princípio de separar domínio e controller num back-end: dá para mudar a
tela sem mexer nas regras, e testar as regras sem tela. Foi o que aconteceu na troca para o visual 8-bit:
`game.h`/`game.cpp` não mudaram uma linha.

## Como o desenho 8-bit funciona

- **Pixels 8-bit:** as telas são montadas numa grade de 400x240; cada "pixel 8-bit" vira um quadradinho de 2x2
  na tela de 800x480. Por isso as posições em `ui.cpp` são a metade das medidas da tela (o painel de 390x180 vira 195x90).
  Os toques continuam em pixels reais: `addHitL()` faz a conversão.
- **Uma função só fala com a tela:** o motor só sabe pintar retângulos (`portaPreencher`, em `ui.cpp`), que pinta no
  sprite da LovyanGFX. Trocar de biblioteca gráfica é mexer nessa função.
- **Painéis de cabeça para baixo:** `gOrigem(x, y, w, h, true)` faz o motor desenhar o painel girado 180°. O painel é
  sempre desenhado como se fosse o jogador "de baixo".
- **Cores:** a paleta (constantes `C_...` em `gfx8.h`) usa só cores que existiam no NES. Cada jogador tem um `Estilo`
  (borda, fundo e a cor dos pontinhos de textura) em `EST_JOGADOR`.

## Onde está cada etapa no código

### Etapa 7 — Menu inicial e configuração do Commander
- `App::renderMenu()`: botões COMMANDER, STANDARD, RESUME GAME (volta para a partida em andamento), TURNS (liga/desliga o turno),
  CLEAR LOG (apaga o histórico, com confirmação; a partida continua) e LIGHT (brilho).
- `App::renderSetup()`: vida inicial (20/30/40) e número de jogadores (1 a 4).
- `App::layoutPanels()`: posição dos painéis para 1, 2, 3 ou 4 jogadores (os de cima, girados).
- Ideia-chave: **telas como estados** (`enum class Screen`). O `render()` desenha a tela atual; um toque pode mudar de tela.

### Etapa 8 — Barra superior: LOG, LOCK, UNDO, FIRST, RESET, MENU
- `App::topBar()` desenha os botões; `App::perform()` diz o que cada um faz.
- **LOCK** trava os painéis contra toques acidentais. **FIRST** sorteia quem começa (o painel pisca e mostra "GOES FIRST!").
- **RESET** e **MENU** pedem confirmação (`App::renderConfirm()`).
- **UNDO** (`Game::undo()`): antes de cada ação, o jogo guarda uma "foto" do estado (`pushUndo()`); desfazer é voltar para a última foto.

### Turno (melhoria pedida em 25/11/2026)
- O painel de quem está jogando fica **mais claro e com a borda mais grossa** (`EST_TURNO` em `ui.cpp`), com "YOUR TURN" no alto.
- **PASS TURN**: dois botões na faixa do meio, o da direita girado para quem senta em cima (`App::passButton()`).
  O primeiro toque, se ninguém começou ainda, começa pelo P1.
- **FIRST** sorteia quem começa e já inicia a rodada 1 (`Game::startTurns()`).
- **Segurar o meio de um painel** (0,7 s) corrige à mão: "agora é a vez deste jogador" (`Game::setTurn()`, toque longo em `App::update()`).
- **Sentido horário:** os jogadores são numerados em volta da mesa — P1 em cima à esquerda, P2 em cima à direita,
  P3 embaixo à direita, P4 embaixo à esquerda (com 3: P1 em cima, P2 embaixo à direita, P3 embaixo à esquerda).
  Assim o turno passa P1 → P2 → P3 → P4 (`App::layoutPanels()`).
- `Game::passTurn()` pula eliminados; a **rodada** sobe quando o turno volta para quem começou (mostrada no meio da tela).
- Tudo entra no histórico e pode ser desfeito com UNDO, porque o turno faz parte do `State` (a "foto" do UNDO).
- No menu, **TURNS: ON/OFF** esconde tudo isso para quem não quer usar.
- Para abrir espaço para os botões, os painéis ficaram um pouco mais baixos (194 → 180 px).

### Etapa 9 — Veneno e energia
- `App::renderCounters()`: uma linha por jogador com −, valor e +. A mesma tela serve para veneno (gota) e energia (raio).
- 10 de veneno elimina (`POISON_LETHAL` em `game.h`).

### Etapa 10 — Dano de comandante
- `App::renderCmdr()`: escolhe o alvo (TARGET) e de qual comandante veio o dano (FROM). Mostra quanto já levou e avisa "LETHAL!".
- `Game::applyCommander()`: soma o dano no comandante **e** tira a mesma quantia de vida.
- 21 de um mesmo comandante elimina (`CMDR_LETHAL`). O painel do eliminado fica escuro, com faixa vermelha e o motivo.

### Etapa 11 — Histórico
- `App::renderLog()`: 10 linhas por página, a mais recente primeiro, com NEWER/OLDER. Cada tipo de evento tem uma cor.
- `Game::change()` + `Game::flush()`: toques seguidos no mesmo contador, em até 3 s, viram **uma** linha (ex.: "P2 LIFE -3 = 37").
- O histórico guarda até 200 linhas num buffer circular (`log_` em `game.h`): ocupa sempre o mesmo espaço (~12 KB) e,
  quando enche, as linhas mais antigas são sobrescritas. Por isso ele nunca "pesa" mais com o tempo.
- **CLEAR LOG** (menu) chama `Game::clearLog()`: apaga o histórico e zera o UNDO (as "fotos" do UNDO apontam para
  linhas do histórico). Vida, contadores e turno ficam como estão.

## Testar

- Simulador: `cmake -B build -DSKETCH=../etapas/etapa07a11_visor/etapa07a11_visor.ino`, `cmake --build build -j`, `./build/simulador`
- Placa: abra `etapa07a11_visor.ino` na IDE (os outros arquivos da pasta abrem como abas) e carregue.

## O que ainda falta (etapa 12, precisa da placa)
- Salvar a partida na memória interna para não perder se a bateria acabar.
- Bateria e case.
