#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "driver/gpio.h"
#include "driver/adc.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "driver/ledc.h"
#include "int_i2c.h"
#include "esp_log.h"
#include "sdmmc_cmd.h"
#include "driver/sdspi_host.h"
#include "esp_vfs_fat.h"

#define BOTAO_CIMA GPIO_NUM_4
#define BOTAO_BAIXO GPIO_NUM_1
#define BUZZER GPIO_NUM_37

#define DISPLAY_SEG_A GPIO_NUM_18
#define DISPLAY_SEG_B GPIO_NUM_9
#define DISPLAY_SEG_C GPIO_NUM_8
#define DISPLAY_SEG_D GPIO_NUM_3
#define DISPLAY_SEG_E GPIO_NUM_14
#define DISPLAY_SEG_G GPIO_NUM_16
#define DISPLAY_SEG_F GPIO_NUM_17

#define SENSOR_TEMP 1
#define CANAL_ADC ADC1_CHANNEL_6

#define BETA_TERMISTOR 3950.0
#define RESOLUCAO_ADC 4095.0
#define TEMP_REF_K 298.15

#define I2C_SDA GPIO_NUM_48
#define I2C_SCL GPIO_NUM_47

#define SD_MISO GPIO_NUM_12
#define SD_MOSI GPIO_NUM_11
#define SD_CLK GPIO_NUM_13
#define SD_CS GPIO_NUM_10
#define PONTO_MONTAGEM "/sdcard"
#define ARQUIVO_LOG PONTO_MONTAGEM"/temp_log.csv"

#define TEMP_MINIMA 10
#define TEMP_MAXIMA 50
#define INTERVALO_LOG 10000

#define NUMERO_I2C I2C_NUM_0
#define FREQ_I2C 100000
#define ENDERECO_LCD 0x27
#define TAMANHO_LCD DISPLAY_16X02

static const char *TAG_SISTEMA = "MONITOR_TEMP";

SemaphoreHandle_t mutex_temp;
int limite_temp = 25;
int temp_atual = 20;
bool alarme_ativado = false;
bool sd_pronto = false;
lcd_i2c_handle_t display_lcd;

QueueHandle_t fila_botoes;

static void IRAM_ATTR isr_botao_cima(void* arg) {
    int id_botao = 1;
    xQueueSendFromISR(fila_botoes, &id_botao, NULL);
}

static void IRAM_ATTR isr_botao_baixo(void* arg) {
    int id_botao = 2;
    xQueueSendFromISR(fila_botoes, &id_botao, NULL);
}

void mostrar_digito(char digito);

void configurar_gpios() {
    gpio_config_t config_display = {
        .pin_bit_mask = (1ULL << DISPLAY_SEG_A) | (1ULL << DISPLAY_SEG_B) | (1ULL << DISPLAY_SEG_C) |
                        (1ULL << DISPLAY_SEG_D) | (1ULL << DISPLAY_SEG_E) | (1ULL << DISPLAY_SEG_F) |
                        (1ULL << DISPLAY_SEG_G),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&config_display);

    gpio_config_t config_botoes = {
        .pin_bit_mask = (1ULL << BOTAO_CIMA) | (1ULL << BOTAO_BAIXO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE
    };
    gpio_config(&config_botoes);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(BOTAO_CIMA, isr_botao_cima, NULL);
    gpio_isr_handler_add(BOTAO_BAIXO, isr_botao_baixo, NULL);
    
    mostrar_digito(' ');
}

void configurar_pwm() {
    ledc_timer_config_t timer_pwm = {
        .duty_resolution = LEDC_TIMER_8_BIT,
        .freq_hz = 2000,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&timer_pwm);

    ledc_channel_config_t canal_pwm = {
        .channel = LEDC_CHANNEL_0,
        .duty = 0,
        .gpio_num = BUZZER,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .hpoint = 0,
        .timer_sel = LEDC_TIMER_0,
        .intr_type = LEDC_INTR_DISABLE
    };
    ledc_channel_config(&canal_pwm);
}

void configurar_i2c() {
    i2c_config_t conf_i2c = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_SDA,
        .scl_io_num = I2C_SCL,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = FREQ_I2C,
    };
    i2c_param_config(NUMERO_I2C, &conf_i2c);
    i2c_driver_install(NUMERO_I2C, conf_i2c.mode, 0, 0, 0);
}

