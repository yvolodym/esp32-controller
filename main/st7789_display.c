#include "st7789_display.h"
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/spi_master.h>
#include <driver/gpio.h>
#include <esp_log.h>

static const char *TAG = "ST7789";

// Simple 8x8 ASCII font (characters 32-127)
static const uint8_t font8x8[][8] = {
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // Space
    {0x18, 0x3C, 0x3C, 0x18, 0x18, 0x00, 0x18, 0x00}, // !
    {0x36, 0x36, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // "
    {0x36, 0x36, 0x7F, 0x36, 0x7F, 0x36, 0x36, 0x00}, // #
    {0x0C, 0x3E, 0x03, 0x1E, 0x30, 0x1F, 0x0C, 0x00}, // $
    {0x00, 0x63, 0x33, 0x18, 0x0C, 0x66, 0x63, 0x00}, // %
    {0x1C, 0x36, 0x1C, 0x6E, 0x3B, 0x33, 0x6E, 0x00}, // &
    {0x06, 0x06, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00}, // '
    {0x18, 0x0C, 0x06, 0x06, 0x06, 0x0C, 0x18, 0x00}, // (
    {0x06, 0x0C, 0x18, 0x18, 0x18, 0x0C, 0x06, 0x00}, // )
    {0x00, 0x66, 0x3C, 0xFF, 0x3C, 0x66, 0x00, 0x00}, // *
    {0x00, 0x0C, 0x0C, 0x3F, 0x0C, 0x0C, 0x00, 0x00}, // +
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C, 0x06}, // ,
    {0x00, 0x00, 0x00, 0x3F, 0x00, 0x00, 0x00, 0x00}, // -
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C, 0x00}, // .
    {0x60, 0x30, 0x18, 0x0C, 0x06, 0x03, 0x01, 0x00}, // /
    {0x3E, 0x63, 0x73, 0x7B, 0x6F, 0x67, 0x3E, 0x00}, // 0
    {0x0C, 0x0E, 0x0C, 0x0C, 0x0C, 0x0C, 0x3F, 0x00}, // 1
    {0x1E, 0x33, 0x30, 0x1C, 0x06, 0x33, 0x3F, 0x00}, // 2
    {0x1E, 0x33, 0x30, 0x1C, 0x30, 0x33, 0x1E, 0x00}, // 3
    {0x38, 0x3C, 0x36, 0x33, 0x7F, 0x30, 0x78, 0x00}, // 4
    {0x3F, 0x03, 0x1F, 0x30, 0x30, 0x33, 0x1E, 0x00}, // 5
    {0x1C, 0x06, 0x03, 0x1F, 0x33, 0x33, 0x1E, 0x00}, // 6
    {0x3F, 0x33, 0x30, 0x18, 0x0C, 0x0C, 0x0C, 0x00}, // 7
    {0x1E, 0x33, 0x33, 0x1E, 0x33, 0x33, 0x1E, 0x00}, // 8
    {0x1E, 0x33, 0x33, 0x3E, 0x30, 0x18, 0x0E, 0x00}, // 9
    {0x00, 0x0C, 0x0C, 0x00, 0x00, 0x0C, 0x0C, 0x00}, // :
    {0x00, 0x0C, 0x0C, 0x00, 0x00, 0x0C, 0x0C, 0x06}, // ;
    {0x18, 0x0C, 0x06, 0x03, 0x06, 0x0C, 0x18, 0x00}, // <
    {0x00, 0x00, 0x3F, 0x00, 0x00, 0x3F, 0x00, 0x00}, // =
    {0x06, 0x0C, 0x18, 0x30, 0x18, 0x0C, 0x06, 0x00}, // >
    {0x1E, 0x33, 0x30, 0x18, 0x0C, 0x00, 0x0C, 0x00}, // ?
    {0x3E, 0x63, 0x7B, 0x7B, 0x7B, 0x03, 0x1E, 0x00}, // @
    {0x0C, 0x1E, 0x33, 0x33, 0x3F, 0x33, 0x33, 0x00}, // A
    {0x3F, 0x66, 0x66, 0x3E, 0x66, 0x66, 0x3F, 0x00}, // B
    {0x3C, 0x66, 0x03, 0x03, 0x03, 0x66, 0x3C, 0x00}, // C
    {0x1F, 0x36, 0x66, 0x66, 0x66, 0x36, 0x1F, 0x00}, // D
    {0x7F, 0x46, 0x16, 0x1E, 0x16, 0x46, 0x7F, 0x00}, // E
    {0x7F, 0x46, 0x16, 0x1E, 0x16, 0x06, 0x0F, 0x00}, // F
    {0x3C, 0x66, 0x03, 0x03, 0x73, 0x66, 0x7C, 0x00}, // G
    {0x33, 0x33, 0x33, 0x3F, 0x33, 0x33, 0x33, 0x00}, // H
    {0x1E, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x1E, 0x00}, // I
    {0x78, 0x30, 0x30, 0x30, 0x33, 0x33, 0x1E, 0x00}, // J
    {0x67, 0x66, 0x36, 0x1E, 0x36, 0x66, 0x67, 0x00}, // K
    {0x0F, 0x06, 0x06, 0x06, 0x46, 0x66, 0x7F, 0x00}, // L
    {0x63, 0x77, 0x7F, 0x7F, 0x6B, 0x63, 0x63, 0x00}, // M
    {0x63, 0x67, 0x6F, 0x7B, 0x73, 0x63, 0x63, 0x00}, // N
    {0x1C, 0x36, 0x63, 0x63, 0x63, 0x36, 0x1C, 0x00}, // O
    {0x3F, 0x66, 0x66, 0x3E, 0x06, 0x06, 0x0F, 0x00}, // P
    {0x1E, 0x33, 0x33, 0x33, 0x3B, 0x1E, 0x38, 0x00}, // Q
    {0x3F, 0x66, 0x66, 0x3E, 0x36, 0x66, 0x67, 0x00}, // R
    {0x1E, 0x33, 0x07, 0x0E, 0x38, 0x33, 0x1E, 0x00}, // S
    {0x3F, 0x2D, 0x0C, 0x0C, 0x0C, 0x0C, 0x1E, 0x00}, // T
    {0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x3F, 0x00}, // U
    {0x33, 0x33, 0x33, 0x33, 0x33, 0x1E, 0x0C, 0x00}, // V
    {0x63, 0x63, 0x63, 0x6B, 0x7F, 0x77, 0x63, 0x00}, // W
    {0x63, 0x63, 0x36, 0x1C, 0x1C, 0x36, 0x63, 0x00}, // X
    {0x33, 0x33, 0x33, 0x1E, 0x0C, 0x0C, 0x1E, 0x00}, // Y
    {0x7F, 0x63, 0x31, 0x18, 0x4C, 0x66, 0x7F, 0x00}, // Z
};

