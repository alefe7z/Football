# Protocolo de colaboração

Regras para Claude, ChatGPT e qualquer outro agente que trabalhe neste
repositório. Em caso de conflito com um pedido direto do dono, vale o pedido do dono.

## 1. Papéis

| Quem | Papel |
| --- | --- |
| **Dono do projeto** | Decide prioridades, aprova e faz o merge no `master`, resolve conflitos de área. |
| **Claude** | Agente de código. Área principal: jogabilidade e motor (`src/onthepitch`, partida 3D, testes). |
| **ChatGPT** | Agente de código. Área principal: modos de jogo e menus (`src/menu/career`, `src/league`, `src/data`). |

As áreas são uma proposta inicial; o dono pode trocá-las em
[DIVISAO-DE-TRABALHO.md](DIVISAO-DE-TRABALHO.md).

## 2. Branches

- **Ninguém faz commit direto no `master`.**
- Cada IA trabalha no seu branch:
  - Claude: `claude/<assunto>` (o atual é `claude/tender-pasteur-85r9js`)
  - ChatGPT: `chatgpt/<assunto>` (ex.: `chatgpt/cup-competitions`)
- Antes de começar, atualize o branch a partir do `master`:
  ```bash
  git fetch origin master && git merge origin/master
  ```
- Um branch = um assunto. Branches pequenos são revisados e juntados mais rápido.
- Nunca use `git push --force` no branch da outra IA. Nunca reescreva o histórico do `master`.

## 3. Quem mexe em quê

1. Antes de editar, **reserve a tarefa** no quadro "Em andamento" de
   [DIVISAO-DE-TRABALHO.md](DIVISAO-DE-TRABALHO.md), listando as pastas ou
   arquivos que vai tocar.
2. Se precisar mudar um arquivo da área da outra IA, a mudança deve ser
   **pequena e cirúrgica** (por exemplo, adicionar um campo ou um getter) e
   aparecer no relatório da sessão, na seção "Arquivos fora da minha área".
3. Arquivos compartilhados por todos, onde conflitos são comuns:
   - `data/locale/*.ini` (traduções): só **adicione** linhas novas perto de
     chaves do mesmo assunto; não reordene nem reformate o arquivo.
   - `sources.cmake`, `tests/CMakeLists.txt`, `.github/workflows/ci.yml`: só
     adicione linhas; não reorganize.
   - `docs/gap-list.md`, `ROADMAP.md`: edite só as linhas do item em que trabalhou.

## 4. Definição de pronto

Uma tarefa só está pronta quando:

- [ ] O jogo compila sem erros e sem avisos novos (`cmake --build build -j4`).
- [ ] Todos os testes passam (`cd build && ctest -j4`).
- [ ] A lógica nova sem dependência de gráficos tem teste unitário (veja
      `src/onthepitch/penaltyshootout.hpp` e `tests/onthepitch/penalty_shootout_test.cpp`
      como exemplo do padrão: regras em um `.hpp` puro, testadas sem o motor).
- [ ] Mudanças na partida 3D foram verificadas com um teste de fumaça
      (`tests/run_menu_smoke.sh`). Para um modo novo, crie um `data/menu_smoke_*.config`
      e um passo no CI.
- [ ] Texto que aparece na tela usa `TR("chave")` / `TRF(...)` e a chave existe
      nos **cinco** idiomas (`en`, `pt`, `es`, `fr`, `de`). O teste
      `LocalizationTest.EveryLanguageHasEveryEnglishKey` cobra isso.
- [ ] O código novo segue o `.clang-format` do projeto (só nos arquivos tocados).
- [ ] O relatório da sessão e a linha no `DIARIO.md` foram escritos.

## 5. Compilar e testar (Linux)

```bash
bash scripts/setup_linux_deps.sh          # uma vez: dependências
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j4
cd build && ctest -j4
```

Se o download do GoogleTest for bloqueado (rede restrita), instale o pacote
`googletest` e configure com
`-DFETCHCONTENT_SOURCE_DIR_GOOGLETEST=/usr/src/googletest`.

Partida completa automática numa tela virtual (cerca de 90 s):

```bash
cd build
../tests/run_menu_smoke.sh ./gameplayfootball ../data/menu_smoke_full_match.config 420 \
  "[menu-smoke] Full-match verification succeeded, quitting test run"
```

Para ver imagens do jogo durante o teste, defina `GF_CAPTURE_DIRECTORY=<pasta>`
(e `GF_CAPTURE_INTERVAL_MS=1000`): o jogo salva quadros em PNG.

Atenção: os testes de fumaça **regravam** o arquivo `.config` que recebem. Se
`git status` mostrar um `data/menu_smoke_*.config` alterado depois de um teste,
desfaça com `git checkout` antes do commit.

## 6. Commits

- Mensagem em inglês, no estilo do projeto: `Área: o que mudou` (ex.:
  `Gameplay: penalty shootout for drawn knockout matches`).
- Um commit por mudança lógica. Não misture formatação de arquivos inteiros
  com mudanças de comportamento.
- Não versione: `build/`, saves, `.config` alterados por testes, arquivos de log,
  credenciais.

## 7. Entrega (handoff)

Ao terminar uma sessão, a IA:

1. Faz push do branch.
2. Escreve `docs/colaboracao/relatorios/AAAA-MM-DD-<ia>-<assunto>.md` a partir de
   [modelos/RELATORIO-DE-SESSAO.md](modelos/RELATORIO-DE-SESSAO.md).
3. Acrescenta uma entrada no topo de [DIARIO.md](DIARIO.md).
4. Move a tarefa de "Em andamento" para "Concluído" na divisão de trabalho.
5. Atualiza [ESTADO-ATUAL.md](ESTADO-ATUAL.md) se algo importante mudou.

## 8. Honestidade nos relatórios

- Diga o que **foi verificado** (e como) separado do que **não foi**.
- Teste falhou? Copie a saída. Não deu para testar? Escreva isso.
- Não descreva como pronto algo que é um esboço ou só existe em um teste.
- Números (testes, placares, tempos) vêm de execuções reais, não de estimativas.

## 9. Revisão cruzada

Antes do merge de um branch grande, o dono pode pedir à outra IA uma revisão
usando [modelos/REVISAO-CRUZADA.md](modelos/REVISAO-CRUZADA.md). A revisão
procura bugs de verdade (com cenário que quebra), não preferências de estilo.
