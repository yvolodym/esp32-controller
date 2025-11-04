#ifndef ST7789_DISPLAY_H
#define ST7789_DISPLAY_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <driver/spi_master.h>
#include <driver/gpio.h>
#include <esp_log.h>

// ST7789 Pin Definitions
#define ST7789_SCLK_PIN      GPIO_NUM_18
#define ST7789_MOSI_PIN      GPIO_NUM_23
#define ST7789_CS_PIN        GPIO_NUM_5
#define ST7789_DC_PIN        GPIO_NUM_2
#define ST7789_RST_PIN       GPIO_NUM_4
#define ST7789_BL_PIN        GPIO_NUM_15

// ST7789 Commands
#define ST7789_SWRESET       0x01
#define ST7789_SLPOUT        0x11
#define ST7789_COLMOD        0x3A
#define ST7789_MADCTL        0x36
#define ST7789_CASET         0x2A
#define ST7789_RASET         0x2B
#define ST7789_RAMWR         0x2C
#define ST7789_DISPON        0x29
#define ST7789_INVON         0x21

// Display dimensions
#define ST7789_WIDTH         240
#define ST7789_HEIGHT        240

// Font dimensions (simple 8x8 font)
#define FONT_WIDTH           8
#define FONT_HEIGHT          8

// Colors (RGB565 format)
#define ST7789_BLACK         0x0000
#define ST7789_WHITE         0xFFFF
#define ST7789_RED           0xF800
#define ST7789_GREEN         0x07E0
#define ST7789_BLUE          0x001F
#define ST7789_YELLOW        0xFFE0
#define ST7789_CYAN          0x07FF
#define ST7789_MAGENTA       0xF81F

typedef struct {
    spi_device_handle_t spi_handle;
    bool initialized;
} st7789_display_t;

// Function declarations
esp_err_t st7789_init(st7789_display_t *display);
esp_err_t st7789_fill_screen(st7789_display_t *display, uint16_t color);
esp_err_t st7789_set_window(st7789_display_t *display, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
esp_err_t st7789_write_pixel(st7789_display_t *display, uint16_t x, uint16_t y, uint16_t color);
esp_err_t st7789_draw_string(st7789_display_t *display, uint16_t x, uint16_t y, const char *str, uint16_t color, uint16_t bg_color);
esp_err_t st7789_draw_char(st7789_display_t *display, uint16_t x, uint16_t y, char c, uint16_t color, uint16_t bg_color);
void st7789_display_joystick_data(st7789_display_t *display, int16_t joy1_x, int16_t joy1_y, int16_t joy2_x, int16_t joy2_y, bool joy1_btn, bool joy2_btn, uint8_t battery);

#endif // ST7789_DISPLAY_H