#include "esp_log.h"
#include <stdbool.h>
#include <stddef.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "fsm.h"
#include "motor.h"
#include "sensors.h"

#define TAMANHO_FILA_EVENTOS       10
#define PERIODO_SENSOR_MS          50
#define TEMPO_ESPERA_RETRAIDO_MS   3000
#define TEMPO_MOVIMENTO_MS         1500

static const char *TAG = "VARAL";

/* Este contexto e acessado somente pela atuador_task. */
static bool chuva_seca_atual = false;
static bool umidade_alta_atual = false;
static bool recolhimento_manual = false;

static const char *nome_estado(Estado_t estado)
{
    static const char *const nomes[NUM_ESTADOS] = {
        "RETRAIDO", "ESPERA", "ESTENDIDO", "ESTENDENDO", "RETRAINDO"
    };

    if ((int)estado < 0 || estado >= NUM_ESTADOS) {
        return "ESTADO_INVALIDO";
    }
    return nomes[estado];
}

static void vSensorTask(void *pvParameters)
{
    QueueHandle_t fila_eventos = (QueueHandle_t)pvParameters;
    const TickType_t periodo = pdMS_TO_TICKS(PERIODO_SENSOR_MS);
    TickType_t ultimo_wake = xTaskGetTickCount();

    for (;;) {
        sensores_atualizar_maquina(fila_eventos);
        vTaskDelayUntil(&ultimo_wake, periodo);
    }
}

static void executar_acao(Acao_t acao)
{
    switch (acao) {
        case A01:
            ESP_LOGI(TAG, "A01: iniciando/renovando espera para avaliar o ambiente.");
            break;

        case A02:
            estender();
            ESP_LOGI(TAG, "A02: servo comandado para estender.");
            break;

        case A03:
            parar();
            ESP_LOGI(TAG, "A03: extensao considerada concluida pelo temporizador.");
            break;

        case A04:
            retrair();
            ESP_LOGI(TAG, "A04: servo comandado para retrair.");
            break;

        case A05:
            parar();
            ESP_LOGI(TAG, "A05: retracao considerada concluida pelo temporizador.");
            break;

        case A06:
            retrair();
            ESP_LOGI(TAG, "A06: umidade alta; servo comandado para retrair.");
            break;

        case NENHUMA_ACAO:
        default:
            break;
    }
}

static Estado_t aplicar_evento_fsm(Estado_t estado, Evento_t evento)
{
    Acao_t acao = NENHUMA_ACAO;
    const Estado_t estado_anterior = estado;

    estado = fsm_process_event(estado, evento, &acao);

    ESP_LOGI(TAG, "Evento=%d | %s -> %s | acao=%d",
             (int)evento, nome_estado(estado_anterior), nome_estado(estado), (int)acao);

    executar_acao(acao);
    return estado;
}

/*
 * ESPERA e usada quando o varal esta recolhido, mas as condicoes ainda
 * nao permitem estender. A extensao automatica exige simultaneamente:
 * tempo seco, umidade abaixo do limiar e ausencia de bloqueio manual.
 */
static Estado_t tentar_extensao_automatica(Estado_t estado)
{
    if (estado == ESPERA && chuva_seca_atual &&
        !umidade_alta_atual && !recolhimento_manual) {
        return aplicar_evento_fsm(estado, SECO);
    }

    return estado;
}

static Estado_t processar_evento(Estado_t estado, Evento_t evento)
{
    /* Atualiza o retrato ambiental, mesmo se a FSM permanecer no mesmo estado. */
    switch (evento) {
        case SECO:
            chuva_seca_atual = true;
            break;
        case MOLHADO:
            chuva_seca_atual = false;
            break;
        case UMIDO:
            umidade_alta_atual = true;
            break;
        case UMIDADE_OK:
            umidade_alta_atual = false;
            break;
        default:
            break;
    }

    /*
     * Politica do botao mantida da versao-base:
     * - estendido/estendendo: recolhe e bloqueia a reextensao automatica;
     * - retraido/espera: libera o bloqueio e solicita extensao manual.
     * O comando manual e um override deliberado da condicao ambiental.
     */
    if (evento == BOTAO) {
        if (estado == ESTENDIDO || estado == ESTENDENDO) {
            recolhimento_manual = true;
        } else if (estado == RETRAIDO || estado == ESPERA) {
            recolhimento_manual = false;
        }
    }

    /* A matriz resolve a transicao correspondente ao evento recebido. */
    estado = aplicar_evento_fsm(estado, evento);

    /*
     * Corrige o caso que deixava a FSM presa em ESPERA:
     * cada TEMPO renova a avaliacao, e UMIDADE_OK tambem pode liberar a
     * extensao se o sensor de chuva ja estiver indicando tempo seco.
     */
    if (evento == TEMPO || evento == SECO || evento == UMIDADE_OK) {
        estado = tentar_extensao_automatica(estado);
    }

    return estado;
}

