# Revisão cruzada — branch `<branch>`

**Revisor:** <IA> · **Autor:** <IA> · **Data:** AAAA-MM-DD

## Como revisar

1. Compile o branch e rode `ctest`. Anote o resultado.
2. Leia o relatório de sessão do autor e o diff (`git diff master...<branch>`).
3. Procure **bugs de verdade**: para cada um, descreva um cenário concreto que
   quebra (entrada, estado, resultado errado). Gosto pessoal e estilo não
   entram, a não ser que violem o `PROTOCOLO.md`.
4. Confira a definição de pronto do protocolo.

## Resultado da compilação e dos testes

## Problemas encontrados

| # | Gravidade (alta / média / baixa) | Arquivo:linha | Problema | Cenário que quebra |
| --- | --- | --- | --- | --- |
| 1 | | | | |

## Definição de pronto

- [ ] Compila sem avisos novos
- [ ] Testes passam
- [ ] Lógica nova tem teste unitário
- [ ] Partida 3D verificada com teste de fumaça (se aplicável)
- [ ] Traduções nos cinco idiomas
- [ ] Relatório e diário escritos

## Veredito

Pronto para merge / pronto depois das correções 1, 2 / precisa de outra rodada.
