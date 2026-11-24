# Gera o PDF "Diário do Projeto — Visor de Vida MTG".
# Para atualizar: edite as seções abaixo (ETAPA_ATUAL, ETAPAS, DECISOES, HISTORICO) e rode de novo.
# Uso (na pasta "Projeto Arduíno"):  python3 docs/fonte/diario.py Diario_do_Projeto.pdf docs/fonte
# Requer: pip install reportlab
import sys
from reportlab.lib.pagesizes import A4
from reportlab.lib.units import mm
from reportlab.lib import colors
from reportlab.lib.styles import ParagraphStyle
from reportlab.platypus import (BaseDocTemplate, PageTemplate, Frame, Paragraph, Spacer, Table,
                                TableStyle, Image, KeepTogether, PageBreak)

OUT = sys.argv[1] if len(sys.argv) > 1 else "Diario_do_Projeto.pdf"
IMG = sys.argv[2] if len(sys.argv) > 2 else "."
ATUALIZADO = "24/11/2026"

# Fontes padrão do PDF (Helvetica/Courier): não são embutidas, o arquivo fica pequeno.

NAVY = colors.HexColor("#0B1E5B")
BLUE = colors.HexColor("#1636D8")
LIME = colors.HexColor("#7BC414")
LIGHT = colors.HexColor("#EEF2FB")
GREY = colors.HexColor("#5A6378")
LINE = colors.HexColor("#C9D2E8")
OK = colors.HexColor("#2E8B3E")
WARN = colors.HexColor("#C77700")
TODO = colors.HexColor("#8A93A8")

body = ParagraphStyle("body", fontName="Helvetica", fontSize=10.5, leading=15, textColor=colors.HexColor("#1B2233"), spaceAfter=6)
small = ParagraphStyle("small", parent=body, fontSize=9, leading=12.5, textColor=GREY)
cell = ParagraphStyle("cell", parent=body, fontSize=9.5, leading=13, spaceAfter=0)
cellb = ParagraphStyle("cellb", parent=cell, fontName="Helvetica-Bold")
h1 = ParagraphStyle("h1", fontName="Helvetica-Bold", fontSize=15, leading=20, textColor=NAVY, spaceBefore=14, spaceAfter=8, keepWithNext=1)
h2 = ParagraphStyle("h2", fontName="Helvetica-Bold", fontSize=11.5, leading=16, textColor=BLUE, spaceBefore=10, spaceAfter=4, keepWithNext=1)
title = ParagraphStyle("title", fontName="Helvetica-Bold", fontSize=24, leading=30, textColor=NAVY)
subtitle = ParagraphStyle("subtitle", fontName="Helvetica", fontSize=12, leading=17, textColor=GREY)
bullet = ParagraphStyle("bullet", parent=body, leftIndent=14, bulletIndent=3, spaceAfter=3)
mono = ParagraphStyle("mono", parent=body, fontName="Courier", fontSize=9, leading=12.5, backColor=LIGHT,
                      borderPadding=(6, 6, 6, 6), leftIndent=6, rightIndent=6, spaceBefore=4, spaceAfter=10)

def P(t, s=body): return Paragraph(t, s)
def bullets(items): return [Paragraph(i, bullet, bulletText="•") for i in items]

def table(rows, widths, header=True, zebra=True, extra=None):
    data = [[c if not isinstance(c, str) else Paragraph(c, cellb if (header and r == 0) else cell) for c in row]
            for r, row in enumerate(rows)]
    t = Table(data, colWidths=widths, repeatRows=1 if header else 0)
    st = [("VALIGN", (0, 0), (-1, -1), "TOP"),
          ("TOPPADDING", (0, 0), (-1, -1), 5), ("BOTTOMPADDING", (0, 0), (-1, -1), 5),
          ("LEFTPADDING", (0, 0), (-1, -1), 6), ("RIGHTPADDING", (0, 0), (-1, -1), 6),
          ("LINEBELOW", (0, 0), (-1, -1), 0.5, LINE)]
    if header:
        st += [("BACKGROUND", (0, 0), (-1, 0), LIGHT), ("LINEBELOW", (0, 0), (-1, 0), 1, NAVY)]
    if extra: st += extra
    t.setStyle(TableStyle(st))
    return t

