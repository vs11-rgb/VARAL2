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
    