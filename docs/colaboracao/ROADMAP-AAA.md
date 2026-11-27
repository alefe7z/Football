# Roadmap para qualidade "AAA"

## Antes de tudo: o que "AAA" quer dizer aqui

Um jogo AAA de futebol é feito por centenas de pessoas ao longo de anos, com
captura de movimento, modelos 3D escaneados, licenças oficiais e narração
gravada em estúdio. Duas IAs escrevendo código não substituem isso, porque a
maior diferença é **arte e animação**, não programação.

O que dá para alcançar é um jogo que **se joga e se comporta como profissional**:
regras completas, IA convincente, modos de jogo profundos, apresentação de TV,
nenhum bug que trave e desempenho estável. E deixar o motor **pronto para
receber arte melhor** quando houver artistas ou ferramentas para isso.

Medimos cada pilar de 0 a 5 (0 = não existe, 3 = funciona mas é básico,
5 = nível de jogo comercial). As notas são uma avaliação honesta de 2026-11-27.

## Pilares

### 1. Regras e partida — nota 3,5 → meta 5 (só código)

| Item | Estado | Próximo passo |
| --- | --- | --- |
| Faltas, vantagem, cartões, impedimento, acréscimos | Feito | Ajuste fino com jogo real |
| Prorrogação e pênaltis | **Feito no Quick Match** (2026-11-27) | Ligar às copas da liga e da carreira |
| Substituições | Falta | Menu na pausa, limite de 5, IA troca cansados |
| Lesões | Esboço (somem na próxima bola parada) | Lesão até o fim do jogo, maca, troca obrigatória |
| Bola parada controlada | Falta | Mira, força e efeito em falta, escanteio e pênalti; cobrador escolhido |
| Mão na bola, VAR | Falta | Baixa prioridade |

### 2. IA e jogabilidade — nota 3 → meta 4,5 (só código)

| Item | Estado | Próximo passo |
| --- | --- | --- |
| IA tática (pressão, contra-ataque, papéis) | Feito | Testes automáticos de comportamento (posse, finalizações por partida) |
| Dribles e fintas | Básico (drible lateral desligado) | Reativar o drible lateral; adicionar 3 ou 4 fintas |
| Goleiro | IA ok; humano não controla | Troca para o goleiro; saídas e defesas mais variadas |
| Dificuldade | Multiplicadores | Diferenças de comportamento por dificuldade, não só de atributos |

### 3. Animação — nota 2 → meta 3,5 (precisa de dados)

As animações ficam em `data/media/animations` e são poucas. Movimentos novos exigem dados de
animação; código sozinho não cria isso. Caminhos possíveis: animações livres
(por exemplo, o formato do Mixamo), mistura de animações (*blending*) melhor e
cinemática inversa (IK) para os pés tocarem a bola com precisão.

### 4. Gráficos — nota 2,5 → meta 3,5 (código + arte)

| Item | Estado | Próximo passo (código) |
| --- | --- | --- |
| Iluminação | Sol aleatório, sombras | Iluminação por hora do dia, *tone mapping* |
| Gramado | Procedural | Desgaste da grama, faixas de corte, grama molhada |
| Clima | Só física (vento e chuva sem efeito visual) | Partículas de chuva, céu nublado |
| Torcida | Básica | Torcida animada por cartões de sprite |
| Modelos | Baixa contagem de polígonos | Precisa de arte; o código pode só suportar modelos melhores |

### 5. Apresentação — nota 2,5 → meta 4 (principalmente código)

Placar no estilo de TV (existe), entrada dos times, replays de lances perigosos
(o sinal existe, mas nunca é emitido), melhores momentos no fim, gráficos de
estatística no intervalo e uma câmera de transmissão com cortes automáticos.

### 6. Áudio e narração — nota 2 → meta 3,5

A torcida já reage a chances e gols. Falta narração: frases curtas ligadas aos
eventos (gol, defesa, cartão, pênalti), escolhidas por contexto. Precisa de
áudio gravado ou sintetizado com licença livre.

### 7. Modos de jogo — nota 3 → meta 4,5 (só código)

| Item | Estado | Próximo passo |
| --- | --- | --- |
| Carreira | Ampla, com vários sistemas decorativos | Calendário real, copas, acesso e rebaixamento, janela de transferências |
| Liga | Funciona | Copas eliminatórias com pênaltis |
| Escalação | Automática | Escolher titulares e banco, com efeito no resultado |

### 8. Estabilidade e desempenho — nota 4 → meta 5

336 testes, CI em 3 sistemas, verificação com AddressSanitizer e
ThreadSanitizer já feita. Falta: medir quadros por segundo no CI, testes de
fumaça para cada modo novo e partidas automáticas longas procurando travamentos.

### 9. Online — nota 0 → meta: decidir

Multijogador online é um projeto à parte (sincronização da física,
servidores). Recomendação: só depois dos pilares 1, 2 e 7.

## Ordem recomendada (próximos 3 ciclos)

**Ciclo 1, jogabilidade essencial:** substituições, lesões de verdade, bola
parada com mira (Claude). Calendário real e copas com pênaltis (ChatGPT).

**Ciclo 2, profundidade:** escalação e banco com efeito, janela de
transferências, acesso e rebaixamento (ChatGPT). Goleiro controlável, fintas,
estatística de defesas (Claude).

**Ciclo 3, apresentação:** replays de lances, entrada dos times, chuva
visível, iluminação por horário (Claude). Telas de estatística, histórico
completo e opção de clima no menu (ChatGPT).

## Como medir o progresso

- Cada item concluído atualiza a nota do pilar neste documento e o
  [ESTADO-ATUAL.md](ESTADO-ATUAL.md).
- Todo item de jogabilidade entra com teste unitário **e** um teste de
  fumaça no CI, como a disputa de pênaltis (`data/menu_smoke_shootout.config`).
