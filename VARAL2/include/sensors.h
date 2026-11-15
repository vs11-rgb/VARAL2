#ifndef SENSORS_H
#define SENSORS_H

#include "driver/gpio.h"
#include "fsm.h"
extern float curr_umidade;
void sensores_iniciar(void);
void sensores_atualizar_maquina(void);
static bool ler_dht22_umidade(float *umidade);

#endif