// Send command to ST7789
static esp_err_t st7789_send_cmd(st7789_display_t *display, uint8_t cmd) {
    esp_err_t ret;
    spi_transaction_t t = {
        .length = 8,
        .tx_buffer = &cmd,
        .user = (void *)0, // D/C = 0 for command
    };

    gpio_set_level(ST7789_DC_PIN, 0);
    ret = spi_device_polling_transmit(display->spi_handle, &t);
    return ret;
}

// Send data to ST7789
static esp_err_t st7789_send_data(st7789_display_t *display, const uint8_t *data, int len) {
    esp_err_t ret;

    if (len == 0) return ESP_OK;

    spi_transaction_t t = {
        .length = len * 8,
        .tx_buffer = data,
        .user = (void *)1, // D/C = 1 for data
    };

    gpio_set_level(ST7789_DC_PIN, 1);
    ret = spi_device_polling_transmit(display->spi_handle, &t);
    return ret;
}

// Initialize ST7789 display
esp_err_t st7789_init(st7789_display_t *display) {
    esp_err_t ret;

    ESP_LOGI(TAG, "Initializing ST7789 display");

    // Initialize GPIO pins
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << ST7789_DC_PIN) | (1ULL << ST7789_RST_PIN) | (1ULL << ST7789_BL_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = 0,
        .pull_down_en = 0,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);

    // Initialize SPI bus
    spi_bus_config_t buscfg = {
        .miso_io_num = -1,
        .mosi_io_num = ST7789_MOSI_PIN,
        .sclk_io_num = ST7789_SCLK_PIN,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = ST7789_WIDTH * ST7789_HEIGHT * 2,
    };

    ret = spi_bus_initialize(VSPI_HOST, &buscfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SPI bus: %s", esp_err_to_name(ret));
        return ret;
    }

    // Attach ST7789 to SPI bus
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 40 * 1000 * 1000, // 40 MHz
        .mode = 0,
        .spics_io_num = ST7789_CS_PIN,
        .queue_size = 7,
        .pre_cb = NULL,
    };

    ret = spi_bus_add_device(VSPI_HOST, &devcfg, &display->spi_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add SPI device: %s", esp_err_to_name(ret));
        return ret;
    }

    // Hardware reset
    gpio_set_level(ST7789_RST_PIN, 0);
    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_set_level(ST7789_RST_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(100));

    // Software reset
    st7789_send_cmd(display, ST7789_SWRESET);
    vTaskDelay(pdMS_TO_TICKS(150));

    // Sleep out
    st7789_send_cmd(display, ST7789_SLPOUT);
    vTaskDelay(pdMS_TO_TICKS(120));

    // Color mode: 16-bit color
    st7789_send_cmd(display, ST7789_COLMOD);
    uint8_t color_mode = 0x55; // 16-bit/pixel
    st7789_send_data(display, &color_mode, 1);

    // Memory data access control (0x60 = Landscape mode: MV + MX bits set)
    st7789_send_cmd(display, ST7789_MADCTL);
    uint8_t madctl = 0x60;  // Landscape orientation
    st7789_send_data(display, &madctl, 1);

    // Inversion on
    st7789_send_cmd(display, ST7789_INVON);
    vTaskDelay(pdMS_TO_TICKS(10));

    // Display on
    st7789_send_cmd(display, ST7789_DISPON);
    vTaskDelay(pdMS_TO_TICKS(100));

    // Turn on backlight
    gpio_set_level(ST7789_BL_PIN, 1);

    display->initialized = true;
    ESP_LOGI(TAG, "ST7789 display initialized successfully");

    return ESP_OK;
}

