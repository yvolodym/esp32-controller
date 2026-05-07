# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

An ESP32-based wireless game controller using ESP-NOW for low-latency peer-to-peer communication. Reads two analog joysticks and transmits data to a receiver. Features an ST7789 SPI display showing live joystick values.

## Architecture

- **Hardware**: ESP32-WROVER-KIT, native ESP-IDF (no Arduino)
- **Protocol**: ESP-NOW (no pairing, channel 1, no encryption)
- **Display**: ST7789 320×240 SPI — driven by `main/st7789_display.c`

### Data flow

`app_main` initializes NVS → display → GPIO → ADC → WiFi/ESP-NOW, then spawns `controller_task` (stack: 8 KB, priority 5).

`controller_task` runs in a tight loop:
1. Reads four ADC channels (joystick axes) + two GPIO pins (buttons) → fills `ControllerData`
2. Sends via `esp_now_send` every `SEND_DELAY_MS` (default 20 ms / 50 Hz)
3. Every 10th cycle (200 ms / 5 Hz) refreshes the ST7789 display

`ControllerData` struct (7 bytes sent over ESP-NOW):
```c
typedef struct {
    int16_t joy1_x, joy1_y;
    int16_t joy2_x, joy2_y;
    bool joy1_btn, joy2_btn;
    uint8_t batteryLevel;  // hardcoded to 100 — not yet implemented
} ControllerData;
```

Joystick calibration: raw 12-bit ADC → optional curve/line-fitting voltage calibration → mapped to −512…+512 with configurable deadzone.

### Components

- **`main/main.c`**: application entry point and controller loop
- **`main/st7789_display.c` / `.h`**: ST7789 SPI driver with built-in 8×8 ASCII font (2× scale), RGB565 colors

The `backup/` directory contains the original Arduino/PlatformIO implementation (Ukrainian comments) — useful as a reference for the earlier pin layout.

## Common Development Commands

```bash
idf.py menuconfig        # configure receiver MAC, send delay, deadzone
idf.py build
idf.py flash
idf.py monitor
idf.py build flash monitor
idf.py size              # check flash/RAM usage
idf.py set-target esp32  # only needed when switching targets
idf.py fullclean         # removes sdkconfig too
```

## Configuration Parameters

`idf.py menuconfig` → "ESP32 Controller Configuration":
- **Receiver MAC Address** — default `FF:FF:FF:FF:FF:FF` (broadcast)
- **Send Delay (ms)** — range 10–1000, default 20
- **Joystick Deadzone** — range 10–200, default 50

## Hardware Pin Assignments

### Joysticks (PS5-style Hall-effect)
| Signal | GPIO | ADC |
|---|---|---|
| Joystick 1 X | GPIO36 | ADC1_CH0 |
| Joystick 1 Y | GPIO39 | ADC1_CH3 |
| Joystick 1 Button | GPIO25 | — |
| Joystick 2 X | GPIO34 | ADC1_CH6 |
| Joystick 2 Y | GPIO35 | ADC1_CH7 |
| Joystick 2 Button | GPIO26 | — |

Buttons use internal pull-up; active-low logic is inverted in software.

### ST7789 Display (SPI / VSPI)
| Signal | GPIO |
|---|---|
| SCK | GPIO18 |
| MOSI | GPIO23 |
| CS | GPIO5 |
| DC | GPIO2 |
| RST | GPIO4 |
| Backlight | GPIO15 |

## Key Notes

- ADC uses `esp_adc/adc_oneshot` API with automatic calibration (curve fitting preferred, falls back to line fitting, then raw).
- WiFi is initialized in STA mode solely to enable the ESP-NOW radio — no network association.
- Battery level is **not implemented** (`batteryLevel` is hardcoded to 100).
- `sdkconfig.defaults` enables SPIRAM, 240 MHz CPU, 80 MHz flash/SPIRAM, and WiFi buffer tuning.
- Hardware reference datasheets are in `doc/`.