def status(s):
    c = {"Concluída": OK, "Em andamento": WARN, "Pendente": TODO, "Rascunho pronto": BLUE, "Pronto no simulador": BLUE}[s]
    return Paragraph(f'<font color="{c.hexval()}"><b>{s}</b></font>', cell)

# ----------------------------------------------------------------------------------- conteúdo
ETAPA_ATUAL = "Etapa 1 (comprar a placa). Etapas 3 a 11 prontas e testadas no simulador; falta rodá-las na placa"
PROXIMO = ("Fazer o pedido da placa ESP32-8048S043C (ver seção 4) e, enquanto ela não chega, rodar as etapas 4 a 11 no simulador "
           "(um README por pasta em etapas/). Quando a placa chegar: Etapa 2 (instalar a IDE) e carregar as etapas 3 a 11 nela, na ordem.")

ETAPAS = [
    ("0", "Definir o que o visor deve fazer (a partir do vídeo de referência)", "Concluída"),
    ("0", "Escolher o hardware", "Concluída"),
    ("S", "Simulador no PC: roda os mesmos programas numa janela 800×480 (o mouse faz o papel do dedo)", "Concluída"),
    ("1", "Comprar a placa ESP32-8048S043C e um cabo USB-C de dados", "Em andamento"),
    ("2", "Instalar a Arduino IDE 2, o pacote de placas ESP32 e a biblioteca LovyanGFX", "Pendente"),
    ("3", "Primeiro programa: acender a tela e escrever um texto (etapas/etapa03_ola)", "Pronto no simulador"),
    ("4", "Testar o touch: desenhar um ponto onde o dedo encosta (etapas/etapa04_touch)", "Pronto no simulador"),
    ("5", "Um painel de jogador com a vida e os botões + e - (etapas/etapa05_painel)", "Pronto no simulador"),
    ("6", "Quatro painéis, com os dois de cima girados 180° (etapas/etapa06_quatro_paineis)", "Pronto no simulador"),
    ("7", "Menu inicial e configuração do Commander (etapas/etapa07a11_visor)", "Pronto no simulador"),
    ("8", "Barra superior: UNDO, LOCK, RESET, MENU e FIRST (etapas/etapa07a11_visor)", "Pronto no simulador"),
    ("9", "Marcadores de veneno e de energia (etapas/etapa07a11_visor)", "Pronto no simulador"),
    ("10", "Dano de comandante (etapas/etapa07a11_visor)", "Pronto no simulador"),
    ("11", "Histórico da partida (etapas/etapa07a11_visor)", "Pronto no simulador"),
    ("12", "Extras: salvar a partida ao desligar, bateria, case impressa em 3D", "Pendente"),
]

# Melhorias de produção (depois que o contador funcionar offline)
ETAPAS_PROD = [
    ("13.1", "Gerar a chave de API do Playgroup e testar pelo terminal/Postman (dá para fazer sem a placa)", "Pendente"),
    ("13.2", "Conectar a placa ao Wi-Fi, guardando senha e chave na memória interna", "Pendente"),
    ("13.3", "Puxar o setup da partida do Playgroup (jogadores, decks e vida inicial)", "Pendente"),
    ("13.4", "Enviar os eventos da partida (dano, comandante, veneno) para o Playgroup", "Pendente"),
]