void configurar_adc() {
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(CANAL_ADC, ADC_ATTEN_DB_12);
}

float ler_temperatura() {
    int valor_adc = adc1_get_raw(CANAL_ADC);
    const float BETA = 3950;
    float resistencia = 10000.0 / ((RESOLUCAO_ADC / (float)valor_adc) - 1.0);
    float temp_k = 1 / (log(resistencia / 10000.0) / BETA + 1.0 / 298.15) - 273.15;
    return temp_k;


}

void mostrar_digito(char digito) {
    gpio_set_level(DISPLAY_SEG_A, 1);
    gpio_set_level(DISPLAY_SEG_B, 1);
    gpio_set_level(DISPLAY_SEG_C, 1);
    gpio_set_level(DISPLAY_SEG_D, 1);
    gpio_set_level(DISPLAY_SEG_E, 1);
    gpio_set_level(DISPLAY_SEG_F, 1);
    gpio_set_level(DISPLAY_SEG_G, 1);

    switch (digito) {
        case '0':
            gpio_set_level(DISPLAY_SEG_A, 0);
            gpio_set_level(DISPLAY_SEG_B, 0);
            gpio_set_level(DISPLAY_SEG_C, 0);
            gpio_set_level(DISPLAY_SEG_D, 0);
            gpio_set_level(DISPLAY_SEG_E, 0);
            gpio_set_level(DISPLAY_SEG_F, 0);
            break;
            
        case '3':
            gpio_set_level(DISPLAY_SEG_A, 0);
            gpio_set_level(DISPLAY_SEG_B, 0);
            gpio_set_level(DISPLAY_SEG_C, 0);
            gpio_set_level(DISPLAY_SEG_D, 0);
            gpio_set_level(DISPLAY_SEG_G, 0);
            break;
            
        case '7':
            gpio_set_level(DISPLAY_SEG_A, 0);
            gpio_set_level(DISPLAY_SEG_B, 0);
            gpio_set_level(DISPLAY_SEG_C, 0);
            break;
            
        case 'D':
            gpio_set_level(DISPLAY_SEG_B, 0);
            gpio_set_level(DISPLAY_SEG_C, 0);
            gpio_set_level(DISPLAY_SEG_D, 0);
            gpio_set_level(DISPLAY_SEG_E, 0);
            gpio_set_level(DISPLAY_SEG_G, 0);
            break;
            
        case 'F':
            gpio_set_level(DISPLAY_SEG_A, 0);
            gpio_set_level(DISPLAY_SEG_E, 0);
            gpio_set_level(DISPLAY_SEG_F, 0);
            gpio_set_level(DISPLAY_SEG_G, 0);
            break;
            
        default:
            break;
    }
}

bool inicializar_sd() {
    esp_vfs_fat_sdmmc_mount_config_t config_montagem = {
        .format_if_mount_failed = true,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };
    
    sdmmc_card_t *cartao;
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    
    spi_bus_config_t config_spi = {
        .mosi_io_num = SD_MOSI,
        .miso_io_num = SD_MISO,
        .sclk_io_num = SD_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,
    };
    
    esp_err_t ret = spi_bus_initialize(host.slot, &config_spi, SDSPI_DEFAULT_DMA);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG_SISTEMA, "Erro SPI: 0x%x", ret);
        return false;
    }

    sdspi_device_config_t config_sd = SDSPI_DEVICE_CONFIG_DEFAULT();
    config_sd.gpio_cs = SD_CS;
    config_sd.host_id = host.slot;

    ret = esp_vfs_fat_sdspi_mount(PONTO_MONTAGEM, &host, &config_sd, &config_montagem, &cartao);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG_SISTEMA, "Erro montagem: 0x%x", ret);
        return false;
    }
    
    struct stat st;
    if (stat(ARQUIVO_LOG, &st) == -1) {
        FILE *arquivo = fopen(ARQUIVO_LOG, "w");
        if (arquivo) {
            fclose(arquivo);
        }
    }
    return true;
}

