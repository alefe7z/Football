# Personalize o seu Football

Na pasta do repositório, execute `python3 tools/content_editor.py` (Windows:
`py tools/content_editor.py`). O editor usa Python 3 com Tkinter. Para jogar
com o perfil, compile o jogo desta versão seguindo o README. O editor pode
preparar o conteúdo antes da compilação.

## Criar seu primeiro clube

1. Escolha a liga de destino e um clube para usar como modelo.
2. Clique em **Criar clube a partir do modelo**, digite o nome e a sigla.
   O editor cria novos registros para o time e seus jogadores e copia a
   formação, as táticas, o escudo e os dois uniformes.
3. Com o novo clube selecionado, use **Editar jogador e rosto 3D** para mudar
   o elenco. Use **Trocar escudo** e os botões de uniforme para escolher
   imagens PNG. Para os uniformes, use como molde uma das texturas existentes.
4. Repita para cada clube. O editor não estabelece um máximo de cadastros.
5. Clique em **Jogar com este perfil** e escolha o executável compilado.
   Em **Partida Rápida**, selecione a liga para encontrar seu time.

## Editar a forma do rosto

Em **Editar jogador e rosto 3D**, escolha um atleta e abra **Editar rosto 3D**.
Arraste a prévia para girar a cabeça ou use o controle **Girar**.
As seis abas oferecem 21 ajustes de forma. Em **Olhos**, altere a distância,
a abertura das pálpebras, a inclinação dos cantos e a dobra palpebral.
Também é possível escolher a cor da íris. Em **Nariz**, ajuste a projeção,
a largura, as narinas e a inclinação da ponta. **Boca** controla a largura,
a posição e os cantos. **Lábios** permite ajustar cada lábio e o arco do cupido.
Em **Orelhas**, altere o tamanho, a abertura e o lóbulo.

**Salvar rosto** grava a escolha para o atleta e exporta a malha e sua
textura em `media/objects/players/faces` no perfil. Abra a partida pelo
editor usando o executável compilado desta versão. O penteado é aplicado
pelo jogo. Para voltar aos traços neutros do modelo detalhado, use
**Restaurar traços** e salve. Fechar a janela descarta a edição em andamento.
Os rostos do editor anterior continuam funcionando e podem ser adaptados
à nova cabeça ao reabrir e salvar. Para compartilhar um perfil, inclua a
pasta de faces e o banco de dados.

A cabeça e os controles anatômicos são derivados dos recursos CC0 do
MakeHuman. [Créditos e licença](../tools/face_assets/README.md)

## Bolas e chuteiras

Escolha **Bola**, **Chuteira** ou **Solado** e clique em **Importar textura
JPEG**. Cada arquivo com nome diferente vira um item do catálogo. Escolha
o item e clique em **Usar item selecionado** antes de abrir o jogo.
**Original.jpg** recupera a aparência inicial. A bola vale para a partida
inteira e a chuteira e o solado valem para todos os jogadores. Use a imagem
original do catálogo como molde para posicionar os desenhos no modelo 3D.

## Outros conteúdos e perfis

**Abrir pasta do perfil** mostra os arquivos usados pelo jogo. Em `media`
ficam modelos, texturas, sons, menus e animações. Em `locale` ficam os textos.
Para personalização manual, mantenha os caminhos e formatos dos arquivos
que o motor carrega. A inclusão de novas regras de jogo exige programação.

O perfil padrão fica em `profiles/meu-futebol`. Para criar ou reabrir outro,
execute `python3 tools/content_editor.py --perfil "caminho/do/perfil"`.
Cada perfil tem sua própria base de clubes, catálogo, configurações e saves.
O cadastro de clubes altera a base usada para partidas rápidas e como origem
de novos jogos salvos. Carreiras já iniciadas usam o banco do próprio save.
Para fazer um backup, feche o jogo e copie a pasta inteira do perfil.
