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
static SemaphoreHandle_t xMutex = NULL;

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
   xLastWakeTime = xTaskGetTickCountFromISR();
   motor_init();
   while (1) {
        // Exemplo de acionamento baseado no evento atual
        // (Substitua pela sua lógica de leitura de evento segura com Mutex)
        
        
        // Se chover:
        // retrair();

        // Se o tempo estiver firme:
        // estender();

        vTaskDelayUntil(&xLastWakeTime, xFrequency);
   }

    

}



void app_main(void) {
  
}
