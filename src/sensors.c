
#include "sensors.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_rom_sys.h"
#include "esp_timer.h"
#include "freertos/task.h"
#include "fsm.h"

#define PIN_CHUVA   GPIO_NUM_4
#define PIN_BOTAO   GPIO_NUM_2
#define PIN_DHT     GPIO_NUM_5

#define LIMIAR_UMIDADE 70.0f
#define INTERVALO_DHT_MS 2500
#define TIMEOUT_ENVIO_FILA_MS 20

static int ultima_chuva = -1;
static int ultimo_botao = 1;

static bool estado_umido_conhecido = false;
static bool ultimo_estado_umido = false;
static bool estado_ambiente_inicial_publicado = false;

static TickType_t ultima_leitura_dht = 0;

static uint32_t total_eventos_descartados = 0;


/* =========================================================
   PUBLICACAO DE EVENTOS
   ========================================================= */

static void publicar_evento(QueueHandle_t fila_eventos, Evento_t evento)
{
    if (fila_eventos == NULL || evento == EVENTO_NENHUM) {
        return;
    }

    if (xQueueSend(fila_eventos, &evento,
                   pdMS_TO_TICKS(TIMEOUT_ENVIO_FILA_MS)) != pdTRUE) {

        total_eventos_descartados++;

        printf("[SENSORES][ERRO] Fila cheia: evento %d descartado (total=%lu).\n",
               (int)evento,
               (unsigned long)total_eventos_descartados);
    }
}


/* =========================================================
   INICIALIZACAO DOS SENSORES
   ========================================================= */

void sensores_iniciar(void)
{
    const gpio_config_t entradas = {
        .pin_bit_mask = (1ULL << PIN_CHUVA) |
                        (1ULL << PIN_BOTAO),

        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    ESP_ERROR_CHECK(gpio_config(&entradas));

    ESP_ERROR_CHECK(gpio_set_direction(PIN_DHT, GPIO_MODE_INPUT));

    printf("[SENSORES] Entradas de chuva e botao configuradas.\n");
}


/* =========================================================
   FUNCOES AUXILIARES DO DHT22
   ========================================================= */

/* Aguarda determinado nivel logico com timeout em microssegundos. */

static bool aguardar_nivel_dht(int nivel, int64_t timeout_us)
{
    const int64_t inicio = esp_timer_get_time();

    while (gpio_get_level(PIN_DHT) != nivel) {

        if (esp_timer_get_time() - inicio >= timeout_us) {
            return false;
        }
    }

    return true;
}


/* =========================================================
   LEITURA DO DHT22
   ========================================================= */

static bool ler_dht22_umidade(float *umidade)
{
    if (umidade == NULL) {
        return false;
    }

    uint8_t dados[5] = {0};


    /* 1. Sinal de inicializacao enviado pelo ESP32 */

    ESP_ERROR_CHECK(gpio_set_direction(PIN_DHT, GPIO_MODE_OUTPUT));

    gpio_set_level(PIN_DHT, 0);
    esp_rom_delay_us(2000);

    gpio_set_level(PIN_DHT, 1);
    esp_rom_delay_us(30);

    ESP_ERROR_CHECK(gpio_set_direction(PIN_DHT, GPIO_MODE_INPUT));


    /* 2. Resposta inicial do sensor */

    if (!aguardar_nivel_dht(0, 100) ||
        !aguardar_nivel_dht(1, 100) ||
        !aguardar_nivel_dht(0, 100)) {

        return false;
    }


    /* 3. Leitura dos 40 bits transmitidos pelo sensor */

    for (int bit = 0; bit < 40; bit++) {

        /* Aguarda o inicio do pulso alto */

        if (!aguardar_nivel_dht(1, 100)) {
            return false;
        }

        /* Registra o instante de inicio do pulso */

        const int64_t inicio = esp_timer_get_time();

        /* Aguarda o termino do pulso alto */

        if (!aguardar_nivel_dht(0, 100)) {
            return false;
        }

        /* Calcula a duracao do pulso em microssegundos */

        const int64_t duracao = esp_timer_get_time() - inicio;

        /* Desloca os bits anteriores para a esquerda */

        dados[bit / 8] <<= 1;

        /* Pulso maior que 40 us representa bit 1 */

        if (duracao > 40) {
            dados[bit / 8] |= 1;
        }
    }


    /* 4. Verificacao do checksum */

    if ((uint8_t)(dados[0] + dados[1] +
                  dados[2] + dados[3]) != dados[4]) {

        return false;
    }


    /* 5. Conversao dos dados para umidade percentual */

    *umidade = (float)(((uint16_t)dados[0] << 8) | dados[1]) / 10.0f;

    return true;
}


/* =========================================================
   ATUALIZACAO DOS SENSORES E PUBLICACAO DE EVENTOS
   ========================================================= */

void sensores_atualizar_maquina(QueueHandle_t fila_eventos)
{
    /* -----------------------------------------------------
       BOTAO MANUAL
       Alterna a acao dependendo do estado atual da FSM.
       ----------------------------------------------------- */

    const int botao = gpio_get_level(PIN_BOTAO);

    if (botao != ultimo_botao) {

        ultimo_botao = botao;

        if (botao == 0) {

            publicar_evento(fila_eventos, BOTAO);

            printf("[SENSORES] Botao manual pressionado.\n");
        }
    }


    /* -----------------------------------------------------
       SENSOR DE CHUVA
       Nivel baixo = molhado.
       Nivel alto = seco.
       ----------------------------------------------------- */

    const int chuva = gpio_get_level(PIN_CHUVA);

    bool chuva_mudou = false;

    if (ultima_chuva < 0) {

        ultima_chuva = chuva;

    } else if (chuva != ultima_chuva) {

        ultima_chuva = chuva;
        chuva_mudou = true;
    }

    if (chuva_mudou && estado_ambiente_inicial_publicado) {

        publicar_evento(fila_eventos,
                        (chuva == 0) ? MOLHADO : SECO);

        printf("[SENSORES] Estado da chuva: %s.\n",
               (chuva == 0) ? "molhado" : "seco");
    }


    /* -----------------------------------------------------
       SENSOR DE UMIDADE DHT22
       Intervalo minimo de 2,5 segundos entre leituras.
       ----------------------------------------------------- */

    const TickType_t agora = xTaskGetTickCount();

    if ((agora - ultima_leitura_dht) >=
        pdMS_TO_TICKS(INTERVALO_DHT_MS)) {

        ultima_leitura_dht = agora;

        float umidade = 0.0f;

        if (ler_dht22_umidade(&umidade)) {

            const bool esta_umido = (umidade >= LIMIAR_UMIDADE);

            /* Publica evento somente quando o estado muda */

            if (!estado_umido_conhecido ||
                esta_umido != ultimo_estado_umido) {

                const bool ja_tinha_leitura = estado_umido_conhecido;

                estado_umido_conhecido = true;
                ultimo_estado_umido = esta_umido;

                publicar_evento(
                    fila_eventos,
                    esta_umido ? UMIDO :
                    (ja_tinha_leitura ? UMIDADE_OK : EVENTO_NENHUM)
                );
            }


            /* Publica o estado inicial combinado do ambiente */

            if (!estado_ambiente_inicial_publicado) {

                estado_ambiente_inicial_publicado = true;

                publicar_evento(
                    fila_eventos,
                    (ultima_chuva == 0) ? MOLHADO : SECO
                );
            }


            printf("[SENSORES] Umidade: %.1f%%\n", umidade);

        } else {

            printf("[SENSORES] Falha na leitura do DHT22.\n");
        }
    }
}