void escrever_log() {
    time_t agora;
    struct tm timeinfo;
    time(&agora);
    localtime_r(&agora, &timeinfo);

    FILE *arquivo = fopen(ARQUIVO_LOG, "a");
    if (arquivo == NULL) {
        ESP_LOGE(TAG_SISTEMA, "Erro arquivo");
        return;
    }

    fprintf(arquivo, "%04d-%02d-%02d,%02d:%02d:%02d,%d,%d,%s\n",
            timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
            timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec,
            temp_atual, limite_temp, alarme_ativado ? "LIGADO" : "DESLIGADO");
    
    fclose(arquivo);
    ESP_LOGI(TAG_SISTEMA, "Dados registrados");
}

void tarefa_ler_temperatura(void *pvParam) {
    while(1) {
        float temp = ler_temperatura();
        
        if (xSemaphoreTake(mutex_temp, portMAX_DELAY)) {
            temp_atual = (int)temp;
            alarme_ativado = (temp_atual >= limite_temp);
            xSemaphoreGive(mutex_temp);
        }
        
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

bool botao_cima_pressionado = false;
bool botao_baixo_pressionado = false;

static void IRAM_ATTR isr_local_botao_cima(void* arg) {
    gpio_intr_disable(BOTAO_CIMA);
    botao_cima_pressionado = true;
}

static void IRAM_ATTR isr_local_botao_baixo(void* arg) {
    gpio_intr_disable(BOTAO_BAIXO);
    botao_baixo_pressionado = true;
}

void tarefa_tratar_botoes(void *pvParam) {
    uint8_t estado_anterior_cima = 1;
    uint8_t estado_anterior_baixo = 1;
    TickType_t ultima_vez_cima = 0;
    TickType_t ultima_vez_baixo = 0;
    const TickType_t tempo_debounce = pdMS_TO_TICKS(50);
    
    gpio_isr_handler_remove(BOTAO_CIMA);
    gpio_isr_handler_remove(BOTAO_BAIXO);
    gpio_isr_handler_add(BOTAO_CIMA, isr_local_botao_cima, NULL);
    gpio_isr_handler_add(BOTAO_BAIXO, isr_local_botao_baixo, NULL);

    while(1) {
        if (botao_cima_pressionado) {
            botao_cima_pressionado = false;
            uint8_t estado_atual = gpio_get_level(BOTAO_CIMA);
            TickType_t agora = xTaskGetTickCount();

            if (estado_atual == 0 && estado_anterior_cima == 1) {
                if ((agora - ultima_vez_cima) >= tempo_debounce) {
                    if (xSemaphoreTake(mutex_temp, portMAX_DELAY)) {
                        if (limite_temp < TEMP_MAXIMA) {
                            limite_temp += 5;
                            ESP_LOGI(TAG_SISTEMA, "Novo limite: %d", limite_temp);
                        }
                        xSemaphoreGive(mutex_temp);
                    }
                    ultima_vez_cima = agora;
                }
            }
            estado_anterior_cima = estado_atual;
            gpio_intr_enable(BOTAO_CIMA);
        }

        if (botao_baixo_pressionado) {
            botao_baixo_pressionado = false;
            uint8_t estado_atual = gpio_get_level(BOTAO_BAIXO);
            TickType_t agora = xTaskGetTickCount();

            if (estado_atual == 0 && estado_anterior_baixo == 1) {
                if ((agora - ultima_vez_baixo) >= tempo_debounce) {
                    if (xSemaphoreTake(mutex_temp, portMAX_DELAY)) {
                        if (limite_temp > TEMP_MINIMA) {
                            limite_temp -= 5;
                            ESP_LOGI(TAG_SISTEMA, "Novo limite: %d", limite_temp);
                        }
                        xSemaphoreGive(mutex_temp);
                    }
                    ultima_vez_baixo = agora;
                }
            }
            estado_anterior_baixo = estado_atual;
            gpio_intr_enable(BOTAO_BAIXO);
        }
        
        vTaskDelay(pdMS_TO_TICKS(10)); 
    }
}

void tarefa_acionar_alarme(void *pvParam) {
    while(1) {
        bool alarme;
        
        if (xSemaphoreTake(mutex_temp, pdMS_TO_TICKS(100))) {
            alarme = alarme_ativado;
            xSemaphoreGive(mutex_temp);
        } else {
            continue;
        }
        
        if (alarme) {
            ledc_set_freq(LEDC_LOW_SPEED_MODE, LEDC_TIMER_0, 2500);
            ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 128);
            ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
            vTaskDelay(pdMS_TO_TICKS(100));
            ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);
            ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
            vTaskDelay(pdMS_TO_TICKS(100));
        } else {
            ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);
            ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
            vTaskDelay(pdMS_TO_TICKS(200));
        }
    }
}

