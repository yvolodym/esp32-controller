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
#include <esp_adc/adc_cali.h>
#include <esp_adc/adc_cali_scheme.h>
#include <driver/gpio.h>
#include <nvs_flash.h>
#include <sdkconfig.h>

static const char *TAG = "ESP32_CONTROLLER";

// ========== CONFIGURATION ========== //
// Joystick 1
#define JOY1_X_PIN ADC_CHANNEL_4  // GPIO32 - X-axis joystick 1
#define JOY1_Y_PIN ADC_CHANNEL_5  // GPIO33 - Y-axis joystick 1
// Joystick 2
#define JOY2_X_PIN ADC_CHANNEL_6  // GPIO34 - X-axis joystick 2
#define JOY2_Y_PIN ADC_CHANNEL_7  // GPIO35 - Y-axis joystick 2
// Joystick buttons
#define JOY1_BTN_PIN GPIO_NUM_12  // Joystick 1 button
#define JOY2_BTN_PIN GPIO_NUM_14  // Joystick 2 button
// Battery
#define BATTERY_PIN ADC_CHANNEL_0 // GPIO36 - battery monitoring

const uint32_t SEND_DELAY_MS = CONFIG_CONTROLLER_SEND_DELAY_MS; // Delay between transmissions (ms)
const int DEADZONE = CONFIG_CONTROLLER_JOYSTICK_DEADZONE;      // Joystick deadzone

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
// Receiver MAC address from configuration
static uint8_t receiverMac[6];

// ADC handles
static adc_oneshot_unit_handle_t adc1_handle;
static adc_cali_handle_t adc1_cali_handle = NULL;
static bool adc_calibrated = false;

// Transmission statistics
static uint32_t send_success_count = 0;
static uint32_t send_fail_count = 0;

// Parse MAC address from configuration string
static void parse_mac_address(void) {
    const char* mac_str = CONFIG_CONTROLLER_RECEIVER_MAC;
    sscanf(mac_str, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
           &receiverMac[0], &receiverMac[1], &receiverMac[2],
           &receiverMac[3], &receiverMac[4], &receiverMac[5]);
    ESP_LOGI(TAG, "Receiver MAC: %02x:%02x:%02x:%02x:%02x:%02x",
             receiverMac[0], receiverMac[1], receiverMac[2],
             receiverMac[3], receiverMac[4], receiverMac[5]);
}

// ESP-NOW Send Callback
static void espnow_send_cb(const uint8_t *mac_addr, esp_now_send_status_t status) {
    if (status == ESP_NOW_SEND_SUCCESS) {
        send_success_count++;
    } else {
        send_fail_count++;
        ESP_LOGW(TAG, "Send failed, count: %lu", send_fail_count);
    }
}

// Joystick calibration
static int16_t calibrateJoystick(int rawValue) {
    int voltage = rawValue;

    // Use calibrated voltage if available
    if (adc_calibrated && adc1_cali_handle != NULL) {
        adc_cali_raw_to_voltage(adc1_cali_handle, rawValue, &voltage);
        // Convert mV to ADC equivalent range (0-3300mV to 0-4095)
        rawValue = (voltage * 4095) / 3300;
    }

    int16_t mapped = (int16_t)((rawValue - 2048) * 512 / 2048);  // Map 0-4095 to -512..512
    if (abs(mapped) < DEADZONE) return 0;
    return mapped;
}

// GPIO initialization for buttons
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

// ADC initialization
static void init_adc(void) {
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
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

    // Initialize ADC calibration
#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    adc_cali_curve_fitting_config_t cali_config = {
        .unit_id = ADC_UNIT_1,
        .atten = ADC_ATTEN_DB_11,
        .bitwidth = ADC_BITWIDTH_12,
    };
    esp_err_t ret = adc_cali_create_scheme_curve_fitting(&cali_config, &adc1_cali_handle);
    if (ret == ESP_OK) {
        adc_calibrated = true;
        ESP_LOGI(TAG, "ADC calibration enabled (Curve Fitting)");
    }
