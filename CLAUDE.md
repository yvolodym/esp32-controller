# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is an ESP32-based wireless game controller project that uses ESP-NOW protocol for low-latency communication. The controller reads input from two analog joysticks and their buttons, then transmits the data wirelessly to a receiver device.

## Architecture

- **Hardware Platform**: ESP32 (ESP-WROVER-Kit board)
- **Framework**: Native ESP-IDF
- **Communication Protocol**: ESP-NOW for peer-to-peer wireless communication
- **Input Hardware**: 2 analog joysticks (X/Y axes) with push buttons
- **Power Monitoring**: Battery level reading via ADC

### Key Components

- **Main Controller Logic** (`main/main.c`): Native ESP-IDF application handling joystick input, calibration, and ESP-NOW transmission
- **Data Structure**: `ControllerData` struct containing joystick positions, button states, and battery level
- **Hardware Configuration**: Pin definitions and calibration settings for dual joystick setup
- **Configuration**: Kconfig-based configuration system for receiver MAC address and timing parameters
- **KiCad PCB Design** (`kicad/esp32-controller/`): Hardware schematics and PCB layout

## Common Development Commands

### Build and Flash
```bash
# Configure the project (run once or when changing settings)
idf.py menuconfig

# Build the project
idf.py build

# Flash to ESP32
idf.py flash

# Build and flash in one command
idf.py build flash

# Monitor serial output
idf.py monitor

# Build, flash, and monitor in sequence
idf.py build flash monitor

# Clean build
idf.py clean

# Full clean (including sdkconfig)
idf.py fullclean
```

### Configuration
```bash
# Open configuration menu
idf.py menuconfig

# Set target chip (if different from ESP32)
idf.py set-target esp32

# Show project size information
idf.py size
```

## Configuration Files

- **`CMakeLists.txt`**: Root build configuration for ESP-IDF
- **`main/CMakeLists.txt`**: Main component build configuration
- **`main/Kconfig.projbuild`**: Project-specific configuration options accessible via `idf.py menuconfig`
- **`sdkconfig.defaults`**: Default ESP-IDF configuration with WiFi optimizations and SPIRAM support
- **`sdkconfig`**: Generated configuration file (auto-generated, can be committed for reproducible builds)

## Hardware Configuration

### Pin Assignments
- Joystick 1: X-axis (Pin 32), Y-axis (Pin 33), Button (Pin 12)
- Joystick 2: X-axis (Pin 34), Y-axis (Pin 35), Button (Pin 14)
- Battery Monitor: Pin 36 (ADC)

### Key Parameters
- **Communication**: ESP-NOW with configurable receiver MAC address
- **Sampling Rate**: 20ms transmission interval (50Hz)
- **Joystick Calibration**: Dead zone of 50 units, mapped to ±512 range
- **WiFi Channel**: Channel 1, no encryption

## Development Notes

- All code comments and log messages are in English
- Battery level monitoring is implemented but may need calibration for specific battery types
- Receiver MAC address is configurable via `idf.py menuconfig` under "ESP32 Controller Configuration"
- Send delay and joystick deadzone are also configurable through menuconfig
- ESP_LOG is used for debugging output - adjust log level via menuconfig if needed
- Project uses native ESP-IDF APIs for optimal performance and full hardware access
- FreeRTOS task-based architecture allows for easy extension with additional features

## Configuration Parameters

Access these via `idf.py menuconfig` -> "ESP32 Controller Configuration":
- **Receiver MAC Address**: Target device MAC address for ESP-NOW communication
- **Send Delay**: Transmission interval in milliseconds (default: 20ms)
- **Joystick Deadzone**: Input filtering threshold (default: 50)