DECISOES = [
    ("24/11/2026", "Usar uma placa com ESP32-S3 e tela de 4,3\" integradas (Sunton ESP32-8048S043C), em vez de Arduino Mega + shield + tela separada, como no vídeo.",
     "É uma peça só, sem fios nem solda; tela 800×480 com touch capacitivo (não precisa calibrar); muito mais memória e velocidade; programa-se na mesma IDE do Arduino."),
    ("24/11/2026", "Biblioteca gráfica: LovyanGFX.",
     "Já traz a configuração pronta desta placa (pinos da tela e do touch), o que reduz o risco para quem está começando."),
    ("24/11/2026", "Textos da interface em inglês (LIFE, POISON, COMMANDER...).",
     "Igual ao vídeo e aos termos usados no Magic; as fontes da biblioteca não têm acentos. Pode ser trocado depois."),
    ("24/11/2026", "O ícone de raio vira contador de energia.",
     "O vídeo não mostra o que o raio faz; energia é um palpite e pode ser trocado."),
    ("24/11/2026", "Integração com o Playgroup.gg entra como melhoria de produção (etapa 13), depois do contador offline.",
     "O Wi-Fi é um extra: um problema de rede nunca pode travar a partida."),
    ("24/11/2026", "Testar no PC com um simulador (LovyanGFX + SDL), em vez de um Arduino barato.",
     "Um Arduino Uno tem 2 KB de memória e não consegue guardar uma tela de 800×480 (~750 KB). O simulador roda exatamente o mesmo código da placa, de graça."),
    ("24/11/2026", "Versionar o projeto no GitHub; o Claude faz os commits e o Enzo atualiza a pasta local com git pull.",
     "Histórico de cada mudança e backup fora do computador. O Claude não grava mais direto na pasta local, para o git pull não dar conflito."),
    ("24/11/2026", "Trabalhar em mini tarefas, uma etapa por vez.",
     "Pedido do Enzo, para aprender aos poucos e acompanhar o projeto."),
]

HISTORICO = [
    ("24/11/2026", "Análise do vídeo de referência e especificação das funções. Escolha do hardware e da biblioteca. "
                   "Pesquisa de onde comprar. Primeiro rascunho do firmware completo (compila, ainda não testado na placa). "
                   "Roteiro em 12 mini tarefas. Criação deste diário."),
    ("24/11/2026", "Confirmado que a placa tem Wi-Fi 2,4 GHz e Bluetooth 5. Pesquisa da API pública do Playgroup.gg "
                   "e inclusão da integração como melhoria de produção (etapa 13)."),
    ("24/11/2026", "Etapa 3 adiantada: primeiro programa (etapas/etapa03_ola) compilado para a placa e testado no novo "
                   "simulador do PC (pasta simulador). Pasta do projeto espelhada no Google Drive (código e scripts; "
                   "o PDF e as fotos ficaram só na pasta local)."),
    ("24/11/2026", "Simulador instalado e rodando no PC do Enzo (Fedora: dnf install gcc-c++ make cmake git sdl2-compat-devel). "
                   "Envio ao Google Drive encerrado a pedido do Enzo: daqui em diante, os arquivos ficam só na pasta local."),
    ("24/11/2026", "Etapas 4 a 11 montadas sem a placa: touch, um painel, quatro painéis girados e o visor completo "
                   "(menu, barra superior, veneno/energia, comandante e histórico). Todas compilam para a placa e foram "
                   "testadas no simulador com toques automáticos. O simulador passou a compilar os .cpp da pasta da etapa."),
    ("24/11/2026", "Projeto publicado no GitHub (github.com/ensosampaio/visor-vida-mtg, privado). A partir daqui, o Claude faz commit e "
                   "push direto no repositório, e a pasta local é atualizada com git pull."),
    ("24/11/2026", "Briefing para um artista fazer a arte personalizada das telas (docs/arte): medidas e limitações da tela, "
                   "lista de peças, formato de entrega, direitos e checklist, com gabaritos em tamanho real e versão em PDF."),
]

# ----------------------------------------------------------------------------------- montagem
def on_page(c, doc):
    c.saveState()
    c.setFillColor(NAVY); c.rect(0, A4[1] - 9 * mm, A4[0], 9 * mm, stroke=0, fill=1)
    c.setFillColor(LIME); c.rect(0, A4[1] - 10 * mm, A4[0], 1 * mm, stroke=0, fill=1)
    c.setFont("Helvetica", 8); c.setFillColor(colors.white)
    c.drawString(18 * mm, A4[1] - 6 * mm, "Visor de Vida MTG — Diário do Projeto")
    c.drawRightString(A4[0] - 18 * mm, A4[1] - 6 * mm, f"Atualizado em {ATUALIZADO}")
    c.setFillColor(GREY); c.setFont("Helvetica", 8)
    c.drawRightString(A4[0] - 18 * mm, 10 * mm, f"Página {doc.page}")
    c.restoreState()

doc = BaseDocTemplate(OUT, pagesize=A4, leftMargin=18 * mm, rightMargin=18 * mm, topMargin=18 * mm, bottomMargin=18 * mm,
                      title="Visor de Vida MTG — Diário do Projeto", author="Enzo Sampaio")
