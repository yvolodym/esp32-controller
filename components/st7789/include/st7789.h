#ifndef ST7789_H
#define ST7789_H

#include <stdint.h>
#include <stdbool.h>
#include "driver/spi_master.h"
#include "driver/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

// ST7789 Display Configuration
#define ST7789_WIDTH  240
#define ST7789_HEIGHT 320

// Color definitions (RGB565 format)
#define ST7789_BLACK       0x0000
#define ST7789_NAVY        0x000F
#define ST7789_DARKGREEN   0x03E0
#define ST7789_DARKCYAN    0x03EF
#define ST7789_MAROON      0x7800
#define ST7789_PURPLE      0x780F
#define ST7789_OLIVE       0x7BE0
#define ST7789_LIGHTGREY   0xC618
#define ST7789_DARKGREY    0x7BEF
#define ST7789_BLUE        0x001F
#define ST7789_GREEN       0x07E0
#define ST7789_CYAN        0x07FF
#define ST7789_RED         0xF800
#define ST7789_MAGENTA     0xF81F
#define ST7789_YELLOW      0xFFE0
#define ST7789_WHITE       0xFFFF
#define ST7789_ORANGE      0xFD20
#define ST7789_GREENYELLOW 0xAFE5
#define ST7789_PINK        0xF81F

// ST7789 Configuration Structure
typedef struct {
    spi_host_device_t spi_host;
    int pin_mosi;
    int pin_clk;
    int pin_cs;
    int pin_dc;
    int pin_rst;
    int pin_bckl;
    int spi_clock_speed_hz;
} st7789_config_t;

// ST7789 Handle Structure
typedef struct {
    spi_device_handle_t spi_handle;
    st7789_config_t config;
    bool initialized;
} st7789_handle_t;

/**
 * @brief Initialize ST7789 display
 *
 * @param handle Pointer to ST7789 handle structure
 * @param config Pointer to ST7789 configuration structure
 * @return esp_err_t ESP_OK on success
 */
esp_err_t st7789_init(st7789_handle_t *handle, const st7789_config_t *config);

/**
 * @brief Deinitialize ST7789 display
 *
 * @param handle Pointer to ST7789 handle structure
 * @return esp_err_t ESP_OK on success
 */
esp_err_t st7789_deinit(st7789_handle_t *handle);

/**
 * @brief Fill entire screen with specified color
 *
 * @param handle Pointer to ST7789 handle structure
 * @param color RGB565 color value
 * @return esp_err_t ESP_OK on success
 */
esp_err_t st7789_fill_screen(st7789_handle_t *handle, uint16_t color);

/**
 * @brief Set pixel at specified coordinates
 *
 * @param handle Pointer to ST7789 handle structure
 * @param x X coordinate (0 to ST7789_WIDTH-1)
 * @param y Y coordinate (0 to ST7789_HEIGHT-1)
 * @param color RGB565 color value
 * @return esp_err_t ESP_OK on success
 */
esp_err_t st7789_set_pixel(st7789_handle_t *handle, uint16_t x, uint16_t y, uint16_t color);

/**
 * @brief Draw rectangle
 *
 * @param handle Pointer to ST7789 handle structure
 * @param x X coordinate of top-left corner
 * @param y Y coordinate of top-left corner
 * @param width Rectangle width
 * @param height Rectangle height
 * @param color RGB565 color value
 * @return esp_err_t ESP_OK on success
 */
esp_err_t st7789_draw_rect(st7789_handle_t *handle, uint16_t x, uint16_t y,
                          uint16_t width, uint16_t height, uint16_t color);

/**
 * @brief Draw filled rectangle
 *
 * @param handle Pointer to ST7789 handle structure
 * @param x X coordinate of top-left corner
 * @param y Y coordinate of top-left corner
 * @param width Rectangle width
 * @param height Rectangle height
 * @param color RGB565 color value
 * @return esp_err_t ESP_OK on success
 */
esp_err_t st7789_fill_rect(st7789_handle_t *handle, uint16_t x, uint16_t y,
                          uint16_t width, uint16_t height, uint16_t color);

/**
 * @brief Draw simple text string
 *
 * @param handle Pointer to ST7789 handle structure
 * @param x X coordinate for text start
 * @param y Y coordinate for text start
 * @param text Text string to display
 * @param color RGB565 color value
 * @param bg_color RGB565 background color value
 * @param scale Text scale factor (1-4)
 * @return esp_err_t ESP_OK on success
 */
esp_err_t st7789_draw_text(st7789_handle_t *handle, uint16_t x, uint16_t y,
                          const char *text, uint16_t color, uint16_t bg_color, uint8_t scale);

/**
 * @brief Set backlight brightness
 *
 * @param handle Pointer to ST7789 handle structure
 * @param brightness Brightness level (0-255)
 * @return esp_err_t ESP_OK on success
 */
esp_err_t st7789_set_backlight(st7789_handle_t *handle, uint8_t brightness);

#ifdef __cplusplus
}
#endif

#endif // ST7789_H