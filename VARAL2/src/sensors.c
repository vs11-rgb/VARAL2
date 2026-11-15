#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "sensors.h"
#include "fsm.h"

#define PIN_LIMIT_1     GPIO_NUM_42  // Conectado ao btn3
#define PIN_LIMIT_2     GPIO_NUM_41  // Conectado ao btn2
#define PIN_CHUVA       GPIO_NUM_0   // Conectado ao sw1 (Alavanca)
#define PIN_DHT         45
#define PIN_SERVO       GPIO_NUM_18
#define PIN_LED_R       GPIO_NUM_19
#define PIN_LED_G       GPIO_NUM_22
#define PIN_LED_B       GPIO_NUM_23

// Variáveis externas compartilhadas (vindas da main.c)
extern Evento_t evento_atual;
extern SemaphoreHandle_t mutexEvento;

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
