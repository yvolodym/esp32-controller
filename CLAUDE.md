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

`ControllerData` struct (sent over ESP-NOW):
```c
typedef struct {
    int16_t joy1_x, joy1_y;
    int16_t joy2_x, joy2_y;
    bool joy1_btn, joy2_btn;
    uint8_t batteryLevel;
} ControllerData;
```

`batteryLevel` is derived in software from a 220 kΩ / 100 kΩ divider on `HVBAT`
(R1/R2/C1 in `supply.kicad_sch`) feeding GPIO36 / ADC1_CH0 (SENSOR_VP).
Mapping is linear: 4200 mV → 100 %, 3000 mV → 0 %.

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

### Joysticks (resistive / potentiometer-based)

Physical placement (from schematic): **U2 = right stick**, **U3 = left stick**.
The firmware names `joy1` = U2 (right) and `joy2` = U3 (left); the display
labels `RIGHT` (line for joy1) and `LEFT` (line for joy2) match this mapping.

| Signal | GPIO | WROVER pin name | Module pin # | ADC |
|---|---|---|---|---|
| Joystick 1 (RIGHT, U2) X — net `VRx` | GPIO32 | GPIO32 | 8 | ADC1_CH4 |
| Joystick 1 (RIGHT, U2) Y — net `VRy` | GPIO33 | GPIO33 | 9 | ADC1_CH5 |
| Joystick 1 (RIGHT, U2) Button | GPIO25 | GPIO25 | 10 | — |
| Joystick 2 (LEFT,  U3) X — net `HRx` | GPIO34 | GPIO34 | 6 | ADC1_CH6 |
| Joystick 2 (LEFT,  U3) Y — net `HRy` | GPIO35 | GPIO35 | 7 | ADC1_CH7 |
| Joystick 2 (LEFT,  U3) Button | GPIO26 | GPIO26 | 11 | — |

Buttons use internal pull-up; active-low logic is inverted in software.

> **Joystick buttons are not wired in hardware (status 2026-06-03).** All four
> SEL+/SEL− pins on U2 and U3 carry explicit `no_connect` markers, and the
> matching ESP32 pins (GPIO25 pin 10, GPIO26 pin 11) are also marked
> `no_connect` on U1. The firmware still configures both GPIOs as inputs with
> internal pull-ups and reads them every cycle, so they always report HIGH
> (i.e. "not pressed"). The button logic in software is correct and ready —
> only the schematic/PCB is missing the routes.

> SENSOR_VN (GPIO39, module pin 5) is **not connected** in the schematic —
> explicit `no_connect` marker at U1. SENSOR_VP (GPIO36, module pin 4) is the
> ADC tap of the VBAT divider (R1/R2/C1, net `HVBAT`). Earlier firmware
> versions read joy1 from GPIO36/39, which produced floating ADC values and a
> stick that "did not react"; fixed by moving joy1 to GPIO32/33 (the pins the
> schematic actually wires to U2).

#### Measured deflection range (post-calibration, after boot zero-reference)

The firmware maps each axis to nominal **−512…+512** with a deadzone around
zero (see `calibrateJoystick` in `main/main.c`). Measured per-axis full-swing
extremes on the current hardware (resistive thumbstick modules with two
potentiometers per stick) are:

| Joystick | X min | X max | Y min | Y max |
|---|---|---|---|---|
| RIGHT (joy1, U2) | −464 | +453 | −476 | +441 |
| LEFT  (joy2, U3) | −479 | +439 | −461 | +457 |

These are useful for downstream model/vehicle control: the sticks don't reach
the theoretical ±512 limit, so any actuator-mapping curve should clamp/scale
against the table above rather than ±512. Values are asymmetric (offset of
~10–20 counts between positive and negative directions) — apply per-direction
scaling if symmetric output is required.

### ST7789 Display (SPI / VSPI)

8-pin header J1 (`Conn_01x08_Pin`, schematic location top-of-board):

| J1 pin | Signal | GPIO |
|---|---|---|
| 1 | Backlight | GPIO15 |
| 2 | CS  | GPIO5  |
| 3 | DC  | GPIO2  |
| 4 | RST | GPIO4  |
| 5 | MOSI / SDA | GPIO23 |
| 6 | SCK / SCL  | GPIO18 |
| 7 | VCC | +3.3 V |
| 8 | GND | GND |

## Power Supply Design (supply.kicad_sch)

Automatic USB/battery switching — no manual intervention required. Reference
designators below match the current schematic exactly (verified by walking
`supply.kicad_sch` 2026-06-03).

