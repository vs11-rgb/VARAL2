# Varal automático - FSM temporizada e servo posicional

## Estrutura e responsabilidades

- `sensor_task` lê o seletor de chuva, o botão manual e o DHT22. Os eventos são enviados pela fila FreeRTOS.
- `atuador_task` e a única task que processa a FSM e comanda o servo.
- A fila e o mecanismo de comunicação/sincronização entre as tasks. Não há Mutex em torno de `xQueueSend()`/`xQueueReceive()`, pois a fila já e segura para uso entre tasks.
- `fsm.c` contem a matriz de transições. Eventos sem transição mantém o estado atual e não executam ação.
- `motor.c` concentra a configuração LEDC e os comandos de posição do servo.

## Correção da FSM

A versão anterior podia ficar presa no estado `ESPERA`: depois de avaliar um ambiente úmido/molhado, a task bloqueava indefinidamente aguardando outro evento, mesmo que a chuva parasse ou a umidade baixasse. Também faltava reavaliar as condições no próprio estado `ESPERA`.

Agora:

1. O estado inicial e `RETRAIDO`.
2. Após `TEMPO_ESPERA_RETRAIDO_MS`, a FSM entra em `ESPERA`.
3. `ESPERA` tem reavaliação periódica pelo evento `TEMPO`; enquanto não for seguro estender, permanece em `ESPERA` sem repetir comandos do motor.
4. A extensão automática só ocorre quando `chuva_seca_atual == true`, `umidade_alta_atual == false` e não existe bloqueio de recolhimento manual.
5. Um evento `UMIDADE_OK` também reavalia as condições. Assim, se já estiver seco e a umidade cair, a extensão pode iniciar sem depender de uma nova mudança no sensor de chuva.
6. Chuva ou umidade alta durante `ESTENDENDO`/`ESTENDIDO` iniciam a retração.
7. O botão mantém a política manual da versão-base: pressionado com o varal estendido/em extensão, recolhe e bloqueia reextensão automática; pressionado com o varal recolhido/em espera, libera o bloqueio e solicita extensão manual. O comando manual e um override deliberado das condições ambientais.

## Matriz de transições

| Estado atual | Evento | Próximo estado | Ação |
|---|---|---|---|
| `RETRAIDO` | `TEMPO` | `ESPERA` | `A01` |
| `RETRAIDO` | `BOTAO` | `ESTENDENDO` | `A02` |
| `ESPERA` | `TEMPO` | `ESPERA` | nenhuma |
| `ESPERA` | `SECO` | `ESTENDENDO` | `A02`* |
| `ESPERA` | `BOTAO` | `ESTENDENDO` | `A02` |
| `ESPERA` | `MOLHADO`, `UMIDO`, `UMIDADE_OK` | `ESPERA` | nenhuma |
| `ESTENDENDO` | `MOVIMENTO_CONCLUIDO` | `ESTENDIDO` | `A03` |
| `ESTENDENDO` | `MOLHADO` ou `BOTAO` | `RETRAINDO` | `A04` |
| `ESTENDENDO` | `UMIDO` | `RETRAINDO` | `A06` |
| `ESTENDIDO` | `MOLHADO` ou `BOTAO` | `RETRAINDO` | `A04` |
| `ESTENDIDO` | `UMIDO` | `RETRAINDO` | `A06` |
| `RETRAINDO` | `MOVIMENTO_CONCLUIDO` | `RETRAIDO` | `A05` |

`*` A transição da tabela `ESPERA + SECO` e condicionada no `main.c`: só e aplicada automaticamente se a umidade estiver abaixo do limiar e o bloqueio manual estiver desativado. Um comando `BOTAO` tem a política manual descrita acima.

`EVENTO_NENHUM` e somente uma sentinela (valor fora das 7 colunas da matriz); nunca deve ser publicado na fila nem passado como evento valido para a FSM.

## Movimento sem fins de curso

