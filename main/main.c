#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include <esp_now.h>
#include <esp_log.h>
#include <esp_adc/adc_oneshot.h>
#include <driver/gpio.h>
#include <sdkconfig.h>

static const char *TAG = "ESP32_CONTROLLER";

// ========== НАСТРОЙКИ ========== //
// Джойстик 1
#define JOY1_X_PIN ADC_CHANNEL_4  // GPIO32 - X-ось джойстика 1
#define JOY1_Y_PIN ADC_CHANNEL_5  // GPIO33 - Y-ось джойстика 1
// Джойстик 2
#define JOY2_X_PIN ADC_CHANNEL_6  // GPIO34 - X-ось джойстика 2
#define JOY2_Y_PIN ADC_CHANNEL_7  // GPIO35 - Y-ось джойстика 2
// Кнопки джойстиков
#define JOY1_BTN_PIN GPIO_NUM_12  // Кнопка джойстика 1
#define JOY2_BTN_PIN GPIO_NUM_14  // Кнопка джойстика 2
// Батарея
#define BATTERY_PIN ADC_CHANNEL_0 // GPIO36 - мониторинг батареи

const uint32_t SEND_DELAY_MS = CONFIG_CONTROLLER_SEND_DELAY_MS; // Задержка между отправками (мс)
const int DEADZONE = CONFIG_CONTROLLER_JOYSTICK_DEADZONE;      // Мертвая зона джойстика

typedef struct {
    int16_t joy1_x;
    int16_t joy1_y;
    int16_t joy2_x;
    int16_t joy2_y;
    bool joy1_btn;
    bool joy2_btn;
    uint8_t batteryLevel;
} ControllerData;

static ControllerData txData;
// Замените на MAC-адрес вашего приёмника
static uint8_t receiverMac[] = {0x94, 0x3C, 0xC6, 0x33, 0x71, 0x68};

// ADC handle
static adc_oneshot_unit_handle_t adc1_handle;

// Калибровка джойстика
static int16_t calibrateJoystick(int rawValue) {
    int16_t mapped = (int16_t)((rawValue - 2048) * 512 / 2048);  // Map 0-4095 to -512..512
    if (abs(mapped) < DEADZONE) return 0;
    return mapped;
}

// Инициализация GPIO для кнопок
static void init_gpio(void) {
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << JOY1_BTN_PIN) | (1ULL << JOY2_BTN_PIN),
        .pull_down_en = 0,
        .pull_up_en = 1,
    };
    gpio_config(&io_conf);
}

// Инициализация ADC
static void init_adc(void) {
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_12,
        .atten = ADC_ATTEN_DB_11,
    };

    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, JOY1_X_PIN, &config));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, JOY1_Y_PIN, &config));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, JOY2_X_PIN, &config));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, JOY2_Y_PIN, &config));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, BATTERY_PIN, &config));
}

// Инициализация WiFi и ESP-NOW
static void init_wifi_espnow(void) {
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_ERROR_CHECK(esp_now_init());

    esp_now_peer_info_t peerInfo = {0};
    memcpy(peerInfo.peer_addr, receiverMac, 6);
    peerInfo.channel = 1;
    peerInfo.encrypt = false;

    ESP_ERROR_CHECK(esp_now_add_peer(&peerInfo));
    ESP_LOGI(TAG, "ESP-NOW инициализирован");
}

// Отправка данных
static void send_data(void) {
    int raw_value;

    // Чтение джойстиков
    ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, JOY1_X_PIN, &raw_value));
    txData.joy1_x = calibrateJoystick(raw_value);

    ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, JOY1_Y_PIN, &raw_value));
    txData.joy1_y = calibrateJoystick(raw_value);

    ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, JOY2_X_PIN, &raw_value));
    txData.joy2_x = calibrateJoystick(raw_value);

    ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, JOY2_Y_PIN, &raw_value));
    txData.joy2_y = calibrateJoystick(raw_value);

    // Чтение кнопок (инвертируем из-за PULLUP)
    txData.joy1_btn = !gpio_get_level(JOY1_BTN_PIN);
    txData.joy2_btn = !gpio_get_level(JOY2_BTN_PIN);

    // Чтение батареи
    ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, BATTERY_PIN, &raw_value));
    txData.batteryLevel = (uint8_t)(raw_value * 100 / 4095);

    // Отправка данных
    esp_err_t result = esp_now_send(receiverMac, (uint8_t *)&txData, sizeof(txData));

    // Отладочная информация
    ESP_LOGI(TAG, "J1: %d,%d BTN:%d | J2: %d,%d BTN:%d | Battery: %d%% | Status: %s",
        txData.joy1_x, txData.joy1_y, txData.joy1_btn,
        txData.joy2_x, txData.joy2_y, txData.joy2_btn,
        txData.batteryLevel,
        result == ESP_OK ? "OK" : "FAIL");
}

// Основная задача контроллера
static void controller_task(void *pvParameters) {
    ESP_LOGI(TAG, "Контроллер готов к работе");

    while (1) {
        send_data();
        vTaskDelay(pdMS_TO_TICKS(SEND_DELAY_MS));
    }
}

void app_main(void) {
    ESP_LOGI(TAG, "Запуск ESP32 контроллера");

    // Инициализация компонентов
    init_gpio();
    init_adc();
    init_wifi_espnow();

    // Создание основной задачи
    xTaskCreate(controller_task, "controller_task", 4096, NULL, 5, NULL);
}