static void vAtuadorTask(void *pvParameters)
{
    QueueHandle_t fila_eventos = (QueueHandle_t)pvParameters;
    Estado_t estado = RETRAIDO;
    TickType_t inicio_estado = xTaskGetTickCount();

    ESP_LOGI(TAG, "FSM iniciada em %s.", nome_estado(estado));

    for (;;) {
        Evento_t evento = EVENTO_NENHUM;
        TickType_t espera = portMAX_DELAY;
        TickType_t limite_ms = 0;

        /*
         * RETRAIDO e ESPERA fazem verificacao periodica do ambiente.
         * Durante os movimentos, o temporizador representa o tempo de curso
         * esperado: sem fim de curso, ele gera MOVIMENTO_CONCLUIDO, nao falha.
         */
        if ((estado == RETRAIDO && !recolhimento_manual) || estado == ESPERA) {
            limite_ms = TEMPO_ESPERA_RETRAIDO_MS;
        } else if (estado == ESTENDENDO || estado == RETRAINDO) {
            limite_ms = TEMPO_MOVIMENTO_MS;
        }

        if (limite_ms > 0) {
            const TickType_t limite_ticks = pdMS_TO_TICKS(limite_ms);
            const TickType_t decorrido = xTaskGetTickCount() - inicio_estado;

            if (decorrido >= limite_ticks) {
                evento = ((estado == RETRAIDO) || (estado == ESPERA))
                             ? TEMPO
                             : MOVIMENTO_CONCLUIDO;
            } else {
                espera = limite_ticks - decorrido;
            }
        }

        if (evento == EVENTO_NENHUM &&
            xQueueReceive(fila_eventos, &evento, espera) != pdTRUE) {
            evento = ((estado == RETRAIDO) || (estado == ESPERA))
                         ? TEMPO
                         : MOVIMENTO_CONCLUIDO;
        }

        const Estado_t estado_anterior = estado;
        estado = processar_evento(estado, evento);

        /* Reinicia o relogio a cada transicao e a cada ciclo de avaliacao. */
        if (estado != estado_anterior || evento == TEMPO) {
            inicio_estado = xTaskGetTickCount();
        }
    }
}

void app_main(void)
{
    QueueHandle_t fila_eventos = xQueueCreate(TAMANHO_FILA_EVENTOS, sizeof(Evento_t));
    if (fila_eventos == NULL) {
        ESP_LOGE(TAG, "Nao foi possivel criar a fila de eventos.");
        return;
    }

    fsm_init();
    motor_init();
    sensores_iniciar();

    /* Cria primeiro o consumidor da fila para evitar acumulo sem consumidor. */
    const BaseType_t atuador_ok = xTaskCreate(
        vAtuadorTask, "atuador_task", 3072, fila_eventos, 2, NULL);

    if (atuador_ok != pdPASS) {
        ESP_LOGE(TAG, "Falha ao criar atuador_task.");
        vQueueDelete(fila_eventos);
        return;
    }

    const BaseType_t sensor_ok = xTaskCreate(
        vSensorTask, "sensor_task", 3072, fila_eventos, 2, NULL);

    if (sensor_ok != pdPASS) {
        ESP_LOGE(TAG, "Falha ao criar sensor_task; FSM ficara aguardando eventos.");
        /* A fila nao pode ser deletada porque atuador_task ja a utiliza. */
    }
}