### Requirements

1. **USB connected** → ESP32 is powered from USB **and** the Li-ion cell is charged.
2. **USB disconnected** → ESP32 is powered from the Li-ion cell (NCR18650B).
3. The crossover between USB and battery must happen **automatically**, with **no external control** (no switch, no MCU GPIO, no jumper). It must be fully analog/self-contained on the supply schematic.

### Topology

```mermaid
graph LR
    USB["USB 5V (+5V)"]
    VBAT["VBAT\nNCR18650B"]
    VPWR(["VPWR"])
    V33(["＋3.3 V"])

    USB  -->|"D5 SS14\nVf ≈ 0.3 V"| VPWR
    VBAT -->|"Q3 Si2319CDS (LTC4412 U12)\nVds ≈ 20 mV"| VPWR
    VPWR -->|"U11 AP2112K-3.3\nLDO 600 mA · 250 mV dropout"| V33
    USB  -->|"U5 TP4056\nCC/CV 1 A charger"| VBAT
```

Q3 is oriented per the LTC4412 datasheet (Figure 1): **drain → VBAT (battery
side), source → VPWR (load side)**. Its intrinsic drain-to-source body diode
therefore points battery → load, so it is *reverse-biased* whenever USB pulls
VPWR above VBAT — no current can be injected back into the battery and the
TP4056 charge path is never bypassed. This makes the earlier inline Schottky
**D6 unnecessary; it has been removed** (2026-06-03), recovering ~0.3 V of
battery-path headroom. See voltage budget below.

> **History:** an earlier revision had Q3 wired backwards (source → battery,
> drain → load). That reversed the body diode (load → battery), which *would*
> inject USB current into the cell and bypass the charger, so a series Schottky
> D6 was added to block it. Flipping Q3 to the canonical orientation eliminates
> the root cause and the extra ~0.3 V drop. Do **not** re-introduce D6 unless Q3
> is reverted.

- **When USB present**: D5 conducts (VPWR ≈ +5 V − Vf ≈ 4.7 V), the LTC4412 senses VPWR > VBAT and turns Q3 off → battery isolated; Q3 body diode reverse-biased.
- **When USB absent**: LTC4412 turns Q3 on, current flows VBAT → Q3.D → Q3.S → VPWR (≈ VBAT − Vds ≈ VBAT − 20 mV); D5 is reverse-biased.

### Components

| Ref | Part | Package | Function |
|---|---|---|---|
| U5  | TP4056-42-ESOP8 | SOIC-8 EP | Li-ion charger, 1 A CC/CV, 4.2 V |
| U6  | USBLC6-2P6 | SOT-23-6 | USB D+/D− ESD protection |
| U7  | 8205A | SOT-23-6 | Dual N-MOSFET, battery protection switch |
| U8  | DW01A | SOT-23-6 | Single-cell Li-ion protection IC (drives U7) |
| U9  | tactile switch (small) | — | (mechanical, see schematic) |
| U10 | NCR18650B holder | — | 18650 Li-ion cell, 3.6 V nominal, 4.2 V max |
| U11 | AP2112K-3.3 | SOT-23-5 | LDO 3.3 V, 600 mA, 250 mV dropout |
| U12 | LTC4412ES6 (marking **LTA2**) | TSOT-23-6 | PowerPath controller |
| Q3  | Si2319CDS | SOT-23 | P-ch MOSFET (VDS −40 V, ID −4.4 A, RDS_on ≈ 100 mΩ at VGS −10 V) |
| D5  | SS14 | SMA | USB-side Schottky (VBUS → VPWR) |
| ~~D6~~ | ~~SS14~~ | — | **Removed 2026-06-03** — Q3 reoriented per datasheet, body diode now blocks reverse current natively |
| D3  | LED red | 0603 | TP4056 ~CHRG indicator (lit while charging) |
| D4  | LED blue | 0603 | +3.3 V power-on indicator |

> Q1 and Q2 (BCW66G NPN BJTs) live on the **main** schematic and are part of
> the CH340C UART auto-reset circuit. The supply schematic only contains Q3.