// Set drawing window
esp_err_t st7789_set_window(st7789_display_t *display, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    x0 += ST7789_X_OFFSET;
    x1 += ST7789_X_OFFSET;
    y0 += ST7789_Y_OFFSET;
    y1 += ST7789_Y_OFFSET;

    st7789_send_cmd(display, ST7789_CASET);
    uint8_t col_data[4] = {x0 >> 8, x0 & 0xFF, x1 >> 8, x1 & 0xFF};
    st7789_send_data(display, col_data, 4);

    st7789_send_cmd(display, ST7789_RASET);
    uint8_t row_data[4] = {y0 >> 8, y0 & 0xFF, y1 >> 8, y1 & 0xFF};
    st7789_send_data(display, row_data, 4);

    st7789_send_cmd(display, ST7789_RAMWR);

    return ESP_OK;
}

// Fill screen with color
esp_err_t st7789_fill_screen(st7789_display_t *display, uint16_t color) {
    st7789_set_window(display, 0, 0, ST7789_WIDTH - 1, ST7789_HEIGHT - 1);

    // Prepare color data (big endian)
    uint8_t color_data[2] = {color >> 8, color & 0xFF};

    // Send color for each pixel
    for (int i = 0; i < ST7789_WIDTH * ST7789_HEIGHT; i++) {
        st7789_send_data(display, color_data, 2);
    }

    return ESP_OK;
}