frame = Frame(doc.leftMargin, doc.bottomMargin, doc.width, doc.height, id="f")
doc.addPageTemplates([PageTemplate(id="p", frames=[frame], onPage=on_page)])
W = doc.width
s = []

# capa / resumo
s += [Spacer(1, 6 * mm), P("Visor de Vida MTG", title),
      P("Contador de vida de Magic: The Gathering numa placa com tela touch — diário do projeto", subtitle),
      Spacer(1, 6 * mm)]

box = Table([[Paragraph("<b>ONDE ESTAMOS</b>", ParagraphStyle("x", parent=cell, textColor=colors.white))],
             [Paragraph(f"<b>Etapa atual:</b> {ETAPA_ATUAL}<br/><b>Próximo passo:</b> {PROXIMO}", cell)]],
            colWidths=[W])
box.setStyle(TableStyle([("BACKGROUND", (0, 0), (-1, 0), BLUE), ("BACKGROUND", (0, 1), (-1, 1), LIGHT),
                         ("BOX", (0, 0), (-1, -1), 1, BLUE), ("LEFTPADDING", (0, 0), (-1, -1), 10),
                         ("RIGHTPADDING", (0, 0), (-1, -1), 10), ("TOPPADDING", (0, 0), (-1, -1), 7),
                         ("BOTTOMPADDING", (0, 0), (-1, -1), 8)]))
s += [box, Spacer(1, 4 * mm)]

s += [P("1. Roteiro e status", h1),
      P("O projeto está dividido em etapas pequenas, feitas uma de cada vez. A coluna Status é atualizada a cada sessão. "
        "\"Pronto no simulador\" = o código compila para a placa e foi testado no simulador do PC; falta rodar na placa.")]
rows = [["Etapa", "O que é", "Status"]] + [[n, t, status(st)] for n, t, st in ETAPAS]
s += [table(rows, [17 * mm, W - 17 * mm - 34 * mm, 34 * mm])]
rows = [["Etapa", "O que é", "Status"]] + [[n, t, status(st)] for n, t, st in ETAPAS_PROD]
s += [KeepTogether([P("Melhorias de produção", h2),
      P("Só depois que o contador estiver funcionando offline. Detalhes na seção 6.", small),
      table(rows, [17 * mm, W - 17 * mm - 34 * mm, 34 * mm])])]

# objetivo
s += [P("2. Objetivo e referência", h1),
      P("Montar um visor para marcar a vida dos jogadores numa partida de Magic, inspirado num vídeo de referência "
        "(WhatsApp, 24/11/2026). No vídeo aparece um Arduino com uma tela touch rodando um contador completo para até 4 jogadores.")]
imgw = (W - 6 * mm) / 2
imgs = Table([[Image(f"{IMG}/c.jpg", imgw, imgw * 576 / 1024), Image(f"{IMG}/b.jpg", imgw, imgw * 576 / 1024)],
              [P("Tela de jogo do vídeo: 4 jogadores, os dois de cima de cabeça para baixo.", small),
               P("Histórico da partida, com a última ação no topo.", small)]],
             colWidths=[imgw + 3 * mm, imgw + 3 * mm])
imgs.setStyle(TableStyle([("LEFTPADDING", (0, 0), (-1, -1), 0), ("VALIGN", (0, 0), (-1, -1), "TOP")]))
s += [imgs, Spacer(1, 3 * mm)]

s += [P("Funções que queremos reproduzir", h2)]
s += bullets([
    "<b>Menu:</b> Commander (20, 30 ou 40 de vida; 1 a 4 jogadores) e Standard (20 de vida; 2 jogadores).",
    "<b>Tela de jogo:</b> um painel por jogador, com a vida em números grandes e botões + e -. "
    "Os painéis de cima ficam girados 180° para quem está do outro lado da mesa. Mostra a última variação (ex.: -3).",
    "<b>Barra superior:</b> LOG (histórico), LOCK (trava a tela contra toques acidentais), UNDO (desfaz), "
    "FIRST (sorteia quem começa), RESET e MENU.",
    "<b>Veneno:</b> contador por jogador; 10 marcadores eliminam o jogador.",
    "<b>Dano de comandante:</b> registra quanto cada jogador levou de cada comandante; 21 de um mesmo comandante elimina. "
    "Aplicar o dano também tira vida.",
    "<b>Eliminação:</b> vida <= 0, 10 de veneno ou 21 de um comandante. O painel do eliminado fica marcado em vermelho.",
    "<b>Histórico:</b> lista paginada de tudo o que aconteceu (ex.: \"P2 LIFE -9 = 31\").",
])

