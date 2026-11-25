#!/usr/bin/env bash
# Testa a API pública do Playgroup.gg com a SUA chave (etapa 13.1 do roteiro).
#
# Uso (funciona chamado do fish ou do bash):
#   bash docs/playgroup/testar_api.sh
#   bash docs/playgroup/testar_api.sh 123      # força o playgroup de id 123
#
# - Faz SÓ leituras (GET). Não envia nada para a sua conta.
# - A chave é pedida na hora, não aparece na tela e não é gravada em lugar nenhum.
# - As respostas ficam em docs/playgroup/respostas/ (fora do GitHub: têm nomes e dados da sua conta).
# Precisa de: curl e python3 (já vêm no Fedora).
set -euo pipefail

BASE="${PLAYGROUP_BASE:-https://playgroup.gg/api/public/v1}"
UA="VisorVidaMTG/0.1 (+https://github.com/ensosampaio/visor-vida-mtg)"
AQUI="$(cd "$(dirname "$0")" && pwd)"
OUT="$AQUI/respostas"
mkdir -p "$OUT"

if [ -z "${PLAYGROUP_KEY:-}" ]; then
  read -r -s -p "Cole a chave da API do Playgroup (não aparece na tela): " PLAYGROUP_KEY
  echo
fi
[ -n "$PLAYGROUP_KEY" ] || { echo "Sem chave, nada a fazer."; exit 1; }

# get NOME CAMINHO  -> grava respostas/NOME.json e mostra o código HTTP
get() {
  local nome="$1" caminho="$2" corpo codigo
  corpo="$(mktemp)"
  codigo="$(curl -sS -o "$corpo" -w '%{http_code}' \
    -H "Authorization: Bearer $PLAYGROUP_KEY" -H "User-Agent: $UA" -H "Accept: application/json" \
    "$BASE$caminho")" || codigo="erro"
  if python3 -m json.tool "$corpo" > "$OUT/$nome.json" 2>/dev/null; then :; else cp "$corpo" "$OUT/$nome.json"; fi
  rm -f "$corpo"
  printf '  %-45s HTTP %s  -> respostas/%s.json\n' "GET $caminho" "$codigo" "$nome"
  [ "$codigo" = "200" ]
}

# py ARQUIVO EXPRESSAO  -> roda um trecho de python com o JSON carregado em 'd'
py() { python3 -c "import json,sys; d=json.load(open('$1')); $2"; }

echo "1) Quem sou eu"
get me "/me" || { echo "A chave não funcionou (veja respostas/me.json)."; exit 1; }
USER_ID="$(py "$OUT/me.json" "print(d['id'])")"
echo "   usuário: $(py "$OUT/me.json" "print(d.get('username'))") (id $USER_ID)"

echo "2) Meus playgroups"
get playgroups "/users/$USER_ID/playgroups"
py "$OUT/playgroups.json" "
for p in d: print('   - id', p['id'], '|', p.get('name'), '|', p.get('game_count'), 'partidas |', p.get('member_count'), 'membros')"
PG_ID="${1:-$(py "$OUT/playgroups.json" "print(d[0]['id'] if d else '')")}"
[ -n "$PG_ID" ] || { echo "Você não está em nenhum playgroup."; exit 0; }
echo "   usando o playgroup $PG_ID (para outro: bash testar_api.sh <id>)"

echo "3) Membros"
get membros "/playgroups/$PG_ID/members" || true

echo "4) Últimas 5 partidas, com eventos"
get partidas "/playgroups/$PG_ID/games?limit=5&include_events=true"

echo
echo "=== RESUMO DAS PARTIDAS ==="
py "$OUT/partidas.json" "
for g in d:
    andamento = g.get('ended_at') in (None, '')
    print()
    print('Partida', g['id'], '| EM ANDAMENTO' if andamento else '| terminada em ' + str(g.get('ended_at')))
    print('  início:', g.get('started_at'), '| vida inicial:', g.get('life_amount'), '| rodadas:', g.get('total_rounds'), '| vitória por:', g.get('win_con'))
    for i, p in enumerate(g.get('participations') or []):
        print('  [%d] %-20s deck: %-30s deck_id=%s user_id=%s%s' % (i, p.get('user_name'), p.get('deck_name'), p.get('deck_id'), p.get('user_id'), '  VENCEDOR' if p.get('winner') else ''))
    ev = g.get('events') or []
    tipos = {}
    for e in ev: tipos[e.get('kind')] = tipos.get(e.get('kind'), 0) + 1
    print('  eventos:', len(ev), tipos)
"

echo
echo "5) Deck do primeiro jogador da partida mais recente (comandante e imagem)"
DECK_ID="$(py "$OUT/partidas.json" "
ps = (d[0].get('participations') or []) if d else []
ids = [p.get('deck_id') for p in ps if p.get('deck_id')]
print(ids[0] if ids else '')")"
if [ -n "$DECK_ID" ]; then
  get deck "/decks/$DECK_ID" || true
  py "$OUT/deck.json" "
c = d.get('commander') or {}
print('   deck:', d.get('name'), '| comandante:', c.get('name'), '| bracket:', d.get('bracket'))
print('   arte:', c.get('art_crop_url'))" || true
  get elo "/decks/$DECK_ID/elo_history?playgroup_id=$PG_ID" || true
else
  echo "   (nenhum deck_id na partida mais recente)"
fi

echo
echo "Pronto. Os arquivos estão em $OUT"
echo "Rode de novo com uma partida COMEÇADA no app do celular e veja se ela aparece como EM ANDAMENTO."
