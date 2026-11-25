# Estudo da API do Playgroup.gg para o Visor de Vida MTG

Estudo feito em 25/11/2026 a partir da especificação pública (`https://playgroup.gg/.well-known/openapi.yaml`)
e das páginas do tracker e do Playgroup Live. Alguns pontos a especificação **não explica**. Eles estão
marcados com **[testar]** e o script `testar_api.sh`, nesta pasta, confirma cada um com a sua chave.

## 1. Resumo em 30 segundos

| Pergunta | Resposta |
|---|---|
| Dá para **criar a partida pelo visor**? | **Não.** A API não tem endpoint para criar partida do contador de mesa. A partida nasce no celular (app ou playgroup.gg/tracker). |
| Dá para **puxar a partida do celular para o visor**? | **Provavelmente sim.** O visor lista as partidas do playgroup e pega a que ainda não terminou (`ended_at` vazio): vida inicial, jogadores e decks. **[testar]** se a partida aparece na API enquanto ainda está em andamento. |
| Dá para **mandar o que acontece no visor** (dano, turno, eliminação)? | **Sim, com condições.** `POST /games/{id}/events/batch`, desde que o dono da chave esteja na partida. Os nomes exatos dos eventos e a ordem dos assentos precisam ser testados. |
| Dá para **ver a vida ao vivo do celular** no visor? | **Não diretamente.** Não há endpoint de "estado ao vivo". Talvez dê lendo os eventos da partida a cada poucos segundos **[testar]**. |
| E o `POST /live_sessions`? | Cria uma sala do **Playgroup Live**, a mesa **virtual online** (cartas no navegador, voz). **Não serve** para jogo de papel na mesa. |
| O que dá para mostrar de bônus? | Arte do comandante no painel, ELO e taxa de vitória de cada deck, estatísticas do comandante, curva média de dano por turno e "elogios" (endorsements) no fim da partida. |

## 2. Como a API funciona

- **Endereço base:** `https://playgroup.gg/api/public/v1`

- **Chave:** gerada nas configurações da conta, enviada como `Authorization: Bearer <chave>`.
  - Sem chave: dados públicos de comandantes, usuários e decks.
  - Com chave: partidas, playgroups, membros e envio de eventos (só nos playgroups e partidas de que você participa).

- **User-Agent descritivo** (com um site ou e-mail de contato): com ele, **400 requisições por minuto** por IP; sem ele, só **10**.
  No visor: `VisorVidaMTG/1.0 (+https://github.com/ensosampaio/visor-vida-mtg)`.

- **Comandantes:** a mesma página/caminho só pode ser pedida **20 vezes a cada 10 minutos** por IP. Vale guardar em cache.

- **Limite estourado:** resposta `429` com o cabeçalho `Retry-After` (quantos segundos esperar).

- **Crédito obrigatório:** as estatísticas de comandante são licença CC BY 4.0. Toda tela que mostrar esses dados
  deve dizer "Dados: Playgroup.gg" (num aparelho sem link clicável, o texto basta junto com o endereço).

## 3. O que dá para puxar (e para que serve no visor)

| Endpoint | Precisa de chave? | O que devolve | Uso no visor |
|---|---|---|---|
| `GET /me` | sim | seu usuário (id, nome, partidas, vitórias, médias) | Saber quem é o dono do visor; testar a chave |
| `GET /users/{id}/playgroups` | sim | seus playgroups (id, nome, nº de partidas e membros, ligas) | Tela "escolha o playgroup" |
| `GET /playgroups/{id}/members` | sim (membro) | membros ativos (user_id, username, admin) | Nomes dos jogadores |
| `GET /playgroups/{id}/games` | sim (membro) | partidas (mais recentes; `page`, `limit` até 100; `include_events`) | **Achar a partida em andamento** |
| `GET /playgroups/{id}/games/{game_id}` | sim (membro) | uma partida | Atualizar a partida escolhida |
| `GET /decks/{id}` | opcional | deck: nome, comandante (e parceiro) com **imagem**, bracket, vitórias, ELO, cores | Arte e nome do comandante no painel |
| `GET /decks/{id}/elo_history` | opcional | ELO atual e histórico (global, do playgroup ou da liga) | Tela "antes da partida": ELO de cada deck |
| `GET /users/{id}` e `/users/{id}/decks` | opcional | estatísticas do jogador e decks dele | Ficha do jogador |
| `GET /commanders/by_name/{nome}` | não | comandante (busca aproximada): imagens, ELO global, ranking, estatísticas | Curiosidades sobre o comandante |
| `GET /commanders/{id}/turn_damage` | não | dano médio por turno (até o turno 30): normal, comandante, veneno, cura | Gráfico "como esse comandante costuma bater" |