# decisões
s += [P("3. Decisões tomadas", h1)]
rows = [["Data", "Decisão", "Por quê"]] + [[d, a, b] for d, a, b in DECISOES]
s += [table(rows, [25 * mm, (W - 25 * mm) * 0.48, (W - 25 * mm) * 0.52])]

# compras
s += [P("4. Lista de compras", h1),
      P("<b>Só é preciso uma placa.</b> No vídeo foram usadas duas peças: um Arduino (o \"cérebro\") e uma tela encaixada nele. "
        "A placa escolhida já junta as duas coisas: o processador ESP32-S3 fica soldado atrás da tela. "
        "Ela se liga ao computador pelo USB-C, sem fios nem solda.")]
rows = [["Item", "Detalhes", "Obrigatório?"],
        ["Placa Sunton <b>ESP32-8048S043C</b>", "ESP32-S3, tela IPS de 4,3\" com 800×480, touch capacitivo GT911, 16 MB de flash, 8 MB de PSRAM, entrada para microSD, USB-C.", "Sim"],
        ["Cabo USB-C <b>de dados</b>", "Cabos que só carregam não funcionam: o computador não enxerga a placa.", "Sim"],
        ["Cartão microSD", "Para etapas futuras (não é necessário no começo).", "Não"]]
s += [table(rows, [50 * mm, W - 50 * mm - 30 * mm, 30 * mm])]

s += [P("Cuidados ao escolher o anúncio", h2)]
s += bullets([
    "Confira se a tela é <b>800×480</b>. Existe a ESP32-<b>4827</b>S043, quase igual, com 480×272 pixels, e alguns anúncios misturam as duas.",
    "Confira se o touch é <b>capacitivo</b>: o <b>C</b> no final do nome. A versão R tem touch resistivo e a N não tem touch.",
    "Variações do nome que são a mesma placa: ESP32-8048S043C, ESP32-8048S043C-I, \"ESP32-S3 4.3 inch 800x480 Smart Display\".",
])

s += [P("Onde comprar (preços pesquisados em 24/11/2026)", h2)]
rows = [["Loja", "Anúncio", "Preço", "Observação"],
        ["AliExpress", "DIYmalls ESP32-8048S043C-I", "US$ 39,42", "Recomendado: modelo exato, nota 4,8, 122 vendidos."],
        ["AliExpress", "Surenoo ESP32-8048S043 / 4827S043", "US$ 35,99", "Escolher a variante 800×480 com touch capacitivo."],
        ["Mercado Livre", "Waveshare ESP32-S3 4,3\" (compra internacional)", "R$ 408,54", "Equivalente, mas é outro modelo: o código precisaria de ajustes."],
        ["Amazon BR", "Waveshare 4,3\" B com case", "R$ 943,87", "Caro demais para o projeto."],
        ["RoboBuilders", "ESP32 com display de 4,3\"", "R$ 482,71", "Evitar: tela de 480×272."]]
s += [table(rows, [30 * mm, 48 * mm, 22 * mm, W - 100 * mm])]
s += [P("No AliExpress, os impostos de importação aparecem no fechamento do pedido e a entrega costuma levar algumas semanas.", small)]
s += [P("Links", h2)]
s += bullets([
    '<link href="https://www.aliexpress.us/item/3256808406435888.html" color="#1636D8">AliExpress — DIYmalls ESP32-8048S043C-I</link>',
    '<link href="https://www.aliexpress.us/item/3256809100592093.html" color="#1636D8">AliExpress — Surenoo ESP32-8048S043</link>',
    '<link href="https://www.mercadolivre.com.br/esp32-s3-lcd-de-43-polegadas-com-tela-de-toque-cativa-de-80/p/MLB2022723079" color="#1636D8">Mercado Livre — ESP32-S3 LCD 4,3" (Waveshare)</link>',
])