### LTC4412 (U12) wiring
- **VIN (pin 1) → VBAT** — the chip is powered from the battery so it stays alive when USB is absent.
- **GND (pin 2) → GND**.
- **CTL (pin 3) → GND** — CTL is active-low; tying it high *forces* the PFET off (V_IH ≥ 0.9 V per datasheet).
- **STAT (pin 4) → not connected**.
- **GATE (pin 5) → Q3 gate** (net `Q1_GATE` — the name is historical, this is Q3's gate, not Q1's).
- **SENSE (pin 6) → VPWR** — load side of Q3 (Q3 **source**). The chip switches Q3 off when V_SENSE − V_IN > 20 mV, which is how it detects "USB has pulled VPWR above VBAT".
- PWR_FLAG `#FLG04` on VPWR (declares the rail as externally supplied for ERC).

### Q3 PMOS (Si2319CDS) — canonical LTC4412 orientation
- **Drain (pin 3) → VBAT / `HVBAT`** (battery side, direct — no series diode).
- **Source (pin 2) → VPWR** (load side).
- Gate (pin 1) → `Q1_GATE` (driven by U12 GATE).
- The drain-to-source body diode (anode = drain = battery, cathode = source =
  load) points battery → load: forward only while the battery supplies the
  load, reverse-biased the instant USB raises VPWR above VBAT. This is the exact
  wiring the LTC4412 datasheet calls for, and is why no battery-side Schottky is
  needed.

### Why AP2112K instead of AMS1117
AMS1117 has 1.3 V dropout — cannot regulate from a depleted Li-ion cell
(needs ≥ 4.6 V in). AP2112K has 250 mV dropout — usable across the cell
range. With D6 removed the only series element in the battery → load path is
Q3 (R_DS(on) ≈ 100 mΩ), so the dropout margin below is now ~0.3 V better than
the earlier D6-inline design.

### Battery-path voltage budget

VPWR (battery side) = VBAT − Vds(Q3) ≈ VBAT − 0.02 V at light load (D6 removed).

| VBAT | VPWR | AP2112K +3.3V output | Notes |
|---|---|---|---|
| 4.20 V (full) | ≈ 4.18 V | 3.30 V (in regulation) | margin 880 mV |
| 3.70 V (nominal) | ≈ 3.68 V | 3.30 V (in regulation) | comfortable margin |
| 3.30 V (low) | ≈ 3.28 V | ≈ 3.03 V (in dropout) | just above ESP32 brown-out (~3.0 V) |
| 3.10 V (depleted) | ≈ 3.08 V | ≈ 2.83 V | below brown-out, system shuts down |

The previous design had a battery-side Schottky (D6) in series with Q3, costing
~0.3 V of usable battery range. It has been removed: orienting Q3 per the
LTC4412 datasheet (drain → battery, source → load) makes the body diode block
reverse current natively, so no series diode and no back-to-back PMOS pair is
required. The single Q3 (R_DS(on) ≈ 100 mΩ) is now the only series element.

### TP4056 (U5) notes
- TEMP (pin 1) → VCC — NTC disabled by design (TEMP = GND would permanently disable charging).
- CE (pin 8) → VCC — charger always enabled when VBUS is present.
- ~CHRG (pin 7) → D3 (red LED) via R10 to +5 V — lit while charging.
- ~STDBY (pin 6) → no-connect — no charge-complete LED is wired in this design.
- EPAD (pin 9) → GND.
- VBUS / VCC (pin 4) → +5 V via the USB connector.
- BAT (pin 5) → VBAT through the DW01A + 8205A protection cell.
- USB-C connector P1: CC1/CC2 each have a 5.1 kΩ pull-down to GND (sink-mode requirement).

### Battery protection cell (DW01A U8 + 8205A U7)
Textbook single-cell Li-ion protection in the cell-negative path (verified
against the exported netlist 2026-06-03). Cell + (`U10.1`) ties straight to
`HVBAT`/pack+; the two series N-FETs sit in the negative return:

```
Zelle− (U10.2) ─ U9(switch) ─ S2(B−) ─┤8205A FET2├─ common drain ─┤FET1├─ S1 ─ GND(P−)
DW01A: VCC ← R18(100Ω) ← HVBAT · GND = S2(B−) · CS ← R17(1k) ← GND(P−)
       OC → G1(FET1, overcharge) · OD → G2(FET2, overdischarge) · TD = NC
```

- **R18 = 100 Ω** — DW01A VCC supply resistor (B+ → VCC). ✓
- **R17 = 1 kΩ** — overcurrent sense resistor (CS → P−/GND). ✓
- **C7 = 100 nF** — DW01A VCC↔GND bypass. *(Was 100 pF in the schematic — a
  ~1000× undersized bypass that barely decouples the IC and risks nuisance
  protection trips; corrected to 100 nF 2026-06-03.)*
