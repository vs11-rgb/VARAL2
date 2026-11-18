# Relatorio de validacao - VARAL2

## Arquivos comparados

- Base funcional/estrutura: `VARAL2_sem_fins_de_curso.zip`.
- Origem da implementacao do motor: `VARAL2_motor.zip`.

## Integracao realizada

- `src/motor.c`: substituido pelo arquivo do ZIP `VARAL2_motor`.
- `include/motor.h`: interface verificada; permaneceu compativel.
- `include/fsm.h` e `src/fsm.c`: matriz revisada para 5 estados e 7 eventos indexaveis, sem evento de fim de curso e sem estado `FALHA` por ausencia de feedback de posicao.
- `src/main.c`: temporizador de movimento agora emite `MOVIMENTO_CONCLUIDO`; estado `ESPERA` tem reavaliacao periodica e a extensao automatica respeita chuva, umidade e bloqueio manual.
- `src/sensors.c`: mantido sem leitura ou publicacao de fim de curso.
- `diagram.json`: mantido a partir do projeto sem fins de curso; conexoes conferidas com os pinos usados por sensores/motor.
- `platformio.ini`, `wokwi.toml`, CMakeLists e configuracoes do VS Code: preservados da base.

## Verificacoes executadas

- Compilacao host da FSM e testes com `gcc -std=c11 -Wall -Wextra -Werror`.
- Checagem sintatica de `main.c`, `fsm.c`, `motor.c` e `sensors.c` com GCC e cabecalhos simulados das APIs ESP-IDF/FreeRTOS, tambem com `-Wall -Wextra -Werror`.
- Testes das transicoes principais e da sentinela `EVENTO_NENHUM`.
- Inspecao do fluxo da task: `TEMPO` reavalia `RETRAIDO` e `ESPERA`; movimento termina por `MOVIMENTO_CONCLUIDO`.
- Verificacao de ausencia de `LIMIT_SWITCH`, `PIN_LIMIT`, GPIO de fim de curso e referencias operacionais a fim de curso no firmware/diagrama.
- JSON do `diagram.json` parseado e conexoes conferidas.
- Interface de `motor.h` comparada com funcoes exportadas em `motor.c`.
- Estrutura do projeto preservada; arquivos de build/cache `.pio` nao foram empacotados.

## Limites desta validacao

O executavel PlatformIO/ESP-IDF nao esta instalado neste ambiente. Portanto, nao foi possivel afirmar que `pio run` ou a simulacao Wokwi foram executados. O teste GCC da FSM valida a tabela em C no host, mas nao substitui o build completo para ESP32-S2.

## Ajuste de diagnostico do servo

- `motor.c` usa inicialmente 45 graus para recolher e 135 graus para estender, em vez de comandar os extremos de 0/180 graus.
- Cada comando registra angulo e duty cycle no log `MOTOR`.
- Essa calibracao e um ponto de partida, nao uma medicao do curso mecanico real. A simulacao/hardware deve confirmar que os dois alvos sao visualmente distintos e seguros.