# software
s += [P("5. Software", h1),
      P("O programa é escrito em C++ na <b>Arduino IDE</b>. Quando a placa chegar, a configuração será esta:")]
rows = [["Item", "Valor"],
        ["IDE", "Arduino IDE 2 (Linux)"],
        ["Pacote de placas", "esp32 da Espressif, versão 3.x (testado com 3.3.4)"],
        ["Biblioteca", "LovyanGFX (instalar pelo Gerenciador de Bibliotecas)"],
        ["Placa no menu Ferramentas", "ESP32S3 Dev Module"],
        ["Flash Size", "16MB (128Mb)"],
        ["PSRAM", "<b>OPI PSRAM</b> (obrigatório: a tela usa essa memória)"],
        ["Partition Scheme", "qualquer esquema de 16MB"]]
s += [table(rows, [55 * mm, W - 55 * mm])]

s += [P("Rascunho do firmware", h2),
      P("Antes de dividirmos o trabalho em etapas, foi escrita uma primeira versão completa do programa. "
        "Ela <b>compila sem erros</b> para a placa, mas <b>ainda não foi testada numa placa de verdade</b>. "
        "Ele foi a base das etapas 7 a 11: a versão de referência agora é etapas/etapa07a11_visor, revisada e testada no simulador."),
      P("Local: <font name=\"Courier\">Projeto Arduíno/rascunho/LifeCounter/</font>")]
rows = [["Arquivo", "O que faz"],
        ["LifeCounter.ino", "Ponto de entrada: liga a tela e o touch e repassa os toques para a interface."],
        ["game.h / game.cpp", "Regras do jogo em C++ puro: vida, veneno, energia, dano de comandante, eliminação, undo e histórico. Não depende da tela."],
        ["ui.h / ui.cpp", "Telas e toques: menu, configuração, jogo, veneno/energia, comandante, histórico e caixas de confirmação."]]
s += [table(rows, [40 * mm, W - 40 * mm])]
s += [P("Como o desenho funciona", h2)]
s += bullets([
    "Cada quadro é desenhado numa \"tela invisível\" na memória (sprite) e copiado de uma vez para o display, para evitar que a tela pisque.",
    "Os painéis de cima são desenhados normalmente e depois girados 180° (a ordem dos pixels é invertida).",
    "Toques seguidos no mesmo contador, num intervalo de até 3 segundos, viram uma única linha no histórico.",
    "Segurar o + ou o - repete a ação; depois de um tempo segurando, o valor muda de 5 em 5.",
])

s += [P("Simulador no PC", h2),
      P("A pasta <font name=\"Courier\">simulador/</font> roda qualquer programa das etapas numa janela de 800×480 no computador. "
        "O código é o mesmo da placa: o arquivo <font name=\"Courier\">placa.h</font> de cada etapa escolhe entre a tela de verdade e a janela. "
        "Instalação e uso no <font name=\"Courier\">simulador/README.md</font>.")]
simw = 80 * mm
s += [Table([[Image(f"{IMG}/etapa03_simulador.png", simw, simw * 480 / 800),
              P("Etapa 3 rodando no simulador: o mesmo programa que vai para a placa. "
                "O contador no canto mostra que o loop() está funcionando.", small)]],
            colWidths=[simw + 6 * mm, W - simw - 6 * mm],
            style=TableStyle([("VALIGN", (0, 0), (-1, -1), "MIDDLE"), ("LEFTPADDING", (0, 0), (-1, -1), 0)]))]
rows = [["Pasta", "Conteúdo"],
        ["etapas/etapa03_ola/", "Etapa 3: programa comentado, placa.h, guia (README.md) e print do simulador."],
        ["etapas/etapa04_touch/ a etapa06_quatro_paineis/", "Etapas 4 a 6: um programa pequeno e comentado por etapa, cada um com guia e print."],
        ["etapas/etapa07a11_visor/", "Visor completo: .ino, game.h/.cpp (regras), ui.h/.cpp (telas), guia que liga cada etapa (7 a 11) ao código, e prints de todas as telas."],
        ["simulador/", "CMakeLists.txt, main.cpp e arduino_shim.h (imita Serial, delay e millis no PC) e o README.md."],
        ["rascunho/LifeCounter/", "Primeiro rascunho do firmware (substituído por etapas/etapa07a11_visor)."],
        ["docs/fonte/", "Script e imagens que geram este diário."],
        ["docs/arte/", "Briefing para o artista (BRIEFING_ARTE.md e Briefing_Arte.pdf) e gabaritos das telas em tamanho real."]]
