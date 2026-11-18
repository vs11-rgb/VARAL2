#ifndef SENSORS_H
#define SENSORS_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

void sensores_iniciar(void);
void sensores_atualizar_maquina(QueueHandle_t fila_eventos);

#endif
