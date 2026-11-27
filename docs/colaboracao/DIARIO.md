# Diário de bordo

Uma entrada por sessão, **a mais recente no topo**. Curta: o detalhe fica no
relatório da sessão.

Formato:

```
## AAAA-MM-DD — <IA> — <assunto>
- Branch: `...`
- Feito: ...
- Verificado: ...
- Pendente: ...
- Relatório: relatorios/<arquivo>.md
```

---

## 2026-11-27 — Claude — goleiro que mergulha
- Branch: `claude/tender-pasteur-85r9js`
- Feito: o goleiro escolhe um lado e mergulha nos pênaltis (lê o batedor com
  chance ligada aos atributos, às vezes chuta o lado, às vezes fica no meio),
  fica em cima da linha, se adianta ao chute e pode defender de verdade; no
  jogo normal se estica para chutes no gol fora do alcance. "Defendeu!" na
  tela e defesas contadas.
- Verificado: 344/344 testes; disputas automáticas com 74% de gols, 18% de
  defesas e 9% para fora (34 cobranças); vídeo de uma defesa conferido;
  partidas normais completas passam, mas uma caiu ao abrir a tela da partida
  (1 em ~25 execuções, não reproduzida depois; registrada no relatório).
- Pendente: só existem ~20 animações de goleiro (sem soco, sem espalmar por
  cima do travessão); o mergulho do jogo normal não foi exercitado (nenhum
  chute no gol fora do alcance nas partidas CPU contra CPU).
- Relatório: [relatorios/2026-11-27-claude-goleiro.md](relatorios/2026-11-27-claude-goleiro.md)

## 2026-11-27 — Claude — disputa de pênaltis, estatísticas e estrutura de colaboração
- Branch: `claude/tender-pasteur-85r9js`
- Feito: disputa de pênaltis 3D com regras da IFAB; opção de mata-mata no
  Quick Match; escanteios, impedimentos e cartões no fim de jogo; 12 testes
  unitários; teste de fumaça da disputa no CI; esta pasta de colaboração.
- Verificado: build Linux sem avisos novos; 336/336 testes; partida normal e
  partida com disputa automáticas; quadros capturados conferidos.
- Pendente: cobranças alternam de gol; nenhuma copa usa a disputa ainda;
  estatísticas novas fora do histórico; não testei um humano cobrando.
- Relatório: [relatorios/2026-11-27-claude-penaltis-e-estatisticas.md](relatorios/2026-11-27-claude-penaltis-e-estatisticas.md)
