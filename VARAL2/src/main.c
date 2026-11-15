#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "fsm.h"
#include "sensors.h"
#include "motor.h"

/*definição das portas*/
#define PIN_LIMIT_1     GPIO_NUM_42  // Conectado ao btn3
#define PIN_LIMIT_2     GPIO_NUM_41  // Conectado ao btn2
#define PIN_CHUVA       GPIO_NUM_0   // Conectado ao sw1 (Alavanca)
#define PIN_DHT         GPIO_NUM_45
#define PIN_SERVO       GPIO_NUM_18
#define PIN_LED_R       GPIO_NUM_19
#define PIN_LED_G       GPIO_NUM_22
#define PIN_LED_B       GPIO_NUM_23

static const char *TAG = "MAIN";

static Evento_t evento_atual = TEMPO;
static SemaphoreHandle_t mutexEvento = NULL;

void vSensorTask(void*pvParameters){
   const TickType_t xFrequency = pdMS_TO_TICKS(50); // Taxa de polling / debounce
    TickType_t xLastWakeTime = xTaskGetTickCount();

    while (1) {
        sensores_atualizar_maquina();
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

void vAtuadorTask(void*pvParameters){
    TickType_t xLastWakeTime;
    const TickType_t xFrequency = pdMS_TO_TICKS(100);
    xLastWakeTime = xTaskGetTickCount();
    motor_init();
    fsm_init();
    static bool estendido_state = false;

    Estado_t estado_fsm = RETRAIDO; 
    Acao_t acao_atual;

    while (1) {
        Evento_t evento_local = EVENTO_NENHUM;

        
        if (mutexEvento != NULL) {
            if (xSemaphoreTake(mutexEvento, pdMS_TO_TICKS(10)) == pdTRUE) {
                evento_local = evento_atual;
                evento_atual = EVENTO_NENHUM;
                xSemaphoreGive(mutexEvento);
                }
            }
        if (evento_local != EVENTO_NENHUM) {
            estado_fsm = fsm_process_event(estado_fsm, evento_local, &acao_atual);
            ESP_LOGI(TAG, "Evento: %d | Novo Estado: %d | Ação: %d", evento_local, estado_fsm, acao_atual);

            // Executa a ação determinada pela matriz
            switch (acao_atual) {
                case A01:
                    ESP_LOGI(TAG, "[ATUADOR] Ação A01: Iniciando espera...");
                    break;

                case A02:
                    estender();
                    ESP_LOGI(TAG, "[ATUADOR] Ação A02: Estendendo o varal...");
                    break;

                case A03:
                    ESP_LOGI(TAG, "[ATUADOR] Ação A03: Fim de curso atingido (Estendido).");
                    break;

                case A04:
                    retrair();
                    ESP_LOGI(TAG, "[ATUADOR] Ação A04: Recolhendo o varal (Chuva detectada)...");
                    break;

                case A05:
                    ESP_LOGI(TAG, "[ATUADOR] Ação A05: Fim de curso atingido (Retraído).");
                    break;

                case A06:
                    retrair();
                    ESP_LOGI(TAG, "[ATUADOR] Ação A06: Umidade alta na espera -> Recolhendo...");
                    break;

                case NENHUMA_ACAO:
                default:
                    break;
            }
        }
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
   }
}



void app_main(void) {
    mutexEvento = xSemaphoreCreateMutex();
    xTaskCreate(vSensorTask, "sensor task", 2048, NULL, 2, NULL);
    xTaskCreate(vAtuadorTask, "atuador task", 2048, NULL, 2, NULL);

}