void tarefa_atualizar_lcd(void *pvParam) {
    char buffer[20];
    while(1) {
        int temp_medida, temp_limite;
        
        if (xSemaphoreTake(mutex_temp, pdMS_TO_TICKS(100))) {
            temp_medida = temp_atual;
            temp_limite = limite_temp;
            xSemaphoreGive(mutex_temp);
        } else {
            continue;
        }
        
        lcd_i2c_cursor_set(&display_lcd, 0, 0);
        snprintf(buffer, sizeof(buffer), "Temp: %d C", temp_medida);
        lcd_i2c_print(&display_lcd, buffer);
        
        lcd_i2c_cursor_set(&display_lcd, 0, 1);
        snprintf(buffer, sizeof(buffer), "Limite: %d C", temp_limite);
        lcd_i2c_print(&display_lcd, buffer);
        
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void tarefa_atualizar_display(void *pvParam) {
    char digito = ' ';
    uint64_t ultimo_piscar = 0;
    bool display_ligado = true;
    
    while(1) {
        int temp_medida, temp_limite;
        bool alarme;
        
        if (xSemaphoreTake(mutex_temp, pdMS_TO_TICKS(100))) {
            temp_medida = temp_atual;
            temp_limite = limite_temp;
            alarme = alarme_ativado;
            xSemaphoreGive(mutex_temp);
        } else {
            continue;
        }
        
        int diferenca = temp_limite - temp_medida;
        
        if (alarme) {
            digito = 'F';
        } else if (diferenca <= 2) {
            digito = 'D';
        } else if (diferenca <= 10) {
            digito = '7';
        } else if (diferenca <= 15) {
            digito = '3';
        } else if (diferenca <= 20) {
            digito = '0';
        } else {
            digito = ' ';
        }
        
        if (alarme) {
            uint64_t agora = esp_timer_get_time();
            if (agora - ultimo_piscar > 500000) {
                ultimo_piscar = agora;
                display_ligado = !display_ligado;
            }
        } else {
            display_ligado = true;
        }
        
        if (display_ligado) {
            mostrar_digito(digito);
        } else {
            mostrar_digito(' ');
        }
        
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void tarefa_log_dados(void *pvParam) {
    uint64_t ultimo_log = 0;
    
    while(1) {
        uint64_t agora = esp_timer_get_time() / 1000;
        
        if ((agora - ultimo_log) >= INTERVALO_LOG) {
            if (sd_pronto) {
                escrever_log();
                ultimo_log = agora;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void app_main() {
    configurar_gpios();
    configurar_pwm();
    configurar_i2c();
    configurar_adc();
    
    display_lcd.address = ENDERECO_LCD;
    display_lcd.num = NUMERO_I2C;
    display_lcd.backlight = 1;
    display_lcd.size = TAMANHO_LCD;
    lcd_i2c_init(&display_lcd);
    
    sd_pronto = inicializar_sd();
    
    lcd_i2c_cursor_set(&display_lcd, 0, 0);
    lcd_i2c_cursor_set(&display_lcd, 0, 1);
    vTaskDelay(pdMS_TO_TICKS(2000));
    lcd_i2c_write(&display_lcd, 0, CLEAR_DISPLAY);

    mutex_temp = xSemaphoreCreateMutex();
    fila_botoes = xQueueCreate(10, sizeof(int));
    
    xTaskCreate(tarefa_ler_temperatura, "leitura_temp", 2048, NULL, 5, NULL);
    xTaskCreate(tarefa_tratar_botoes, "tratar_botoes", 2048, NULL, 4, NULL);
    xTaskCreate(tarefa_acionar_alarme, "controle_alarme", 2048, NULL, 3, NULL);
    xTaskCreate(tarefa_atualizar_lcd, "atualizar_lcd", 2048, NULL, 2, NULL);
    xTaskCreate(tarefa_atualizar_display, "atualizar_display", 2048, NULL, 2, NULL);
    xTaskCreate(tarefa_log_dados, "log_sd", 4096, NULL, 1, NULL);

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}