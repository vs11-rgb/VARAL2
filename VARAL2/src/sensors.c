#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/semphr.h"
#include "fsm.h"
#include "sensors.h"

// Declaração de variáveis globais definidas no main.c
extern const char *TAG;
extern SemaphoreHandle_t xMutex;
extern Evento_t evento_atual;

#define PIN_LIMIT_1 GPIO_NUM_41

void vSensorTask(void *pvParameters) {
    int last_btn_state = -1;

    ESP_LOGI(TAG, "[SENSOR] Task de Sensores iniciada.");

    while (1) {
        int current_btn_state = gpio_get_level(PIN_LIMIT_1);

        // Detecta mudança de estado (transição)
        if (current_btn_state != last_btn_state) {
            last_btn_state = current_btn_state;

            if (current_btn_state == 0) {
                ESP_LOGI(TAG, "[SENSOR] Limit Switch Pressionado");

                if (xSemaphoreTake(xMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
                    evento_atual = LIMIT_SWITCH;
                    xSemaphoreGive(xMutex);
                }
            } else {
                ESP_LOGI(TAG, "[SENSOR] Limit Switch Solto.");
            }
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}