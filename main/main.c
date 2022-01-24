#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lib/dht22/include/DHT.h"

void app_main(void) {
    setDHTgpio(4);

    while (1) {
        printf("Reading DHT...\n");
        
        int ret = readDHT();
        errorHandler(ret);
        printf("Hum: %.1f Tmp: %.1f\n", getHumidity(), getTemperature());

        vTaskDelay(3000 / portTICK_RATE_MS);
    }
}
