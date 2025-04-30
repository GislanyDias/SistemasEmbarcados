#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#define LED1_GPIO 7
#define LED2_GPIO 3


void app_main(void) {
  // Config como saidas
    gpio_set_direction(LED1_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(LED2_GPIO, GPIO_MODE_OUTPUT);

  // Estado dos leds
    int led1_state = 0;
    int led2_state = 0;

    // count ciclos de 200ms
    int count = 0;

    while (1) {
        // Pisca LED2 a cada 200ms
        led2_state = !led2_state;
        gpio_set_level(LED2_GPIO, led2_state);

        // LED1 a cada 1000ms - a cada 5 ciclos de 200ms
        if (count % 5 == 0) { //faz a divisao por ciclos de 200ms, 1s = 5 ciclos de 200ms
            led1_state = !led1_state;
            gpio_set_level(LED1_GPIO, led1_state);
        }

        count++;
        vTaskDelay(200 / portTICK_PERIOD_MS);
    }
}