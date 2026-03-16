# Big Air Scoring System

An embedded scoring system for Big Air snowboarding/skiing tricks, built for the **Raspberry Pi Pico**. It uses a BNO085 IMU to track rotation in real time and calculates a score based on trick type, rotation accuracy, landing precision, and airtime.

---

## Table of Contents

- [Overview](#overview)
- [Hardware Requirements](#hardware-requirements)
- [Wiring](#wiring)
- [Repository Structure](#repository-structure)
- [Trick Modes & Scoring](#trick-modes--scoring)
- [Button Controls](#button-controls)
- [Screen States](#screen-states)
- [Building & Flashing](#building--flashing)
- [Dependencies](#dependencies)

---

## Overview

The system continuously polls a BNO085 9-DOF IMU over I2C, accumulating relative rotations (yaw, pitch, roll) using quaternion delta integration. When a jump is started and stopped with the Red button, the system calculates a final score and displays it on a 128×64 SSD1306 OLED display. Up to 50 scores are stored per session and the top 3 are shown on the Scores screen.

---

## Hardware Requirements

| Component | Details |
|---|---|
| Microcontroller | Raspberry Pi Pico |
| IMU | BNO085 (I2C address `0x4B`) |
| Display | SSD1306 OLED 128×64 (I2C address `0x3C`) |
| Buttons | 4× momentary push buttons (White, Red, Blue, Green) |

---

## Wiring

### I2C Bus (shared by IMU and display)

| Signal | Pico GPIO |
|---|---|
| SDA | GP20 |
| SCL | GP21 |

### Buttons

| Button | Pico GPIO |
|---|---|
| White (Reset) | GP3 |
| Red (Record) | GP7 |
| Blue (Mode cycle) | GP11 |
| Green (Scores) | GP14 |
| Interrupt line | GP13 |

All button GPIO pins use internal pull-downs. Connect each button between its GPIO pin and 3.3 V. The interrupt line (GP13) is driven high by whichever button is currently pressed and is used to debounce inputs in the GPIO callback.

---

## Repository Structure

```
BigAirScoringSystem/
├── BigAirScoringSystem.cpp   # Main entry point: hardware init, ISRs, main loop
├── CMakeLists.txt            # Top-level CMake build
├── pico_sdk_import.cmake     # Pico SDK cmake helper
├── modules/
│   ├── CMakeLists.txt
│   └── src/
│       ├── calculations.cpp  # IMU relative tracking & score calculation
│       ├── calculations.h
│       ├── displays.cpp      # SSD1306 screen rendering functions
│       └── displays.h
├── BNO08x_Pico_Library/      # BNO085 driver (submodule/vendored)
└── pico_ssd1306/             # SSD1306 OLED driver (submodule/vendored)
```

### `BigAirScoringSystem.cpp`

- Initialises the hardware (I2C, GPIO buttons, manual 1 ms timer interrupt).
- Handles button debouncing via a GPIO edge-triggered ISR (`gpio_callback`). Presses shorter than 50 ms are filtered as noise.
- Runs a concurrent main loop that polls the IMU at full rate and refreshes the display every 200 ms.

### `modules/src/calculations.cpp`

- `updateRelativeTracking()` — integrates incremental quaternion deltas each IMU frame to accumulate body-frame rotation around each axis, avoiding gimbal lock.
- `calculateScore()` — computes the final score from trick base points, rotation bonus, landing accuracy, and airtime (see [Trick Modes & Scoring](#trick-modes--scoring)).

### `modules/src/displays.cpp`

- `showWelcomeScreen()` — idle screen showing the currently selected mode.
- `showRecordScreen()` — live display of accumulated yaw, pitch, and roll during a jump.
- `showResultScreen()` — breakdown of the score (trick, rotation, landing, airtime) after a jump.
- `showResetMsg()` — brief confirmation displayed on a system reset.
- `showScoresScreen()` — top 3 scores from the current session.

---

## Trick Modes & Scoring

Select a mode with the **Blue** button before recording a jump.

| Mode | Trick | Base Points | Primary Axis |
|---|---|---|---|
| 0 | Flat Spin | 15 | Yaw |
| 1 | Backflip | 20 | Pitch |
| 2 | Frontflip | 25 | Pitch |
| 3 | Side Flip | 30 | Roll |

### Score Breakdown

| Component | Range | Notes |
|---|---|---|
| Trick (base) | 15 – 30 | Fixed per mode (see table above) |
| Rotation bonus | 0 – 30 | 10 pts for a 360°, +5 pts per additional 180°, capped at 30 |
| Landing accuracy | 0 – 20 | Awarded when total rotation reaches at least 180° and lands within 20° of a multiple of 180° (180°, 360°, 540°, …) |
| Airtime | 0 – 20 | 1 pt per second, capped at 20 |

A *fall* (landing outside ±20° of any 180° mark, or less than 180° total rotation) awards only the base trick points with no rotation, landing, or airtime bonus.

**Maximum possible score: 95 pts** (30 base + 30 rotation + 20 landing + 20 airtime — with Side Flip at ≥ 1440°)

---

## Button Controls

| Button | Action |
|---|---|
| **White** | Full system reset — clears scores and returns to Welcome screen |
| **Blue** | Cycle trick mode (0 → 1 → 2 → 3 → 0). Only works when not recording |
| **Red** | Start recording a jump (press once); stop recording (press again) |
| **Green** | From Welcome screen: open the Scores screen |

---

## Screen States

```
WELCOME  ──[RED]──►  RECORDING  ──[RED]──►  RESULT
   ▲                                            │
   └────────────────────────────────────────────┘
   │                                         (any btn)
   └──[GREEN]──►  SCORES  ──(any btn)──►  WELCOME
   
[WHITE at any time] ──► WELCOME (full reset)
```

---

## Building & Flashing

### Prerequisites

- [Raspberry Pi Pico SDK](https://github.com/raspberrypi/pico-sdk) (v2.2.0)
- CMake ≥ 3.13
- ARM GCC toolchain (`arm-none-eabi-gcc`)
- `picotool` (optional, for flashing)

### Build

```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

This produces `BigAirScoringSystem.uf2` in the `build/` directory.

### Flash

1. Hold the **BOOTSEL** button on the Pico while connecting it to USB.
2. It will appear as a USB mass-storage device (`RPI-RP2`).
3. Copy `BigAirScoringSystem.uf2` to the drive. The Pico will reboot and run the firmware automatically.

### Serial Monitor (optional)

USB stdio is enabled. Connect at **115200 baud** to see debug output including button press confirmations and IMU status messages.

---

## Dependencies

| Library | Purpose | Location |
|---|---|---|
| [pico-sdk](https://github.com/raspberrypi/pico-sdk) | Raspberry Pi Pico SDK | Installed separately |
| [BNO08x_Pico_Library](https://github.com/myles-parfeniuk/BNO08x_Pico_Library) | BNO085 IMU driver | `BNO08x_Pico_Library/` |
| [pico-ssd1306](https://github.com/daschr/pico-ssd1306) | SSD1306 OLED driver | `pico_ssd1306/` |
