# Divisão de trabalho

Proposta inicial. O dono do projeto pode mudar qualquer linha.

## Áreas

| Área | Pastas principais | Dono | Observação |
| --- | --- | --- | --- |
| Partida 3D: regras, árbitro, bola parada | `src/onthepitch/` (`referee.*`, `match.*`, `setpiecerules.hpp`) | **Claude** | |
| IA dos jogadores e táticas | `src/onthepitch/teamAIcontroller.*`, `AIsupport/`, `player/controller/` | **Claude** | |
| Física da bola e animação | `src/onthepitch/ball*`, `player/humanoid/` | **Claude** | |
| HUD e telas da partida (placar, pausa, fim de jogo) | `src/menu/ingame/` | **Claude** | Ligadas às regras da partida |
| Gráficos e shaders | `src/systems/graphics/`, `data/media/shaders/` | **Claude** | |
| Modo carreira | `src/menu/career/`, `src/data/career*` | **ChatGPT** | |
| Modo liga, calendário e competições | `src/league/`, `src/menu/league*` | **ChatGPT** | Copas usam a disputa de pênaltis do Claude |
| Menus fora da partida | `src/menu/` (exceto `ingame/` e `career/`) | **ChatGPT** | |
| Banco de dados e saves | `src/utils/database.*`, `data/databases/` | **ChatGPT** | |
| Áudio e narração | `src/systems/audio/`, `data/media/sounds/` | A definir | |
| Traduções | `data/locale/*.ini` | Compartilhado | Só adicionar linhas (veja o protocolo) |
| Build, CI e empacotamento | `CMakeLists.txt`, `sources.cmake`, `.github/`, `scripts/` | Compartilhado | Só adicionar linhas |
| Documentação de colaboração | `docs/colaboracao/` | Compartilhado | |

## Pontos de contato entre as áreas

Quando uma área depende da outra, a interface fica registrada aqui para ninguém
quebrar a outra sem perceber.

| Interface | Fornecido por | Usado por | Contrato |
| --- | --- | --- | --- |
| `MatchData::GetWinnerTeamID()`, `HasShootout()`, `GetShootoutGoals()` | Claude (partida) | ChatGPT (copas, liga, carreira) | Vencedor contando os pênaltis; o placar do jogo continua empate. |
| `ResolveKnockoutRules()` em `src/onthepitch/penaltyshootout.hpp` | Claude | ChatGPT | Hoje, jogos de liga e de carreira **ignoram** a opção de mata-mata. Para uma copa, adicione um parâmetro (ex.: "esta partida é eliminatória") em vez de mudar a opção global. |
| `LeagueHasPendingFixture()`, `CareerDatabase::HasPendingFixture()` | ChatGPT | Claude (árbitro) | Indicam se a partida 3D é de liga ou de carreira. |
| `MatchData::GetCorners/GetOffsides/GetYellowCards/GetRedCards` | Claude | ChatGPT (histórico, carreira) | Contados ao vivo na partida 3D. Ainda não são salvos no histórico de partidas. |

## Quadro de tarefas

### Em andamento

| Tarefa | IA | Branch | Arquivos / pastas | Desde |
| --- | --- | --- | --- | --- |
| _(vazio: reserve aqui antes de começar)_ | | | | |

### Próximas (sugestão, em ordem)

| # | Tarefa | Área | Sugerido para |
| --- | --- | --- | --- |
| 1 | Substituições: menu na pausa, limite de 5, IA substitui cansados/lesionados | Partida + HUD | Claude |
| 2 | Lesões que duram a partida inteira e obrigam substituição | Partida | Claude (depois do item 1) |
| 3 | Calendário de verdade (turno e returno) na carreira | Carreira | ChatGPT |
| 4 | Copa eliminatória na liga e na carreira, usando a disputa de pênaltis | Liga / carreira | ChatGPT |
| 5 | Acesso e rebaixamento aplicados de verdade no fim da temporada | Liga / carreira | ChatGPT |
| 6 | Bola parada com mira, força e efeito (falta, escanteio, pênalti) | Partida | Claude |
| 7 | Salvar escanteios, impedimentos, cartões e pênaltis no histórico de partidas | Banco de dados | ChatGPT |
| 8 | Estatística de defesas do goleiro | Partida | Claude |
| 9 | Chuva e vento visíveis, opção de clima no menu | Gráficos + menu | Claude + ChatGPT |
| 10 | Narração (frases gravadas ou geradas, ligadas aos eventos da partida) | Áudio | A definir |

### Concluído

| Data | Tarefa | IA | Branch | Relatório |
| --- | --- | --- | --- | --- |
| 2026-11-27 | Disputa de pênaltis 3D, opção de mata-mata no Quick Match, estatísticas de escanteios, impedimentos e cartões | Claude | `claude/tender-pasteur-85r9js` | [relatório](relatorios/2026-11-27-claude-penaltis-e-estatisticas.md) |