#elif ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    adc_cali_line_fitting_config_t cali_config = {
        .unit_id = ADC_UNIT_1,
        .atten = ADC_ATTEN_DB_11,
        .bitwidth = ADC_BITWIDTH_12,
    };
    esp_err_t ret = adc_cali_create_scheme_line_fitting(&cali_config, &adc1_cali_handle);
    if (ret == ESP_OK) {
        adc_calibrated = true;
        ESP_LOGI(TAG, "ADC calibration enabled (Line Fitting)");
    }
#endif
    if (!adc_calibrated) {
        ESP_LOGW(TAG, "ADC calibration not available - using raw values");
    }
}

// WiFi and ESP-NOW initialization
static void init_wifi_espnow(void) {
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    // Parse MAC address from configuration
    parse_mac_address();

    ESP_ERROR_CHECK(esp_now_init());

    // Register callback for send confirmation
    ESP_ERROR_CHECK(esp_now_register_send_cb(espnow_send_cb));

    esp_now_peer_info_t peerInfo = {0};
    memcpy(peerInfo.peer_addr, receiverMac, 6);
    peerInfo.channel = 1;
    peerInfo.encrypt = false;
    peerInfo.ifidx = WIFI_IF_STA;

    ESP_ERROR_CHECK(esp_now_add_peer(&peerInfo));
    ESP_LOGI(TAG, "ESP-NOW initialized and peer added");
}

// Data transmission
static void send_data(void) {
    int raw_value;

    // Read joysticks
    ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, JOY1_X_PIN, &raw_value));
    txData.joy1_x = calibrateJoystick(raw_value);

    ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, JOY1_Y_PIN, &raw_value));
    txData.joy1_y = calibrateJoystick(raw_value);

    ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, JOY2_X_PIN, &raw_value));
    txData.joy2_x = calibrateJoystick(raw_value);

    ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, JOY2_Y_PIN, &raw_value));
    txData.joy2_y = calibrateJoystick(raw_value);

    // Read buttons (invert due to PULLUP)
    txData.joy1_btn = !gpio_get_level(JOY1_BTN_PIN);
    txData.joy2_btn = !gpio_get_level(JOY2_BTN_PIN);

    // Read battery
    ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, BATTERY_PIN, &raw_value));
    txData.batteryLevel = (uint8_t)(raw_value * 100 / 4095);

    // Send data
    esp_err_t result = esp_now_send(receiverMac, (uint8_t *)&txData, sizeof(txData));

    // Debug information (every 100 transmissions)
    static uint32_t debug_counter = 0;
    debug_counter++;
    if (debug_counter % 100 == 0 || result != ESP_OK) {
        ESP_LOGI(TAG, "J1: %d,%d BTN:%d | J2: %d,%d BTN:%d | Battery: %d%% | Send: %lu/%lu | Status: %s",
            txData.joy1_x, txData.joy1_y, txData.joy1_btn,
            txData.joy2_x, txData.joy2_y, txData.joy2_btn,
            txData.batteryLevel,
            send_success_count, send_fail_count,
            result == ESP_OK ? "OK" : "FAIL");
    }
}

// Main controller task
static void controller_task(void *pvParameters) {
    ESP_LOGI(TAG, "Controller ready to work");

    while (1) {
        send_data();
        vTaskDelay(pdMS_TO_TICKS(SEND_DELAY_MS));
    }
}

void app_main(void) {
    ESP_LOGI(TAG, "Starting ESP32 controller");

    // Initialize NVS Flash (for WiFi)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Initialize components
    init_gpio();
    init_adc();
    init_wifi_espnow();

    // Short delay for stabilization
    vTaskDelay(pdMS_TO_TICKS(1000));

    // Create main task
    xTaskCreate(controller_task, "controller_task", 8192, NULL, 5, NULL);
}