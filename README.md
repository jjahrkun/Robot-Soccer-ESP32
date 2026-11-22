# Robot Soccer ESP32-C3

Proyek robot soccer 2WD berbasis ESP32-C3 SuperMini dengan sistem kendali menggunakan smartphone melalui Bluetooth Low Energy (BLE).

## Hardware
- ESP32-C3 SuperMini
- MX1508 Dual Motor Driver
- 2x N30 DC Motor
- 2x 18650 Battery
- 2WD Robot Chassis

## Features
- Bluetooth BLE Remote Control
- RemoteXY Dual Joystick
- Differential Drive Control
- PWM Motor Speed Control
- Deadzone Protection
- Startup Motor Protection
- PWM Ramp Control
- Communication Disconnect Protection

## Pin Configuration

| ESP32-C3 | MX1508 |
|---|---|
| GPIO 10 | L_IN1 |
| GPIO 0 | L_IN2 |
| GPIO 1 | R_IN1 |
| GPIO 3 | R_IN2 |

## Software
- Arduino IDE
- ESP32 Arduino Core
- RemoteXY Library 4.1.10+

## Development
Project ini dikembangkan untuk eksperimen sistem kendali robot bergerak, komunikasi nirkabel, dan pengendalian motor DC.
