# FPGA Weather Station

A real-time environmental monitoring system implemented on an Intel Cyclone V FPGA. The system integrates a Nios II softcore processor with multiple sensors to measure temperature, humidity, and barometric pressure. Readings are displayed on an I2C LCD, and configurable thresholds trigger LED and display alerts when environmental conditions exceed user-defined limits.

## Overview
Demonstrates a complete embedded SoC design on an FPGA, combining RTL hardware description with embedded C firmware. Two sensors: DHT11 (temperature/humidity, single-wire) and HP206C (barometric pressure, I2C). A four-line character LCD shows live readings; four push buttons let the user navigate between screens and adjust alert thresholds.

Built using Intel Platform Designer (Qsys) and Quartus Prime 18.1.0. Firmware runs on a Nios II/e softcore processor at 50 MHz using the Nios II HAL.

## Features
- Dual-sensor temperature averaging (DHT11 + HP206C)
- Humidity monitoring via DHT11 single-wire interface with checksum verification
- Barometric pressure monitoring via HP206C over I2C
- Four-line I2C character LCD with real-time sensor data
- Four navigation screens: sensor readings, temperature limit, pressure limit, humidity limit
- Adjustable alert thresholds configured with LEFT/RIGHT buttons
- LED alert system with synchronized LED blink and display flash
- Nios II/e soft processor running bare-metal C at 50 MHz
- Bidirectional open-drain I2C interface at the RTL wrapper level

## Architecture
Top-level: `weather_station_I2C.v` handles bidirectional I2C tri-state logic and instantiates the Platform Designer system. The Qsys system contains Nios II/e CPU, on-chip SRAM, Avalon I2C Master, JTAG UART, System Timer, DHT11 PIO, LED PIOs, and button PIOs.

## Hardware Requirements
- **FPGA:** Cyclone V 5CSEMA4U23C6 (e.g., DE1-SoC)
- **Sensors:** DHT11 (PIN_U14), HP206C barometric sensor (I2C address 0x76)
- **Display:** I2C 4-line LCD (address 0x3C)
- **Buttons:** UP (PIN_U13), DOWN (PIN_AG9), LEFT (PIN_AG8), RIGHT (PIN_AG10)
- **Clock:** 50 MHz on PIN_V11

## FPGA Resource Utilization (Cyclone V 5CSEMA4U23C6)
| Resource | Used | Available | % |
|---|---|---|---|
| ALMs | 1,310 | 15,880 | 8% |
| Block Memory Bits | 811,336 | 2,764,800 | 29% |
| RAM Blocks | 105 | 270 | 39% |
| I/O Pins | 11 | 314 | 4% |

## Project Structure

```
fpga-weather-station/
├── Block_digram.jpeg                           # System block diagram
├── Software_FlowChart.png                      # Firmware flow chart
├── weather_station/
│   ├── weather_station.qpf                     # Quartus project file
│   ├── weather_station.qsf                     # Quartus settings file
│   ├── weather_station.qsys                    # Platform Designer (Qsys) system
│   ├── weather_station_I2C.v                   # Top-level Verilog with I2C tri-state logic
│   ├── clk_clk.sdc                             # Timing constraints
│   └── software/
│       └── code/
│           └── weather_station_firmware.c      # Nios II bare-metal C firmware
└── README.md
```

## Academic Context
Completed as part of academic coursework at Hochschule Anhalt (Anhalt University of Applied Sciences), 2024.
