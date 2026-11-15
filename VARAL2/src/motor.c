#include "motor.h"
#include "driver/ledc.h"

#define SERVO_PIN 46
#define LEDC_TIMER LEDC_TIMER_0
#define LEDC_MODE LEDC_LOW_SPEED_MODE
#define LEDC_CHANNEL LEDC_CHANNEL_0
#define LEDC_DUTY_RES LEDC_TIMER_13_BIT
#define LEDC_FREQUENCY 50 // 50Hz padrão para servo motores

// Função auxiliar para converter ângulo em duty cycle do PWM (13 bits: 0 a 8191)
// Para servos padrão: ~0.5ms (0°) a ~2.5ms (180°) em um período de 20ms (50Hz)
static uint32_t angle_to_duty(int angle) {
    uint32_t duty = (uint32_t)(205 + ((float)angle / 180.0f) * (1024 - 205));
    return duty;
}

void motor_init(void) {
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_MODE,
        .timer_num = LEDC_TIMER,
        .duty_resolution = LEDC_DUTY_RES,
        .freq_hz = LEDC_FREQUENCY,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&ledc_timer);

    ledc_channel_config_t ledc_channel = {
        .channel = LEDC_CHANNEL,
        .duty = 0,
        .gpio_num = SERVO_PIN,
        .speed_mode = LEDC_MODE,
        .timer_sel = LEDC_TIMER,
        .hpoint = 0
    };
    ledc_channel_config(&ledc_channel);
}

void estender(void) {
    // 180 graus = Estendido
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, angle_to_duty(180));
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
}

void retrair(void) {
    // 0 graus = Retraído
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, angle_to_duty(0));
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);

    
}