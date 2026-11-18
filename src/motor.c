#include "motor.h"

#include <stdint.h>

#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_err.h"
#include "esp_log.h"

/* GPIO46 do ESP32-S2 e somente entrada. Use GPIO21 para o sinal do servo. */
#define SERVO_PIN       GPIO_NUM_21
#define LEDC_TIMER_ID   LEDC_TIMER_0
#define LEDC_MODE_ID    LEDC_LOW_SPEED_MODE
#define LEDC_CHANNEL_ID LEDC_CHANNEL_0
#define LEDC_DUTY_RES   LEDC_TIMER_13_BIT
#define LEDC_FREQUENCY  50

/*
 * Posicoes iniciais conservadoras para o servo.
 * 45 graus ~= pulso de 1,0 ms; 135 graus ~= pulso de 2,0 ms.
 * Evitamos os extremos de 0,5 ms e 2,5 ms ate calibrar o mecanismo.
 */
#define SERVO_ANGULO_RETRAIDO  45
#define SERVO_ANGULO_ESTENDIDO 135

static const char *TAG = "MOTOR";

static uint32_t angle_to_duty(int angle)
{
    if (angle < 0) angle = 0;
    if (angle > 180) angle = 180;

    /* 0,5 ms a 2,5 ms em um periodo de 20 ms, resolucao de 13 bits. */
    return (uint32_t)(205 + ((float)angle / 180.0f) * (1024 - 205));
}

void motor_init(void)
{
    const ledc_timer_config_t timer_config = {
        .speed_mode = LEDC_MODE_ID,
        .timer_num = LEDC_TIMER_ID,
        .duty_resolution = LEDC_DUTY_RES,
        .freq_hz = LEDC_FREQUENCY,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer_config));

    const ledc_channel_config_t channel_config = {
        .channel = LEDC_CHANNEL_ID,
        .duty = angle_to_duty(SERVO_ANGULO_RETRAIDO),
        .gpio_num = SERVO_PIN,
        .speed_mode = LEDC_MODE_ID,
        .timer_sel = LEDC_TIMER_ID,
        .hpoint = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&channel_config));

    ESP_LOGI(TAG, "PWM inicializado no GPIO%d; posicao inicial: %d graus.",
             (int)SERVO_PIN, SERVO_ANGULO_RETRAIDO);
}

static void posicionar_servo(int angulo, const char *descricao)
{
    const uint32_t duty = angle_to_duty(angulo);

    ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE_ID, LEDC_CHANNEL_ID, duty));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE_ID, LEDC_CHANNEL_ID));

    ESP_LOGI(TAG, "%s: %d graus (duty=%lu).",
             descricao, angulo, (unsigned long)duty);
}

void estender(void)
{
    posicionar_servo(SERVO_ANGULO_ESTENDIDO, "Estender");
}

void retrair(void)
{
    posicionar_servo(SERVO_ANGULO_RETRAIDO, "Retrair");
}

void parar(void)
{
    /* Servo posicional: manter o ultimo PWM mantem o eixo na posicao-alvo.
       Desligar o PWM aqui pode fazer o varal perder torque/posicao. */
}
