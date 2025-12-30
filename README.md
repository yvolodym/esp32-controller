## Controller

### ESP now
* https://randomnerdtutorials.com/esp32-cyd-esp-now-receive-data/
* https://habr.com/ru/companies/beget/articles/929336/
* https://wolles-elektronikkiste.de/esp-now
* https://github.com/SolderedElectronics/USB-UART-CH340C-converter-board-hardware-design

### Joystik
* https://tech.alpsalpine.com/e/products/detail/RKJXV122400R/
* https://hackaday.io/project/199327-tetizmol-gd0153/log/237836-e1r-hall-tmr-joystick-electrical-properties
* https://hackaday.io/project/199327-tetizmol-gd0153/log/237786-br-ps5-hall-joysticks-and-espressif-adcs
* https://github.com/dbenoy/thumbstick-breakout/blob/main/docs/board.png

### PINS

##### ST7789 Display (SPI)
```
VCC → 3.3V
GND → GND
SCL/SCK → GPIO 18 (VSPI SCK)
SDA/MOSI → GPIO 23 (VSPI MOSI)
RES/RST → GPIO 4
DC → GPIO 2
CS → GPIO 5
BLK → GPIO 15 (oder 3.3V für dauerhaft an)
```

##### PS5 Joystick 1 (Linker Stick)
```
VCC → 3.3V
GND → GND
VRx → GPIO 36 (ADC1_CH0)
VRy → GPIO 39 (ADC1_CH3)
SW → GPIO 25 (Button)
```

##### PS5 Joystick 2 (Rechter Stick)
```
VCC → 3.3V
GND → GND
VRx → GPIO 34 (ADC1_CH6)
VRy → GPIO 35 (ADC1_CH7)
SW → GPIO 26 (Button)
```
