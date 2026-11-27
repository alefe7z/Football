# Estado atual do jogo

Atualizado em: **2026-11-27** por Claude.
Base: `master` em `7ca7adf` + branch `claude/tender-pasteur-85r9js`.

## Resumo

O jogo é o **League-Soccer / Gameplay Football**: um motor 3D próprio em C++17
(SDL2 + OpenGL + OpenAL + SQLite), com partida completa contra a CPU, árbitro,
física de bola, IA tática, replay, modo liga e modo carreira em cinco idiomas.
Ele compila e roda no Linux, macOS e Windows, com CI no GitHub.

## Números (medidos nesta sessão, Linux, build Release)

| Item | Valor |
| --- | --- |
| Código-fonte | 496 arquivos, ~89 mil linhas em `src/` |
| Compilação completa | ~2 min 45 s com 4 núcleos |
| Testes automáticos | **336 de 336 passando** (eram 324; 12 novos nesta sessão) |
| Partida completa automática (5 min de jogo, 6x mais rápida) | ~90 s, passa |
| Partida que termina em disputa de pênaltis | passa (ex.: Arsenal 0-0 Man. United, Arsenal vence nos pênaltis) |
| Avisos de compilação | 3, todos antigos (nenhum novo) |
| Idiomas | en, pt, es, fr, de, todos com todas as chaves |

## O que funciona

- **Partida 3D:** 11 contra 11, chute, passe, cruzamento, drible básico,
  carrinho, faltas com vantagem, impedimento, cartões, acréscimos, intervalo,
  replay de gols, comemorações, radar, painel de estatísticas e HUD de fôlego.
- **Mata-mata (novo):** no Quick Match, "Empate decidido por: Não / Pênaltis /
  Prorrogação + pênaltis". A disputa segue a regra da IFAB: 5 cobranças
  alternadas, fim antecipado quando um time não alcança mais o outro, morte
  súbita, melhores batedores primeiro e o goleiro por último, só cobrador e
  goleiro na área e os demais no círculo central. O placar mostra "PEN 4-3" e a
  tela final mostra quem venceu nos pênaltis.
- **Estatísticas (novo):** escanteios, impedimentos e cartões (amarelos e
  vermelhos) contados na partida e mostrados na tela de fim de jogo. Cobranças
  da disputa não contam como chutes.
- **Modos:** Quick Match, Liga, Carreira (Dono/Diretor, Técnico, Jogador),
  liga personalizada, histórico de partidas, controles remapeáveis (teclado e
  controle).

## O que falta (principais)

Lista completa e detalhada: [docs/gap-list.md](../gap-list.md). O mais importante:

| Prioridade | Falta | Área |
| --- | --- | --- |
| Alta | Substituições durante a partida (o banco existe, mas não é usado) | Partida |
| Alta | Lesões que duram a partida (hoje somem na próxima bola parada) | Partida |
| Alta | Calendário real, copas, acesso e rebaixamento na carreira e na liga | Carreira / liga |
| Alta | Bola parada com mira, força e efeito; cobrador escolhido | Partida |
| Média | Goleiro controlável; dribles (o drible lateral está desligado) | Partida |
| Média | Defesas do goleiro nas estatísticas; estatísticas novas no histórico | Partida / banco |
| Média | Chuva e vento visíveis; narração | Gráficos / áudio |

## Limitações conhecidas da disputa de pênaltis

- As cobranças alternam de gol (cada time chuta no gol do adversário), em vez
  de todas no mesmo gol como na regra oficial. Funciona e é justo, mas a câmera
  troca de lado a cada cobrança.
- Ainda não há uma competição de liga ou carreira que use a disputa: hoje só o
  Quick Match tem a opção.
- O jogador humano cobra com os controles normais de chute; ainda não existe
  uma mira específica para pênalti.