Não existe qualquer dependência de fim de curso. O movimento e em malha aberta: o firmware comanda uma posição do servo e presume que ele chegou ao destino quando termina o temporizador. *

- `TEMPO_MOVIMENTO_MS` e a duração assumida para extensão e retração (padrão: 1500 ms).
- Ao fim desse tempo, a task gera `MOVIMENTO_CONCLUIDO`. Esse evento conclui o movimento na matriz; não e um erro nem leva a um estado `FALHA`.
- Sem sensor de posição, o ESP32 não consegue detectar servo travado, perda de alimentação ou bloqueio mecânico. Ajuste `TEMPO_MOVIMENTO_MS` por ensaio real e considere proteção mecânica/elétrica adequada.

## Motor integrado de `VARAL2_motor`

Foi integrado `src/motor.c` do arquivo `VARAL2_motor.zip`: *

- Servo no GPIO21.
- PWM LEDC a 50 Hz, resolução de 13 bits.
- O motor registra no log cada comando, incluindo ângulo e duty cycle, para facilitar o diagnostico.
- `retrair()` comanda inicialmente 45 graus; `estender()` comanda inicialmente 135 graus. Esses valores evitam os extremos mecânicos e devem ser calibrados para o mecanismo real.
- `parar()` mantém o ultimo PWM, permitindo que o servo posicional mantenha o alvo.

O arquivo `include/motor.h` manteve a interface `motor_init()`, `estender()`, `retrair()` e `parar()`. *

## GPIOs usados

| Função | GPIO |
|---|---:|
| Botao manual | 2 |
| Seletor/sensor de chuva | 4 |
| DHT22 - dados | 5 |
| Servo - PWM | 21 |
| LED RGB - vermelho | 20 |
| LED RGB - verde | 19 |
| LED RGB - azul | 18 |

O diagrama Wokwi não possui chaves de fim de curso e não há GPIOs reservados para elas. * 

## Parâmetros

| Parametro | Valor | Funcao |
|---|---:|---|
| `PERIODO_SENSOR_MS` | 50 ms | Período de verificação do botão/seletor |
| `INTERVALO_DHT_MS` | 2500 ms | Intervalo mínimo entre leituras DHT22 |
| `LIMIAR_UMIDADE` | 70% | A partir desse valor, solicita retração |
| `TEMPO_ESPERA_RETRAIDO_MS` | 3000 ms | Intervalo de avaliação quando recolhido/em espera |
| `TEMPO_MOVIMENTO_MS` | 1500 ms | Tempo assumido de curso do servo |
| `TAMANHO_FILA_EVENTOS` | 10 | Capacidade da fila entre tasks |

## Testar a matriz FSM no computador (sem ESP32)

No diretório `VARAL2`, execute:

```sh
gcc -std=c11 -Wall -Wextra -Werror -Iinclude src/fsm.c test/fsm_host_test.c -o fsm_test
./fsm_test
```

O teste verifica as transições principais, os limites da matriz e o tratamento da sentinela `EVENTO_NENHUM`.

## Compilar e simular

No diretório `VARAL2`:

```sh
pio run -e esp32-s2-saola-1
```

O firmware e gerado em `.pio/build/esp32-s2-saola-1/firmware.elf`, conforme `wokwi.toml`.

## Observações *

- O DHT22 e lido por bit-banging. *
- O trilho de GND do servo e do ESP32 e comum. *
- Os GPIOs 18, 19 e 20 estao ligados ao LED RGB no diagrama, mas a logica do LED ainda nao esta implementada. *

## Calibracao inicial do servo *

Os alvos de teste estao em `src/motor.c`: `SERVO_ANGULO_RETRAIDO` (45 graus) e `SERVO_ANGULO_ESTENDIDO` (135 graus). Eles correspondem aproximadamente a pulsos de 1 ms e 2 ms com a conversao atual. Ajuste-os apos observar o movimento no Wokwi e garantir que a geometria do varal nao force o servo contra batentes. O log `MOTOR` deve mostrar cada chamada de extensao/retracao. *