// Write single pixel
esp_err_t st7789_write_pixel(st7789_display_t *display, uint16_t x, uint16_t y, uint16_t color) {
    if (x >= ST7789_WIDTH || y >= ST7789_HEIGHT) return ESP_ERR_INVALID_ARG;

    st7789_set_window(display, x, y, x, y);
    uint8_t color_data[2] = {color >> 8, color & 0xFF};
    st7789_send_data(display, color_data, 2);

    return ESP_OK;
}

// Draw single character with scaling
esp_err_t st7789_draw_char(st7789_display_t *display, uint16_t x, uint16_t y, char c, uint16_t color, uint16_t bg_color) {
    if (c < 32 || c > 90) c = 32; // Space for unsupported chars

    const uint8_t *glyph = font8x8[c - 32];

    // Draw character with scaling
    for (int row = 0; row < FONT_HEIGHT; row++) {
        for (int col = 0; col < FONT_WIDTH; col++) {
            uint16_t pixel_color = (glyph[row] & (1 << col)) ? color : bg_color;

            // Draw scaled pixel (FONT_SCALE x FONT_SCALE block)
            for (int sy = 0; sy < FONT_SCALE; sy++) {
                for (int sx = 0; sx < FONT_SCALE; sx++) {
                    st7789_write_pixel(display,
                                     x + col * FONT_SCALE + sx,
                                     y + row * FONT_SCALE + sy,
                                     pixel_color);
                }
            }
        }
    }

    return ESP_OK;
}

// Draw string with scaling
esp_err_t st7789_draw_string(st7789_display_t *display, uint16_t x, uint16_t y, const char *str, uint16_t color, uint16_t bg_color) {
    uint16_t cursor_x = x;
    uint16_t cursor_y = y;

    while (*str) {
        if (*str == '\n') {
            cursor_x = x;
            cursor_y += (FONT_HEIGHT * FONT_SCALE) + 2;
        } else {
            st7789_draw_char(display, cursor_x, cursor_y, *str, color, bg_color);
            cursor_x += (FONT_WIDTH * FONT_SCALE);
        }
        str++;
    }

    return ESP_OK;
}

// Display joystick data (landscape layout with scaled font)
void st7789_display_joystick_data(st7789_display_t *display, int16_t joy1_x, int16_t joy1_y,
                                   int16_t joy2_x, int16_t joy2_y, bool joy1_btn, bool joy2_btn,
                                   uint8_t battery) {
    char buffer[64];
    const uint16_t line_height = (FONT_HEIGHT * FONT_SCALE) + 4;  // Spacing between lines

    // Display Title
    st7789_draw_string(display, 10, 10, "ESP32 Controller", ST7789_CYAN, ST7789_BLACK);

    // Display Joystick 1
    snprintf(buffer, sizeof(buffer), "J1: X:%4d Y:%4d", joy1_x, joy1_y);
    st7789_draw_string(display, 10, 10 + line_height * 2, buffer, ST7789_WHITE, ST7789_BLACK);

    snprintf(buffer, sizeof(buffer), "BTN: %s", joy1_btn ? "PRESSED" : "RELEASED");
    st7789_draw_string(display, 10, 10 + line_height * 3, buffer, joy1_btn ? ST7789_GREEN : ST7789_RED, ST7789_BLACK);

    // Display Joystick 2
    snprintf(buffer, sizeof(buffer), "J2: X:%4d Y:%4d", joy2_x, joy2_y);
    st7789_draw_string(display, 10, 10 + line_height * 5, buffer, ST7789_WHITE, ST7789_BLACK);

    snprintf(buffer, sizeof(buffer), "BTN: %s", joy2_btn ? "PRESSED" : "RELEASED");
    st7789_draw_string(display, 10, 10 + line_height * 6, buffer, joy2_btn ? ST7789_GREEN : ST7789_RED, ST7789_BLACK);

    // Display Battery
    snprintf(buffer, sizeof(buffer), "BATTERY: %3d%%", battery);
    st7789_draw_string(display, 10, 10 + line_height * 8, buffer, ST7789_YELLOW, ST7789_BLACK);
}
