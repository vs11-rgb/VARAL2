#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "fsm.h"
#include "sensors.h"
#include "atuadores.h"

/*definição das portas*/
#define PIN_SERVO       GPIO_NUM_46
#define PIN_LIMIT_1     GPIO_NUM_41
#define PIN_LIMIT_2     GPIO_NUM_42
#define PIN_DHT         GPIO_NUM_45
#define PIN_LED_R       GPIO_NUM_20
#define PIN_LED_G       GPIO_NUM_19
#define PIN_LED_B       GPIO_NUM_18

static const char *TAG = "MAIN";

static Evento_t evento_atual = TEMPO;
static SemaphoreHandle_t xMutex = NULL;

void vSensorTask(void*pvParameters){
    int last_btn_state = -1;

    printf("[SENSOR] Task de Sensores iniciada.\n");

    while (1) {
        int current_btn_state = gpio_get_level(PIN_LIMIT_1);

        // Detecta se  transição (mudança de estado)
        if (current_btn_state != last_btn_state) {
            last_btn_state = current_btn_state;

            if (current_btn_state == 0) {
                ESP_LOGi(TAG,"[SENSOR] Limit Switch Pressionado");

                if (xSemaphoreTake(xMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
                    evento_atual = LIMIT_SWITCH;
                    xSemaphoreGive(xMutex);
                }
            } 
            else{
                ESP_LOGI(TAG,"[SENSOR] Limit Switch Solto.");
            }
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}



void app_main(void) {
   
    
}

void vSensorTask(void*pvParameters){


}
