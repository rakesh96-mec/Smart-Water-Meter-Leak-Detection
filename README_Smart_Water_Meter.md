# Smart Water Meter – Automatic Reading & Leak Detection

An embedded smart water-meter retrofit developed at Hochschule Ravensburg-Weingarten (RWU) for automatic water-volume measurement, real-time leak detection, and live monitoring through a web dashboard.

## Overview

Conventional mechanical water meters provide cumulative readings but no direct electrical interface for automated monitoring. This project uses an incremental sensing approach: four magnets mounted on an internal meter gear generate pulses detected by a Hall-effect sensor. An STM32F446RE processes the pulses, calculates water consumption and flow rate, evaluates leak conditions, and communicates with a Python/Flask dashboard over UART.

The documented implementation is **Version 2 (v2)**, which adds a five-state leak-detection finite-state machine, bidirectional UART communication, a rolling history buffer, a live trend chart, and remote leak-status reset.

## System Architecture

```text
Mechanical Water Meter
        |
        | Gear rotation
        v
4 × Magnets on gear
        |
        v
Hall-Effect Sensor
        |
        | Falling-edge pulses
        v
STM32F446RE Nucleo
        |
        | USART2 / 115200 baud
        v
Python Flask Dashboard
        |
        +--> Live measurements
        +--> Reading history
        +--> Trend chart
        +--> Remote reset
```

## Hardware

- STM32F446RE Nucleo development board
- A3144 Hall-effect sensor
- Mechanical domestic water meter
- 4 neodymium disc magnets
- Onboard LED for pulse indication
- USB/ST-Link interface

### Key connections

| Signal | STM32 pin |
|---|---|
| Hall sensor | PA0 |
| Pulse indicator LED | PA5 |
| USART2 TX | PA2 |
| USART2 RX | PA3 |

The Hall sensor input uses a falling-edge EXTI interrupt with an internal pull-up. USART2 operates at 115200 baud, 8-N-1.

## Measurement Principle

Four magnets are mounted on the gear connected to the meter's fast-moving needle. Each magnet produces one falling-edge pulse.

```text
4 pulses = 1 gear revolution = 1 litre
1 pulse = 250 mL
```

The firmware therefore calculates cumulative volume as:

```text
Volume [mL] = pulseCount × 250
```

Flow rate is estimated from the number of new pulses detected during each one-second processing cycle.

## Leak Detection

The firmware classifies flow into normal, minor-monitoring, and major-monitoring regions and uses a five-state finite-state machine:

```text
NORMAL
  |
  +--> MINOR MONITORING --> MINOR LEAK
  |
  +--> MAJOR MONITORING --> MAJOR LEAK
```

Current v2 parameters:

| Parameter | Value |
|---|---:|
| Design flow rate | 60,000 mL/min |
| Minor threshold | 30,000 mL/min |
| Major threshold | 45,000 mL/min |
| Confirmation time | 10 s |

A transient high-flow condition is therefore monitored before a major leak is confirmed. Zero flow immediately returns the system to `NORMAL`.

## UART Communication

The STM32 sends one CSV record approximately every second:

```text
<pulseCount>,<volume_mL>,<statusText>
```

Examples:

```text
4,1000,NORMAL
9,2250,MINOR MONITORING
20,5000,MINOR LEAK
300,75000,MAJOR MONITORING
350,87500,MAJOR LEAK
```

The dashboard can send the character `R` back to the STM32 to clear a confirmed leak state without restarting the board.

## Web Dashboard

The Flask application provides:

- Live pulse count
- Cumulative volume
- Current leak status
- Real-time trend chart using Chart.js
- Rolling history of the latest 200 readings
- Remote reset control
- JSON endpoints for live data and history

Endpoints:

```text
/
/data
/history
/reset   (POST)
```

## Software

### Embedded

- C
- STM32 HAL
- STM32CubeMX / STM32CubeIDE
- External interrupt (EXTI)
- USART2

### Dashboard

- Python
- Flask
- PySerial
- HTML / CSS / JavaScript
- Chart.js

## Repository Structure

```text
Smart-Water-Meter-Leak-Detection/
├── stm32_water_meter_firmware.c
├── water_meter_stm32.ioc
├── water_meter_dashboard.py
├── requirements.txt
└── README.md
```

## Running the Dashboard

Install the Python dependencies:

```bash
pip install -r requirements.txt
```

Connect the STM32 board through USB and make sure the serial-port setting in `water_meter_dashboard.py` matches the port assigned by the operating system.

Then run:

```bash
python water_meter_dashboard.py
```

Open:

```text
http://localhost:5000
```

The current source uses `COM6` as the serial-port value and `115200` baud.

## Testing

The final report documents tests for:

- Standby / no-flow condition
- Normal water usage
- Brief high-flow spikes
- Persistent minor flow
- Persistent high flow
- Recovery after flow stops
- Remote reset after a confirmed leak

The documented tests showed immediate monitoring-state transitions, approximately 10-second confirmation for persistent minor/major leak conditions, immediate recovery to `NORMAL` after flow stopped, and remote reset in less than one second.

## Limitations

The v2 implementation has several documented limitations:

- History is stored in server memory and is lost when Flask restarts.
- The serial port is hardcoded and should be made configurable for portability.
- The system measures incremental volume and requires a known starting value; it does not independently determine the absolute mechanical meter reading.
- The reset endpoint has no authentication and is intended for a local-network prototype.

## Future Development

The final project report identifies possible extensions including persistent data storage, authenticated reset control, OCR-based absolute meter reading, wireless/MQTT communication, an RTC, alert notifications, and configurable leak thresholds.

## Project Context

This project was developed as a team-based 4 ECTS practical module at RWU. The repository focuses on the implemented embedded firmware and monitoring software rather than reproducing the complete academic report.

