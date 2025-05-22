#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_timer.h"

#define btn1 GPIO_NUM_4
#define btn2 GPIO_NUM_1

#define LED0 GPIO_NUM_9
#define LED1 GPIO_NUM_11
#define LED2 GPIO_NUM_13
#define LED3 GPIO_NUM_14

#define DEBOUNCE_TIME_US 200000

uint8_t counter = 0;
uint8_t increment = 1;

int64_t last_press_a = 0;
int64_t last_press_b = 0;

volatile bool bnt_a_interrupt = false;
volatile bool bnt_b_interrupt = false;

static void IRAM_ATTR bnt_a_ISR_handler(void* arg) {
    gpio_intr_disable(btn1);
    bnt_a_interrupt = true;
}

static void IRAM_ATTR bnt_b_ISR_handler(void* arg) {
    gpio_intr_disable(btn2);
    bnt_b_interrupt = true;
}


void update_leds(uint8_t value) {
    gpio_set_level(LED0, value & 0x01);
    gpio_set_level(LED1, (value >> 1) & 0x01);
    gpio_set_level(LED2, (value >> 2) & 0x01);
    gpio_set_level(LED3, (value >> 3) & 0x01);
}

void init_gpio() {
    // LEDs como saída
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LED0) | (1ULL << LED1) | (1ULL << LED2) | (1ULL << LED3),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);

    // Botões como entrada com pull-down
    io_conf.pin_bit_mask = (1ULL << btn1) | (1ULL << btn2);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    gpio_config(&io_conf);

    // Configura interrupções
    gpio_set_intr_type(btn1, GPIO_INTR_NEGEDGE); 
    gpio_set_intr_type(btn2, GPIO_INTR_NEGEDGE); 
    gpio_install_isr_service(0);
    gpio_isr_handler_add(btn1, bnt_a_ISR_handler, NULL);
    gpio_isr_handler_add(btn2, bnt_b_ISR_handler, NULL);

}

void app_main() {
    init_gpio();
    update_leds(counter);

    last_press_a = esp_timer_get_time(); 
    last_press_b = esp_timer_get_time();
    printf(" %d \n", gpio_get_level(btn1));
    while (1) {
        int64_t now = esp_timer_get_time();


        if (bnt_a_interrupt) {
            if (now - last_press_a > DEBOUNCE_TIME_US) {
                last_press_a = now;
                counter = (counter + increment) & 0x0F;
                update_leds(counter);
                printf("Botão A pressionado. Contador = %d (0x%X)\n", counter, counter);
            }
            bnt_a_interrupt = false;
            gpio_intr_enable(btn1);
        }

        if (bnt_b_interrupt) {
            if (now - last_press_b > DEBOUNCE_TIME_US) {
                last_press_b = now;
                increment = (increment == 1) ? 2 : 1;
                printf("Botão B pressionado. Incremento = %d\n", increment);
            }
            bnt_b_interrupt = false;
            gpio_intr_enable(btn2);
        }

        vTaskDelay(pdMS_TO_TICKS(10)); 
    }
}