// J1 1 kHz Diagnose-Programm
// Gibt auf allen 6 GPIO-Pins von J1 (Display-Stecker) ein 1 kHz Rechteck
// (50% Duty, 0 V / 3.3 V) aus, damit jede Leitung mit dem Oszilloskop
// auf Durchgang/Kontaktprobleme geprueft werden kann.
//
// J1 Pinbelegung (laut Schaltplan, verifiziert via KiCad):
//   Pin 1 = GPIO15 (BLK)  -> 1 kHz
//   Pin 2 = GPIO5  (CS)   -> 1 kHz
//   Pin 3 = GPIO2  (DC)   -> 1 kHz
//   Pin 4 = GPIO4  (RES)  -> 1 kHz
//   Pin 5 = GPIO23 (SDA)  -> 1 kHz
//   Pin 6 = GPIO18 (SCL)  -> 1 kHz
//   Pin 7 = +3.3 V        (statisch DC, kein Signal)
//   Pin 8 = GND           (0 V, kein Signal)

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>
#include <driver/gptimer.h>
#include <esp_log.h>

static const char *TAG = "J1_DIAG_1KHZ";

#define J1_PIN_MASK ( \
      (1ULL << GPIO_NUM_18) \
    | (1ULL << GPIO_NUM_23) \
    | (1ULL << GPIO_NUM_4)  \
    | (1ULL << GPIO_NUM_2)  \
    | (1ULL << GPIO_NUM_5)  \
    | (1ULL << GPIO_NUM_15))

static volatile uint32_t s_level = 0;

static bool IRAM_ATTR on_timer_alarm(gptimer_handle_t timer,
                                     const gptimer_alarm_event_data_t *edata,
                                     void *user_ctx)
{
    s_level ^= 1;
    // Direkter Registerzugriff via gpio_set_level fuer alle 6 Pins.
    // Bei 2 kHz ISR-Rate (Halbperiode 500 us) unkritisch.
    gpio_set_level(GPIO_NUM_18, s_level);
    gpio_set_level(GPIO_NUM_23, s_level);
    gpio_set_level(GPIO_NUM_4,  s_level);
    gpio_set_level(GPIO_NUM_2,  s_level);
    gpio_set_level(GPIO_NUM_5,  s_level);
    gpio_set_level(GPIO_NUM_15, s_level);
    return false;
}

static void init_gpios(void)
{
    gpio_config_t io_conf = {
        .intr_type    = GPIO_INTR_DISABLE,
        .mode         = GPIO_MODE_OUTPUT,
        .pin_bit_mask = J1_PIN_MASK,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));

    gpio_set_level(GPIO_NUM_18, 0);
    gpio_set_level(GPIO_NUM_23, 0);
    gpio_set_level(GPIO_NUM_4,  0);
    gpio_set_level(GPIO_NUM_2,  0);
    gpio_set_level(GPIO_NUM_5,  0);
    gpio_set_level(GPIO_NUM_15, 0);
}

static void init_1khz_timer(void)
{
    gptimer_handle_t timer = NULL;
    gptimer_config_t cfg = {
        .clk_src       = GPTIMER_CLK_SRC_DEFAULT,
        .direction     = GPTIMER_COUNT_UP,
        .resolution_hz = 1000000,  // 1 MHz -> 1 us pro Tick
    };
    ESP_ERROR_CHECK(gptimer_new_timer(&cfg, &timer));

    gptimer_event_callbacks_t cbs = { .on_alarm = on_timer_alarm };
    ESP_ERROR_CHECK(gptimer_register_event_callbacks(timer, &cbs, NULL));

    gptimer_alarm_config_t alarm = {
        .alarm_count                = 500,   // 500 us -> Toggle alle 500 us = 1 kHz
        .reload_count               = 0,
        .flags.auto_reload_on_alarm = true,
    };
    ESP_ERROR_CHECK(gptimer_set_alarm_action(timer, &alarm));
    ESP_ERROR_CHECK(gptimer_enable(timer));
    ESP_ERROR_CHECK(gptimer_start(timer));
}

void app_main(void)
{
    ESP_LOGI(TAG, "J1 1 kHz Diagnose laeuft");
    ESP_LOGI(TAG, "Pins: GPIO18, GPIO23, GPIO4, GPIO2, GPIO5, GPIO15");

    init_gpios();
    init_1khz_timer();

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(5000));
        ESP_LOGI(TAG, "1 kHz aktiv auf allen 6 J1 GPIOs");
    }
}
