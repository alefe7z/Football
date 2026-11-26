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
3. Com o novo clube selecionado, use **Editar nome de jogador** para mudar
   o elenco. Use **Trocar escudo** e os botões de uniforme para escolher
   imagens PNG. Para os uniformes, use como molde uma das texturas existentes.
4. Repita para cada clube. O editor não estabelece um máximo de cadastros.
5. Clique em **Jogar com este perfil** e escolha o executável compilado.
   Em **Partida Rápida**, selecione a liga para encontrar seu time.

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
