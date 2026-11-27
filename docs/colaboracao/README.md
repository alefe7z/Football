# Colaboração Claude + ChatGPT

Esta pasta organiza o trabalho de duas IAs (Claude e ChatGPT) no mesmo jogo,
sob a direção do dono do projeto. O objetivo é que as duas avancem em paralelo
**sem apagar o trabalho uma da outra** e que o dono saiba sempre o que mudou.

## Documentos

| Documento | Para que serve | Quem atualiza |
| --- | --- | --- |
| [PROTOCOLO.md](PROTOCOLO.md) | As regras: branches, quem mexe em quê, testes obrigatórios, como entregar | Só o dono muda as regras |
| [DIVISAO-DE-TRABALHO.md](DIVISAO-DE-TRABALHO.md) | Áreas de cada IA e o quadro de "quem está mexendo em quê agora" | Cada IA, ao começar e terminar uma tarefa |
| [ESTADO-ATUAL.md](ESTADO-ATUAL.md) | Relatório do estado do jogo: o que funciona, o que falta, números | Quem fechar uma sessão |
| [ROADMAP-AAA.md](ROADMAP-AAA.md) | O caminho até um jogo com qualidade "AAA", por pilar, com prioridades | Discutido entre os três; o dono aprova |
| [DIARIO.md](DIARIO.md) | Registro cronológico de cada sessão (uma entrada curta por sessão) | Cada IA, ao final de cada sessão |
| [PROMPT-PARA-CHATGPT.md](PROMPT-PARA-CHATGPT.md) | Texto pronto para colar no ChatGPT antes de ele começar | O dono |
| [relatorios/](relatorios/) | Relatórios completos de cada sessão | Cada IA |
| [modelos/](modelos/) | Modelos de relatório de sessão, bug, proposta e revisão cruzada | Ninguém (são modelos) |

## Fluxo em uma frase

Ler o `PROTOCOLO` → marcar a tarefa no quadro da `DIVISAO-DE-TRABALHO` → trabalhar
no seu próprio branch → compilar e rodar os testes → escrever o relatório e uma
linha no `DIARIO` → o dono revisa e faz o merge.

## Para o dono do projeto

- Você é o único que junta trabalho no `master`. As IAs entregam em branches.
- Se as duas pedirem a mesma área, decida no quadro da divisão de trabalho.
- Peça a uma IA para **revisar** o branch da outra antes do merge (modelo em
  [modelos/REVISAO-CRUZADA.md](modelos/REVISAO-CRUZADA.md)). Uma pega erros da outra.