- **U7 8205A** — common-drain pair: `D1`/`D2` tied internally only (correct for
  this config — the shared drain node deliberately goes nowhere else).
- The `OC→G1 / OD→G2` mapping and `GND=B− / CS=P−` references match the DW01A
  datasheet application circuit.

> TODO(supply): **U9 master switch sits in the cell-negative (B−) path**
> (`U10.2 → U9 → S2`). It works as a hard battery cutoff, but (a) the full
> load *and* charge current runs through the mechanical contacts, and (b) with
> U9 open the TP4056 charge return is also interrupted, so the pack cannot
> charge while switched off. Consider relocating the on/off switch to VPWR or
> to the LTC4412 CTL pin instead. U9 pin 1 is unconnected (fine for SPST use).

### VBAT sense to MCU
A 220 kΩ / 100 kΩ divider (R1/R2) with 100 nF filter (C1) taps `HVBAT` and
feeds GPIO36 (SENSOR_VP, ADC1_CH0) on U1. `HVBAT` is the same net as VBAT,
brought across as a hierarchical sheet pin. The firmware's `read_battery_level`
recovers VBAT from the divider, smooths it with an EMA, and maps it through a
LiPo discharge curve to a 0–100 % estimate.

**Tap point:** the divider's top sits on net `/HVBAT`, whose members are
`U10.1` (the raw 18650 + terminal), `U5.5` (TP4056 BAT), `U12.1` (LTC4412 VIN),
`Q3.3` (Q3 drain — direct battery connection after the D6 removal) and the
DW01A supply resistor. The mid-tap goes to `U1.4` (SENSOR_VP), and the bottom
leg + filter cap go to GND.

> **Note:** the supply sheet was re-annotated on 2026-06-03; the R/C reference
> designators shifted from the original layout. Re-verified against the current
> schematic 2026-06-07: divider = **R1 (220 k) / R2 (100 k)**, filter = **C1
> (100 nF)** (the old `C9` is now a 100 nF decoupling cap on `+5V`); DW01A VCC
> supply resistor = **R18 (100 Ω)** (was R16), CS resistor = **R17 (1 k)** (was
> R15), VCC bypass = **C7 (100 nF)** (was C6). The mapping below is current.

> **The sense taps the raw cell, *upstream* of the powerpath — Q3 is NOT in the
> measured path.** The discharge path is now `HVBAT → Q3 → VPWR` (D6 removed),
> so Q3 R_DS(on) and the AP2112K dropout are all *load-side* of the tap and do
> not affect the reading. The only error sources in the measured loop are
> the cell's own internal resistance and the IR drop across the low-side
> protection FETs (8205A, in the GND return) — both tens of mV. A genuine
> concern *is* the divider's high Thévenin source impedance (R1‖R2 ≈ 69 kΩ,
> above Espressif's ADC recommendation), but C1 (100 nF) buffers the sample/hold
> so this is acceptable for the slow battery poll. Net: the 50 %→85 % boot
> climb is a measurement/mapping artifact, not a real powerpath collapse — hence
> the firmware-side EMA + boot-settle + non-linear curve in `read_battery_level`.

### ERC pin-type tweaks (embedded library copies)
A few stock symbols had `pin_type` annotations that triggered ERC false
positives once the supply network was complete. The **embedded** copies in
the schematic file have been adjusted (the upstream libraries are unchanged,
hence the resulting `lib_symbol_mismatch` warnings are expected):
- `Simulation_SPICE:NPN` — Q1/Q2 collector & emitter `open_collector` / `open_emitter` → `passive`
- `PCM_yvolodym:8205A` — D1/D2 drains `output` → `passive`
- `Interface_USB:CH340C` — V3 pin `power_output` → `power_input` (CH340C runs in self-powered mode with V3 tied to VCC externally)

## Key Notes

- ADC uses `esp_adc/adc_oneshot` API with automatic calibration (curve fitting preferred, falls back to line fitting, then raw).
- WiFi is initialized in STA mode solely to enable the ESP-NOW radio — no network association.
- Battery level is computed from the GPIO36 / `HVBAT` divider, smoothed with an EMA and mapped through a non-linear LiPo discharge curve (see *Power Supply Design → VBAT sense to MCU*).
- `sdkconfig.defaults` enables SPIRAM, 240 MHz CPU, 80 MHz flash/SPIRAM, and WiFi buffer tuning.
- Hardware reference datasheets are in `doc/`.
