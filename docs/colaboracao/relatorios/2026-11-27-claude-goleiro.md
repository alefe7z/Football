# Relatório de sessão — 2026-11-27 — Claude — goleiro que mergulha

**Branch:** `claude/tender-pasteur-85r9js`

## 1. Objetivo

O dono pediu: "quero que o goleiro pule na bola, faça todos os movimentos de um
goleiro real, se movimente como um goleiro profissional".

## 2. Diagnóstico

Gravei a disputa de pênaltis em vídeo. O goleiro **não pulava** em nenhuma
cobrança no canto. O motivo: o motor só começa uma animação de defesa se ela
consegue tocar a bola prevista. Um pênalti chega à linha em 0,30 a 0,35 s
(medido), e o mergulho lateral leva ~0,31 s para tocar a bola a ~1,9 m do
corpo. Reagindo só depois do chute, nunca dá tempo, então o goleiro ficava
parado. Goleiros reais resolvem isso escolhendo o lado no momento do chute.

## 3. O que mudou

| Mudança | Arquivos |
| --- | --- |
| Regras do mergulho sem gráficos: onde a bola cruza a linha, chance de ler o batedor (reação, agilidade, posicionamento contra a frieza do batedor), escolha do lado (lê, chuta um lado, ou fica no meio), alcance máximo, quando se esticar no jogo normal | `src/onthepitch/goalkeeperdive.hpp` (novo), 8 testes em `tests/onthepitch/goalkeeper_dive_test.cpp` |
| "Plano de mergulho" no jogador: a animação de defesa é escolhida contra o alvo planejado, mas o toque na bola usa a bola real (lado errado = gol) | `src/onthepitch/player/player.*`, `src/onthepitch/player/humanoid/humanoid.cpp` |
| Pênalti: goleiro em cima da linha; decide ~0,15 s antes do contato lendo a direção que o batedor quer chutar; no chute, ajusta o tempo à bola real (e o lugar, se tinha lido o batedor) | `src/onthepitch/referee.*`, `src/onthepitch/teamAIcontroller.cpp`, `src/onthepitch/player/controller/elizacontroller.*` |
| Durante o mergulho planejado, mãos e corpo cobrem uma área (60 cm) por alguns quadros, e o toque com o goleiro esticado desvia a bola com firmeza | `humanoid.cpp` |
| Jogo normal: chute no gol a mais de 1,3 m do goleiro e chegando em menos de 0,9 s → ele se estica em direção à bola | `elizacontroller.cpp` |
| "Defendeu!" na disputa, defesas contadas em `MatchData::GetPenaltySaves` | `referee.cpp`, `matchdata.hpp`, `data/locale/*.ini` |

### Arquivos fora da minha área

- `src/data/matchdata.hpp`: um contador novo (`AddPenaltySave` / `GetPenaltySaves`).

## 4. Verificação

| Verificação | Resultado |
| --- | --- |
| `ctest` | 344 de 344 |
| 3 disputas automáticas (34 cobranças) | 25 gols (74%), 6 defesas (18%), 3 para fora (9%). Referência real: ~75% gols, ~18-20% defesas |
| Todas as cobranças | O goleiro mergulha em todas, exceto quando "fica no meio" |
| Vídeo | Defesa do Szczęsny conferida quadro a quadro: mergulho lateral completo, bola desviada para fora, goleiro no chão e levantando |
| Partidas normais completas (5 min) | Passam, 0-0, sem erros |

**Não verificado:**
- O mergulho no jogo normal (goleiro se esticando num chute fora do alcance)
  **não foi exercitado**: em partidas CPU contra CPU de 5 e de 20 minutos não
  houve nenhum chute do adversário no gol que precisasse dele (todas 0-0;
  medi com um log temporário). O código usa o mesmo mecanismo verificado nos
  pênaltis, mas falta vê-lo numa partida. Sugestão: uma partida com um humano
  chutando de fora da área.
- Pênalti marcado durante o jogo (falta na área) usa o mesmo código, mas não
  ocorreu nos testes.
- Goleiro controlado por humano não existe no jogo (a troca de jogador pula
  o goleiro), então isso não se aplica.

## 5. Bug encontrado (não corrigido)

Numa das ~25 execuções de hoje, uma partida normal (sem disputa) caiu com
*segmentation fault* no instante em que a tela da partida abria (log:
"loadingmatchpage closed", "connecting signals"), antes de qualquer lance, e
portanto antes do código novo do goleiro agir. Rodava junto com outra instância
do jogo. Não reproduzi em mais 10 execuções (4 em sequência e 6 em pares, sob
`gdb`). O `docs/gap-list.md` já registra leituras sem sincronização entre as
threads do jogo e dos gráficos nessa área; suspeito de uma delas. Precisa de uma
rodada com ThreadSanitizer.

## 6. Limites honestos

O jogo tem só ~20 animações de goleiro: mergulhos laterais em 4 alturas,
encaixes e defesas altas. Não existem soco na bola, espalmar por cima do
travessão, saída nos pés do atacante nem queda e recuperação variadas. Esses
movimentos de um goleiro profissional exigem **novas animações** (captura de
movimento ou animação manual). O código novo já escolhe a animação pelo alvo,
então animações novas na pasta `data/media/animations/deflect` passam a ser
usadas sem mudar o código.

## 7. Próximo passo sugerido

- Goleiro: animações novas (soco, espalmar, saída do gol), se houver como
  produzi-las; e o goleiro se mexendo na linha (balanço) antes da cobrança.
- Seguir o roadmap: substituições.
