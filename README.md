## Controller

ESP32-WROVER based wireless game controller. Two resistive (potentiometer)
analog joysticks plus an ST7789 SPI display, transmitting over ESP-NOW.
Firmware uses native ESP-IDF — see `CLAUDE.md` for architecture and pin
layout details.

### References

#### ESP-NOW
* https://randomnerdtutorials.com/esp32-cyd-esp-now-receive-data/
* https://habr.com/ru/companies/beget/articles/929336/
* https://wolles-elektronikkiste.de/esp-now
* https://github.com/SolderedElectronics/USB-UART-CH340C-converter-board-hardware-design

#### Joysticks (resistive thumbstick modules)
* https://tech.alpsalpine.com/e/products/detail/RKJXV122400R/
* https://github.com/dbenoy/thumbstick-breakout/blob/main/docs/board.png

#### LiPo charging module
* https://elektro.turanis.de/html/prj224/index.html

### Pin assignments

`CLAUDE.md` is the source of truth — the lists below are a quick reference.
All pins verified against `kicad/esp32-controller.kicad_sch` and
`kicad/supply.kicad_sch`.

#### ST7789 display (J1, 8-pin header — VSPI)

| J1 pin | Signal | GPIO |
|---|---|---|
| 1 | BL (backlight) | GPIO15 |
| 2 | CS  | GPIO5  |
| 3 | DC  | GPIO2  |
| 4 | RST | GPIO4  |
| 5 | MOSI / SDA | GPIO23 |
| 6 | SCK / SCL  | GPIO18 |
| 7 | VCC | +3.3 V |
| 8 | GND | GND |

#### Joystick 1 — RIGHT stick (U2 in schematic)

| Signal | GPIO | ADC |
|---|---|---|
| VRx (X axis) | GPIO32 | ADC1_CH4 |
| VRy (Y axis) | GPIO33 | ADC1_CH5 |
| Button (SEL) | GPIO25 | — *(not wired in hardware, see CLAUDE.md)* |

#### Joystick 2 — LEFT stick (U3 in schematic)

| Signal | GPIO | ADC |
|---|---|---|
| HRx (X axis) | GPIO34 | ADC1_CH6 |
| HRy (Y axis) | GPIO35 | ADC1_CH7 |
| Button (SEL) | GPIO26 | — *(not wired in hardware, see CLAUDE.md)* |

#### Battery sense

| Signal | GPIO | ADC |
|---|---|---|
| VBAT divider (R17 220 kΩ / R18 100 kΩ, net `HVBAT`) | GPIO36 (SENSOR_VP) | ADC1_CH0 |