s += [Spacer(1, 3 * mm), table(rows, [48 * mm, W - 48 * mm])]
s += [Spacer(1, 3 * mm), Table([[Image(f"{IMG}/visor_jogo.png", simw, simw * 480 / 800),
              P("Visor completo (etapas 7 a 11) no simulador: P2 levou -3 e P4 -1. "
                "Os ícones do meio abrem dano de comandante, energia e veneno.", small)]],
            colWidths=[simw + 6 * mm, W - simw - 6 * mm],
            style=TableStyle([("VALIGN", (0, 0), (-1, -1), "MIDDLE"), ("LEFTPADDING", (0, 0), (-1, -1), 0)]))]

# melhorias de produção
s += [P("6. Melhorias de produção: integração com o Playgroup.gg", h1),
      P("O Playgroup.gg é um app gratuito para registrar partidas presenciais de Commander (vida, dano de comandante, "
        "veneno, estatísticas e ELO). Ele tem uma <b>API pública</b>: a chave é gerada nas configurações da conta, sem processo de aprovação. "
        "Como a placa tem Wi-Fi, o visor pode conversar com essa API."),
      P("O que a API permite", h2)]
rows = [["Objetivo", "Como", "Situação"],
        ["Puxar o setup da partida", "GET /playgroups/{id}/games traz as partidas com <font name=\"Courier\">life_amount</font> (vida inicial) e as participações "
         "(<font name=\"Courier\">user_name</font>, <font name=\"Courier\">deck_name</font>, <font name=\"Courier\">winner</font>).",
         "Possível, com um passo no celular: a API <b>não cria partidas</b>. A partida é criada no app e o visor procura a mais recente sem <font name=\"Courier\">ended_at</font> (não há filtro de partida em andamento)."],
        ["Enviar o que acontece na mesa", "POST /games/{game_id}/events/batch: eventos com tipo, jogador de origem (\"0\" a \"5\"), alvo, turno, valor e comandante.",
         "Possível, mas a lista de tipos de evento não está documentada (só cita Damage, Kill, PassTurn). Confirmar no Discord deles ou testando."],
        ["Mostrar dados extras", "GET /me, /users/{id}/decks, /decks/{id} e estatísticas de comandantes.", "Possível."]]
s += [table(rows, [36 * mm, (W - 36 * mm) * 0.5, (W - 36 * mm) * 0.5])]
s += [P("Regras e cuidados", h2)]
s += bullets([
    "Endereço base: <font name=\"Courier\">https://playgroup.gg/api/public/v1</font>. Autenticação: cabeçalho <font name=\"Courier\">Authorization: Bearer &lt;chave&gt;</font>.",
    "Limite de uso: 400 requisições por minuto com um <font name=\"Courier\">User-Agent</font> descritivo; sem ele, só 10. Enviar eventos em lote (fim do turno), não a cada toque.",
    "Para enviar eventos, o dono da chave precisa ser participante da partida. O lote é \"tudo ou nada\": se um evento for inválido, nenhum é gravado.",
    "Senha do Wi-Fi e chave da API ficam na memória interna da placa, nunca no código que vai para o Git.",
    "Se a rede cair, o visor continua funcionando normalmente e envia depois.",
])
s += [P('Documentação: <link href="https://playgroup.gg/api-docs/index.html" color="#1636D8">playgroup.gg/api-docs</link> · '
        '<link href="https://playgroup.gg/.well-known/openapi.yaml" color="#1636D8">especificação OpenAPI</link>', small)]

# histórico
rows = [["Data", "O que foi feito"]] + [[d, t] for d, t in HISTORICO]
s += [KeepTogether([P("7. Histórico de atualizações", h1), table(rows, [25 * mm, W - 25 * mm])])]
s += [Spacer(1, 4 * mm), P("Este documento é atualizado a cada sessão de trabalho. Basta pedir: \"atualiza o diário do projeto\".", small)]

doc.build(s)
print("ok", OUT)