**A partida (`Game`)** traz: `id`, `started_at`, `ended_at`, `life_amount` (vida inicial), `total_rounds`, `win_con`,
`playgroup_id` e as **participações**: para cada jogador, `user_id`, `user_name`, `deck_id`, `deck_name`, `winner`,
mulligans e as notas de diversão/sal (1 a 5). Com `include_events=true`, vem também a lista de eventos
(`kind`: `normal_damage`, `commander_damage`, `poison_tokens`, `healing`, `mill`, `life_loss`; com turno, quantidade, quem
causou e quem recebeu).

## 4. Iniciar a partida no celular e puxar para o visor

O app do Playgroup tem o **Device Sync**: vários aparelhos abrem a **mesma partida**. O visor não pode entrar no Device Sync
(não há API para isso), mas pode **ler** a partida que o celular criou.

**Fluxo proposto:**

1. **Uma vez só (configuração):** o visor cria uma rede Wi-Fi própria; no celular você abre a página dele, escolhe a rede da casa e
   cola a chave da API. O visor guarda tudo na memória interna (NVS), nunca no código nem no GitHub.
   Também escolhe o playgroup padrão (lista de `GET /users/{id}/playgroups`).
2. **No celular:** o anfitrião começa a partida no app do Playgroup, com os jogadores e decks, como já faz hoje.
3. **No visor:** menu → **PLAYGROUP** → o visor chama `GET /playgroups/{id}/games?limit=5` e mostra as partidas
   sem `ended_at` (em andamento). Você toca na partida certa.
4. O visor monta a mesa: vida inicial = `life_amount`, um painel por participação, com **nome do jogador** e **nome do deck**.
   Para cada `deck_id`, `GET /decks/{id}` traz o comandante e a imagem; o visor baixa a **arte do comandante** e usa de fundo
   no painel (em versão escurecida, para o número continuar legível).
5. **Assentos:** o visor mostra "quem senta onde?" e você arrasta/toca para colocar cada jogador num painel (P1 a P4, sentido horário).
   Isso é necessário porque a API não diz a ordem dos assentos.

**Incertezas [testar]:**

- Uma partida do tracker **em andamento** aparece em `/playgroups/{id}/games`? A página do tracker diz que as estatísticas ficam
  disponíveis "em tempo real", mas não diz se a partida sobe antes de terminar.

- Os eventos da partida (`include_events`) aparecem durante o jogo ou só no fim?

- A partida feita no app com jogadores **convidados** (sem conta) aparece com `user_id` vazio?

Se a partida só subir no fim, o plano B é: o visor joga sozinho e, **no fim**, mostra um resumo para lançar no app,
ou envia os eventos para a partida depois que ela aparecer.

## 5. Mandar dados do visor para o Playgroup

`POST /games/{game_id}/events/batch` importa uma **lista de eventos de uma vez** (tudo ou nada) numa partida
**terminada ou em andamento**. Regras:

- Quem manda (dono da chave) **precisa ser participante** daquela partida (senão `403`).

- Cada evento: `name` (tipo), `source_player_id` e `target_player_id` (o **"HUD index"** do jogador, de `"0"` a `"5"`),
  `id` (número do evento no visor), `time` (hora Unix), `turn`, `amount`, `commander_id`, `metadata`.

- Se um evento falha, nada é gravado; a resposta `422` diz qual (`failed_event_index`) e por quê.

**Como o histórico do visor viraria eventos (proposta, nomes a confirmar):**

| No visor | Evento enviado |
|---|---|
| `P2 LIFE -3` (quem causou? ver abaixo) | `Damage`, origem = quem atacou, alvo = P2, `amount` 3 |
| `P2 CMDR FROM P1 +5` | `Damage` com `commander_id` do comandante do P1, alvo P2, 5 |
| `P2 POISON +2` | evento de veneno (nome a descobrir) |
| `P3 TURN - ROUND 2` | `PassTurn`, origem = jogador que passou, `turn` |
| `P2 OUT` | `Kill`, origem = quem eliminou, alvo P2 |
| Fim da partida, tela "elogios" | `Endorsement` com `metadata.tag`: `good_sport`, `good_tempo`, `clear_communicator`, `would_play_again`, `deck_matched_bracket` (máx. 2 por par de jogadores) |

**Problemas a resolver:**

- **Quem causou o dano?** O visor hoje só sabe que P2 perdeu 3 de vida, não de quem. O Playgroup conta estatísticas por
  quem causou. Opção: ao tirar vida, o visor pergunta "de quem?" (como o app faz com a "seta de ataque"), ou manda sem origem
  se a API aceitar **[testar]**.

- **Nomes dos eventos:** a especificação só cita `Damage`, `Kill`, `PassTurn` e `Endorsement` ("etc."). Veneno, cura e
  perda de vida precisam ser confirmados (testando com uma partida de teste ou perguntando no Discord do Playgroup).

- **HUD index:** a especificação não diz como os números 0–5 se ligam aos jogadores. Provavelmente é a ordem das
  participações **[testar]**.

- **Partida duplicada:** se o celular também estiver registrando, os dois enviariam o mesmo dano. O visor deve ser **o único
  contador** daquela partida (o celular só cria e encerra).

