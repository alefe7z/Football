# Texto para colar no ChatGPT

Copie o bloco abaixo e cole no ChatGPT (ou no Codex) antes de pedir qualquer
tarefa. Ele explica as regras para o ChatGPT trabalhar junto com o Claude sem
conflito.

---

```text
Você vai trabalhar no meu jogo de futebol, o repositório alefe7z/Football
(League-Soccer / Gameplay Football, C++17 com SDL2, OpenGL, OpenAL e SQLite).
Outra IA, o Claude, trabalha no mesmo repositório ao mesmo tempo. Para não
haver conflito, siga estas regras:

1. Antes de começar, leia estes arquivos do repositório:
   - docs/colaboracao/PROTOCOLO.md (regras)
   - docs/colaboracao/DIVISAO-DE-TRABALHO.md (quem mexe em quê)
   - docs/colaboracao/ESTADO-ATUAL.md (o que já funciona)
   - docs/colaboracao/DIARIO.md (o que foi feito nas últimas sessões)
   - docs/gap-list.md (lista detalhada do que falta)

2. Suas áreas: modo carreira (src/menu/career, src/data/career*), liga e
   competições (src/league), menus fora da partida (src/menu, menos
   src/menu/ingame) e banco de dados. A partida 3D (src/onthepitch,
   src/menu/ingame) é área do Claude: só faça mudanças pequenas lá e avise no
   relatório.

3. Nunca faça commit no master. Trabalhe num branch chamado chatgpt/<assunto>,
   criado a partir do master atualizado.

4. Antes de editar, reserve a tarefa no quadro "Em andamento" de
   docs/colaboracao/DIVISAO-DE-TRABALHO.md.

5. Uma tarefa só está pronta quando: compila sem avisos novos, todos os testes
   passam (cd build && ctest -j4), a lógica nova tem teste unitário, todo
   texto da tela existe nos 5 idiomas (data/locale/en, pt, es, fr, de), e
   você escreveu o relatório da sessão em docs/colaboracao/relatorios/ (use o
   modelo em docs/colaboracao/modelos/RELATORIO-DE-SESSAO.md) e uma entrada
   no topo de docs/colaboracao/DIARIO.md.

6. Seja honesto no relatório: separe o que você testou de verdade do que não
   testou. Se não conseguiu compilar ou rodar os testes, diga isso.

7. Nos arquivos compartilhados (data/locale/*.ini, sources.cmake,
   tests/CMakeLists.txt, .github/workflows/ci.yml) apenas ADICIONE linhas; não
   reorganize nem reformate.

Minha primeira tarefa para você: <escreva aqui a tarefa, por exemplo
"implemente uma copa eliminatória na liga, usando a disputa de pênaltis
que o Claude criou (veja MatchData::GetWinnerTeamID e a seção 'Pontos de
contato' da divisão de trabalho)">.
```

---

## Dicas para o dono

- Se o ChatGPT não tiver acesso direto ao repositório, cole também o conteúdo
  dos arquivos que ele precisa ler e peça que ele responda com um *patch*
  (`git diff`) para você aplicar.
- Quando ele terminar, peça ao Claude: "revise o branch chatgpt/<assunto>
  usando docs/colaboracao/modelos/REVISAO-CRUZADA.md". Faça o mesmo no
  sentido contrário.
