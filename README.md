# J1939 / J1708 Custom Truck Dashboard

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Platform: ESP32](https://img.shields.io/badge/Platform-ESP32-blue.svg)](https://www.espressif.com/en/products/socs/esp32)

A custom secondary digital dashboard for heavy-duty trucks, supporting both SAE J1939 (CAN bus) and SAE J1708/J1587 (serial) protocols. This project provides real-time engine, transmission, and ABS data display with data logging capabilities.

## 🚛 Project Overview

This project aims to build a comprehensive vehicle data monitoring system that:

- **Reads J1939 CAN data** from engine ECM and transmission TCM (250 kbps)
- **Reads J1708/J1587 serial data** from older ABS modules (9600 bps)
- **Decodes diagnostic trouble codes (DTCs)** from all connected ECUs
- **Displays real-time parameters** on a custom dashboard
- **Logs data** for later analysis and troubleshooting
- **Future: Wi-Fi connectivity** for Home Assistant integration

## 🔧 Hardware

| Component | Model | Purpose |
|-----------|-------|---------|
| MCU | ESP32 DevKit V1 | Main controller with Wi-Fi/Bluetooth |
| CAN Transceiver | Waveshare SN65HVD230 | J1939 CAN bus interface (3.3V) |
| RS485 Transceiver | ATNSINC TTL-RS485 (MAX485) | J1708 serial interface |
| Display | TFT SPI (Phase 4) | Visual dashboard |

## 📋 Documentation

All specifications and reference materials are in the [Docs](./Docs/) folder:

| Document | Description |
|----------|-------------|
| [PROJECT_SPECIFICATION.md](./Docs/PROJECT_SPECIFICATION.md) | Complete engineering specification with phases |
| [HARDWARE_REFERENCE.md](./Docs/HARDWARE_REFERENCE.md) | Detailed hardware wiring and pinouts |
| [J1939_PGN_CATALOG.md](./Docs/J1939_PGN_CATALOG.md) | J1939 PGN/SPN parameter reference |

## 🚀 Implementation Phases

### Phase 0: Pre-Hardware Preparation *(Current)*
- [x] Create project specification
- [x] Document hardware requirements  
- [x] Create J1939/J1708 protocol references
- [x] Create firmware header files with PGN/SPN definitions
- [ ] Set up development environment (PlatformIO)
- [ ] Create simulation framework
- [ ] Implement parser unit tests

### Phase 1: Bench Testing
- [ ] Assemble hardware
- [ ] Test CAN transceiver with loopback
- [ ] Test RS485 transceiver
- [ ] Verify J1939 parsing with simulated data
- [ ] Verify J1708 parsing with simulated data

### Phase 2: Vehicle Integration
- [ ] Connect to truck diagnostic port
- [ ] Verify live CAN data reception
- [ ] Verify live J1708 data reception
- [ ] Compare decoded values with dashboard

### Phase 3: Full Data Logging
- [ ] Expand PGN coverage
- [ ] Implement DTC parsing
- [ ] Add data logging to serial/SD
- [ ] Extended stability testing

### Phase 4: Basic Display
- [ ] Add TFT display
- [ ] Create display layout
- [ ] Show key parameters
- [ ] Vehicle installation

### Phase 5: Advanced Features *(Future)*
- [ ] LVGL graphical UI
- [ ] Wi-Fi/MQTT connectivity
- [ ] Home Assistant integration
- [ ] Remote start capability

## 📊 Supported Parameters

### Engine (via J1939)
- Engine Speed (RPM)
- Engine Load (%)
- Coolant Temperature
- Oil Pressure & Temperature
- Boost Pressure
- Fuel Rate
- Battery Voltage
- Engine Hours

### Transmission (via J1939)
- Current Gear
- Transmission Temperature
- Torque Converter Lockup
- Output Shaft Speed

### Vehicle (via J1939)
- Vehicle Speed
- Fuel Level
- Ambient Temperature
- Odometer

### Diagnostics
- Active DTCs (J1939 DM1)
- Stored DTCs (J1939 DM2)
- ABS Faults (J1708/J1587)

## 🔌 Pin Connections

```
ESP32 DevKit V1 Pin Mapping:
├── CAN Bus (J1939)
│   ├── GPIO 5  → SN65HVD230 CTX
│   └── GPIO 4  → SN65HVD230 CRX
├── J1708 Serial
│   ├── GPIO 17 → RS485 DI
│   ├── GPIO 16 → RS485 RO
│   └── GPIO 25 → RS485 DE/RE
└── Analog Inputs (ADC1 - WiFi safe)
    ├── GPIO 36 → Fuel Sensor 1
    ├── GPIO 39 → Fuel Sensor 2
    └── GPIO 35 → Dimmer Input
```

## 🔗 References

- [ESP32 TWAI Driver Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/twai.html)
- [J1939 Explained - CSS Electronics](https://www.csselectronics.com/pages/j1939-explained-simple-intro-tutorial)
- [Waveshare SN65HVD230 Wiki](https://www.waveshare.com/wiki/SN65HVD230_CAN_Board)
- [Copperhill J1708 Guide](https://copperhilltech.com/blog/monitoring-sae-j1708j1587-data-traffic-using-the-arduino-mega2560-or-arduino-due/)

## 📄 License

This project is licensed under the MIT License - see the LICENSE file for details.

## ⚠️ Disclaimer

This project is for educational and personal use. Modifying vehicle systems may void warranties or affect vehicle safety. Always ensure proper testing before using in a real vehicle. The authors are not responsible for any damage or safety issues resulting from use of this project.