- **Quando mandar:** em lote, a cada PASS TURN (o botão novo é o gatilho perfeito) e no fim. Nunca a cada toque.
  Sem Wi-Fi, o visor guarda os eventos e manda depois: **a partida nunca pode travar por causa da internet**.

## 6. O que não dá (ou não serve)

- **Criar partida do contador de mesa pela API:** não existe. Só o app/tracker cria.

- **`POST /live_sessions`:** cria uma sala do **Playgroup Live** (mesa virtual online, com cartas no navegador e voz).
  Devolve só um link. Não tem relação com partida de papel.

- **Ler a vida ao vivo do celular:** não há endpoint de estado ao vivo. No máximo ler os eventos de tempos em tempos.

- **Encerrar a partida / marcar vencedor:** não há endpoint. O fim e o vencedor continuam no app.

- **Ranking do playgroup pronto:** não há endpoint de leaderboard; dá para montar com o ELO de cada deck (`elo_history` com `playgroup_id`).

## 7. Outras funcionalidades que a API permite

1. **Arte do comandante no painel** (`Deck.commander.art_crop_url`): a placa baixa a imagem JPEG e desenha
   (a LovyanGFX decodifica JPEG). É o que mais muda a cara do visor.
2. **Tela "antes da partida":** para cada deck, ELO no playgroup, taxa de vitória, bracket e "poder estimado".
3. **Ficha do comandante:** ELO global, posição no ranking, taxa de vitória, turno médio de vitória.
4. **Gráfico de dano por turno** (`turn_damage`): "esse comandante costuma bater X no turno 6". Curiosidade para a mesa.
5. **Elogios no fim da partida:** cada jogador toca em quem jogou bem (`Endorsement`). Divertido e simples de mandar.
6. **Conferir o bracket:** a partida/deck tem bracket 1–5; o visor pode avisar "deck bracket 4 numa mesa bracket 2".

## 8. Viabilidade na placa (ESP32-S3)

- **HTTPS:** o ESP32-S3 faz HTTPS com `WiFiClientSecure` (usar o certificado raiz do site ou o pacote de certificados do ESP32).

- **JSON:** com a biblioteca **ArduinoJson** e um "filtro" que só guarda os campos usados. Uma página de 5 partidas sem eventos
  cabe folgada na PSRAM (8 MB).

- **Imagens:** a arte do comandante é um JPEG pequeno; baixar 4 e desenhar reduzido no painel (390×180) é viável.
  Guardar em cache na memória flash para não baixar de novo.

- **Wi-Fi e chave:** configuração por um portal no celular (biblioteca tipo WiFiManager), guardado na NVS.

- **Cuidado com a chave:** ela dá acesso à sua conta. Nunca no código, nunca no GitHub, nunca no histórico do visor.
  Botão "apagar chave" no menu de configuração.

## 9. Plano revisado da etapa 13

| Etapa | O quê | Precisa da placa? |
|---|---|---|
| 13.1 | Gerar a chave e rodar `testar_api.sh` duas vezes: sem partida e **com uma partida em andamento no celular**. Me mandar a pasta `respostas/` (sem a chave). | Não |
| 13.2 | Simulador: tela PLAYGROUP com os dados de teste (arquivos JSON), escolher a partida e montar a mesa com nomes e decks | Não |
| 13.3 | Placa: Wi-Fi + portal de configuração + chave na NVS | Sim |
| 13.4 | Placa: puxar playgroups, partidas e decks pela internet | Sim |
| 13.5 | Arte do comandante no painel (download, cache, desenho) | Sim (dá para adiantar no simulador) |
| 13.6 | Enviar eventos em lote (a cada PASS TURN e no fim), com fila offline. Antes: descobrir nomes de evento e HUD index numa partida de teste | Sim |
| 13.7 | Extras: tela "antes da partida" (ELO), elogios no fim, ficha do comandante | Sim |

## 10. Como testar (etapa 13.1)

1. Em playgroup.gg → configurações da conta → gere uma **chave de API**.
2. Na pasta do projeto, rode (no fish ou no bash, tanto faz):

   ```bash
   bash docs/playgroup/testar_api.sh
   ```

   O script pede a chave (não aparece na tela e não fica salva), faz **só leituras** (nenhum envio) e grava as respostas em
   `docs/playgroup/respostas/` (essa pasta não vai para o GitHub). No fim, mostra um resumo das partidas.
3. Rode de novo **com uma partida começada no app do celular** e veja se ela aparece como "EM ANDAMENTO" e com eventos.
4. Me conte o resumo (ou mande os arquivos JSON, que não têm a chave) e eu fecho as incertezas marcadas com **[testar]**.

## Fontes

- Especificação OpenAPI: https://playgroup.gg/.well-known/openapi.yaml

- Documentação (Swagger): https://playgroup.gg/api-docs/index.html

- Tracker (contador de mesa): https://playgroup.gg/tracker

- Playgroup Live (mesa virtual online): https://playgroup.gg/playgroup-live

- Perguntas frequentes (ELO, Device Sync): https://playgroup.gg/faqs
