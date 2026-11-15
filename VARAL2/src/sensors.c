#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
<<<<<<< HEAD
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
=======
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "sensors.h"
#include "fsm.h"

#define PIN_SERVO       GPIO_NUM_46
#define PIN_CHUVA       GPIO_NUM_4 
#define PIN_LIMIT_1     GPIO_NUM_41
#define PIN_LIMIT_2     GPIO_NUM_42
#define PIN_DHT         GPIO_NUM_45
#define PIN_LED_R       GPIO_NUM_20
#define PIN_LED_G       GPIO_NUM_19
#define PIN_LED_B       GPIO_NUM_18
>>>>>>> eb31c1e (yep)

// Variáveis externas compartilhadas (vindas da main.c)
extern Evento_t evento_atual;
extern SemaphoreHandle_t mutexEvento;

<<<<<<< HEAD
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
=======
// Variáveis locais para armazenar o último estado de cada pino (útil para o debounce)
static int last_limit1_state = -1;
static int last_limit2_state = -1;
static int last_chuva_state = -1;

void sensores_iniciar(void) {
    gpio_config_t io_conf_in = {
        .pin_bit_mask = (1ULL << PIN_LIMIT_1) | (1ULL << PIN_LIMIT_2) | (1ULL << PIN_CHUVA) | (1ULL << PIN_DHT),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf_in);
    
    printf("[SENSORES] Pinos de entrada configurados com sucesso.\n");
}

void sensores_atualizar_maquina(void) {
    //Limit Switch 1
    int curr_limit1 = gpio_get_level(PIN_LIMIT_1);
    if (curr_limit1 != last_limit1_state){
        last_limit1_state = curr_limit1;
        if (curr_limit1 == 0){
            xSemaphoreTake(mutexEvento, portMAX_DELAY);
            evento_atual = LIMIT_SWITCH;
            xSemaphoreGive(mutexEvento);
            printf("[SENSORES] LIMIT SWITCH 1\n");

>>>>>>> eb31c1e (yep)
        }
    }
    
   int curr_limit2 = gpio_get_level(PIN_LIMIT_2);
    if (curr_limit2 != last_limit2_state){
        last_limit2_state = curr_limit2;
        if (curr_limit2 == 0){
            xSemaphoreTake(mutexEvento, portMAX_DELAY);
            evento_atual = LIMIT_SWITCH;
            xSemaphoreGive(mutexEvento);
            printf("[SENSORES] LIMIT SWITCH 2\n");
        }
    }
    
    int curr_chuva = gpio_get_level(PIN_CHUVA);
    if(curr_chuva != last_chuva_state){
        last_chuva_state = curr_chuva;

<<<<<<< HEAD
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
=======
        if(curr_chuva == 0){
            xSemaphoreTake(mutexEvento, portMAX_DELAY);
            evento_atual = MOLHADO;
            xSemaphoreGive(mutexEvento);
            printf("[SENSORES] MOLHADO\n");

        }
        else{
            xSemaphoreTake(mutexEvento, portMAX_DELAY);
            evento_atual = SECO;
            xSemaphoreGive(mutexEvento);
            printf("[SENSORES] SECO\n");
        }
    }
    }
>>>>>>> eb31c1e (yep)
