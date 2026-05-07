# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

An ESP32-based wireless game controller using ESP-NOW for low-latency peer-to-peer communication. Reads two analog joysticks and transmits data to a receiver. Features an ST7789 SPI display showing live joystick values.

## Architecture

- **Hardware**: ESP32-WROVER-KIT, native ESP-IDF (no Arduino)
- **Protocol**: ESP-NOW (no pairing, channel 1, no encryption)
- **Display**: ST7789 320×240 SPI — driven by `main/st7789_display.c`

### Data flow

```mermaid
flowchart TD
    A[app_main] --> NVS[NVS init]
    NVS --> DISP_INIT[Display init]
    DISP_INIT --> GPIO_INIT[GPIO init]
    GPIO_INIT --> ADC_INIT[ADC init]
    ADC_INIT --> WIFI[WiFi / ESP-NOW init]
    WIFI --> TASK["spawn controller_task\nstack: 8 KB · priority: 5"]

    subgraph loop["controller_task — tight loop"]
        direction TB
        ADC["Read ADC ×4\njoystick axes"] --> CD[ControllerData]
        BTN["Read GPIO ×2\nbuttons"] --> CD
        CD --> SEND["esp_now_send\nevery 20 ms / 50 Hz"]
        CD --> CYC{"10th cycle?\n200 ms / 5 Hz"}
        CYC -->|yes| REFRESH[Refresh ST7789]
    end

    TASK --> loop
```

`ControllerData` struct (7 bytes sent over ESP-NOW):
```c
typedef struct {
    int16_t joy1_x, joy1_y;
    int16_t joy2_x, joy2_y;
    bool joy1_btn, joy2_btn;
    uint8_t batteryLevel;  // hardcoded to 100 — not yet implemented
} ControllerData;
```

Joystick calibration pipeline:

```mermaid
flowchart LR
    RAW["Raw 12-bit ADC"] --> CAL{"Calibration\nscheme"}
    CAL -->|"curve fitting\n(preferred)"| VOLT[Voltage]
    CAL -->|"line fitting\n(fallback)"| VOLT
    CAL -->|"raw\n(fallback)"| VOLT
    VOLT --> MAP["Map to −512…+512"]
    MAP --> DZ["Apply deadzone"]
    DZ --> OUT["ControllerData axis"]
```

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

## Power Supply Design (supply.kicad_sch)

Automatic USB/battery switching — no manual intervention required.

### Topology

```mermaid
graph LR
    USB["USB 5V"]
    VBAT["VBAT\nLiPo"]
    VPWR(["VPWR"])
    V33(["＋3.3 V"])

    USB  -->|"D6 SS14\nVf ≈ 0.3 V"| VPWR
    VBAT -->|"LTC4412 + SI2301\nideal diode · Vdrop ≈ 20 mV"| VPWR
    VPWR -->|"U12 AP2112K-3.3\nLDO 600 mA · 250 mV dropout"| V33
    USB  -->|"U5 TP4056\nCC/CV 1 A charger"| VBAT
```

- **When USB present**: D6 conducts (VPWR ≈ 4.7V), SI2301 is reverse-biased by LTC4412 → battery isolated
- **When USB absent**: SI2301 conducts (VPWR ≈ VBAT − 20mV), D6 reverse-biased → battery powers system

### Components

| Ref | Part | Package | Function |
|---|---|---|---|
| U5 | TP4056-42-ESOP8 | ESOP-8 | LiPo charger (1A, CC/CV) |
| D5 | — | — | removed; replaced by LTC4412+SI2301 |
| D6 | SS14 | SMA | USB path diode |
| U13 | LTC4412ES6 (marking: **LTA2**) | TSOT-23-6 | PowerPath controller |
| Q1 | SI2305 | SOT-23 | P-ch MOSFET (VDS −20V, ID −4.1A, RDS 105–130mΩ) |
| U12 | AP2112K-3.3 | SOT-23-5 | LDO 3.3V, 600mA, 250mV dropout |

### LTC4412 wiring
- VIN + SENSE → VBAT
- GATE → SI2301 Gate
- CTL → VIN (always enabled)
- STAT → optional (open or pulled up to signal USB presence)
- SI2301: Source → VBAT, Drain → VPWR

### Why AP2112K instead of AMS1117
AMS1117 has 1.3V dropout — cannot regulate from LiPo (needs ≥4.6V in).
AP2112K has 250mV dropout — works at VPWR = 3.72V (VBAT 3.7V − 20mV SI2301).

### TP4056 notes
- TEMP pin → VCC (not GND) when no NTC thermistor used; TEMP=GND permanently disables charging
- EPAD → GND
- CC1/CC2 on USB-C connector → 5.1kΩ pull-downs to GND (sink mode, required for USB-C chargers)

## Key Notes

- ADC uses `esp_adc/adc_oneshot` API with automatic calibration (curve fitting preferred, falls back to line fitting, then raw).
- WiFi is initialized in STA mode solely to enable the ESP-NOW radio — no network association.
- Battery level is **not implemented** (`batteryLevel` is hardcoded to 100).
- `sdkconfig.defaults` enables SPIRAM, 240 MHz CPU, 80 MHz flash/SPIRAM, and WiFi buffer tuning.
- Hardware reference datasheets are in `doc/`.
