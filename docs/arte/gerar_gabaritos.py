# Gera os gabaritos (PNG 800x480) com as medidas reais das telas do visor.
# Uso: python3 docs/arte/gerar_gabaritos.py   (requer: pip install pillow)
from PIL import Image, ImageDraw, ImageFont
import os

AQUI = os.path.dirname(os.path.abspath(__file__))
F = "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"
FB = "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf"
def fonte(t, b=False):
    try: return ImageFont.truetype(FB if b else F, t)
    except OSError: return ImageFont.load_default()

FUNDO, LINHA, TXT, ZONA_M, ZONA_P, DESTAQUE = (24, 26, 38), (120, 200, 255), (235, 238, 245), (255, 90, 90), (110, 230, 110), (255, 210, 80)

def caixa(d, x, y, w, h, rotulo, cor=LINHA, medida=True, f=12):
    d.rectangle([x, y, x + w - 1, y + h - 1], outline=cor, width=2)
    txt = rotulo + (f"\n{w}×{h}  (x={x}, y={y})" if medida else "")
    d.multiline_text((x + w / 2, y + h / 2), txt, fill=TXT, font=fonte(f), anchor="mm", align="center")

def base(titulo):
    im = Image.new("RGB", (800, 480), FUNDO)
    d = ImageDraw.Draw(im)
    for gx in range(0, 800, 50): d.line([gx, 0, gx, 479], fill=(36, 39, 55))
    for gy in range(0, 480, 50): d.line([0, gy, 799, gy], fill=(36, 39, 55))
    d.text((796, 476), titulo, fill=(150, 155, 175), font=fonte(11), anchor="rd")
    return im, d

# 1) Menu inicial
im, d = base("gabarito: menu inicial (800×480) — grade de 50 px")
caixa(d, 4, 4, 792, 472, "", medida=False)
d.text((400, 40), "TÍTULO  (y=20 a 62)", fill=DESTAQUE, font=fonte(16, True), anchor="mm")
d.text((400, 78), "subtítulo (y=72)", fill=TXT, font=fonte(11), anchor="mm")
caixa(d, 150, 104, 500, 124, "Botão COMMANDER")
caixa(d, 150, 248, 500, 124, "Botão STANDARD")
caixa(d, 24, 400, 200, 52, "RESUME GAME", f=10)
caixa(d, 300, 400, 200, 52, "TURNS: ON / OFF", f=10)
caixa(d, 576, 400, 200, 52, "LIGHT (brilho)", f=10)
im.save(os.path.join(AQUI, "gabarito_menu.png"))

# 2) Tela de jogo com 4 jogadores
im, d = base("gabarito: jogo, 4 jogadores (800×480) — botões do topo: 82×34, espaço de 6 px, y=6")
d.text((14, 24), "título (x=14, y=24)", fill=DESTAQUE, font=fonte(12, True), anchor="lm")
nomes = ["LOG", "LOCK", "UNDO", "FIRST", "RESET", "MENU"]
for i, n in enumerate(nomes):
    x = 272 + i * 88
    d.rectangle([x, 6, x + 81, 39], outline=LINHA, width=2)
    d.text((x + 41, 23), n, fill=TXT, font=fonte(11), anchor="mm")
for j, (x, y, rot) in enumerate([(6, 48, True), (404, 48, True), (6, 294, False), (404, 294, False)]):
    caixa(d, x, y, 390, 180, f"P{j+1}" + ("  (GIRADO 180°)" if rot else ""), f=12)
for i, n in enumerate(["comandante", "energia", "veneno"]):
    x = 298 + i * 70
    d.rectangle([x, 252, x + 61, 285], outline=DESTAQUE, width=2)
    d.text((x + 31, 269), n, fill=TXT, font=fonte(8), anchor="mm")
d.text((400, 244), "rodada (y=232)  ·  3 ícones 62×34, y=252", fill=DESTAQUE, font=fonte(9), anchor="mb")
caixa(d, 8, 232, 200, 56, "PASS TURN", f=10)
caixa(d, 592, 232, 200, 56, "PASS TURN (GIRADO 180°)", f=9)
im.save(os.path.join(AQUI, "gabarito_jogo_4_jogadores.png"))

# 3) Um painel de jogador (390×180), ampliado 2x
W, H = 390, 180
im = Image.new("RGB", (W * 2 + 20, H * 2 + 60), FUNDO)
d = ImageDraw.Draw(im)
ox, oy = 10, 40
def r(x, y, w, h, cor, rot, f=12):
    d.rectangle([ox + 2 * x, oy + 2 * y, ox + 2 * (x + w) - 1, oy + 2 * (y + h) - 1], outline=cor, width=2)
    d.multiline_text((ox + 2 * (x + w / 2), oy + 2 * (y + h / 2)), rot, fill=TXT, font=fonte(f), anchor="mm", align="center")
d.text((10, 12), "Painel de um jogador: 390×180 (ampliado 2x). Desenhe sempre \"em pé\": o código gira os de cima.",
       fill=TXT, font=fonte(13, True))
r(0, 0, W, H, LINHA, "")
r(0, 0, 130, H, ZONA_M, "ZONA DE TOQUE  −\n130×180\n(terço esquerdo)")
r(260, 0, 130, H, ZONA_P, "ZONA DE TOQUE  +\n130×180\n(terço direito)")
r(131, 50, 128, 84, DESTAQUE, "VIDA\n(desenhada pelo código)\n\nsegurar o meio =\n\"é a vez deste\njogador\"", f=10)
r(169, 146, 54, 26, LINHA, "P1", f=11)
r(60, 146, 100, 26, (150, 155, 175), "variação  −3", f=10)
r(236, 146, 120, 26, (150, 155, 175), "veneno / energia", f=10)
r(8, 146, 50, 26, (150, 155, 175), "cmdr", f=9)
r(110, 12, 170, 30, (150, 155, 175), "YOUR TURN / OUT /\nWINNER / GOES FIRST", f=9)
im.save(os.path.join(AQUI, "gabarito_painel_jogador.png"))
print("gabaritos gerados em", AQUI)
