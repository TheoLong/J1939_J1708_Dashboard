# Custom Truck Data Logging & Display Dashboard
## Engineering Project Specification

**Version:** 2.0  
**Date:** January 22, 2026  
**Status:** Draft - Pre-Hardware Development Phase  
**Last Updated:** January 22, 2026 - Major enhancements for test-driven development

---

## Table of Contents

1. [Executive Summary](#1-executive-summary)
2. [Project Goals & Objectives](#2-project-goals--objectives)
3. [System Architecture](#3-system-architecture)
4. [Hardware Specification](#4-hardware-specification)
5. [Protocol Specifications](#5-protocol-specifications)
6. [Software Architecture](#6-software-architecture)
   - 6.3.5 [Persistent Storage Module (NVS)](#635-persistent-storage-module-nvs)
7. [Pin Mapping & Wiring Diagrams](#7-pin-mapping--wiring-diagrams)
   - 7.5 [Test Data Sources & Simulation Framework](#75-test-data-sources--simulation-framework)
   - 7.6 [Watch List Manager](#76-watch-list-manager-display-parameter-selection)
8. [Phase Implementation Plan](#8-phase-implementation-plan)
   - Phase 0: [Pre-Hardware Preparation](#phase-0-pre-hardware-preparation) (3-4 weeks)
   - Phase 1: [Bench Testing](#phase-1-bench-testing-with-simulated-data) (1-2 weeks)
   - Phase 2: [Vehicle Integration](#phase-2-vehicle-integration--initial-testing) (2-3 weeks)
   - Phase 3: [Complete Data Logging](#phase-3-complete-data-logging--decoding) (2-3 weeks)
   - Phase 4: [Display Integration](#phase-4-basic-display-integration) (2-3 weeks)
   - Phase 5: [Advanced Features](#phase-5-advanced-features-future) (ongoing)
9. [Data Parameters Catalog](#9-data-parameters-catalog)
   - 9.6 [Stored/Computed Parameters](#96-storedcomputed-parameters)
10. [Testing Strategy](#10-testing-strategy)
11. [Risk Assessment & Mitigation](#11-risk-assessment--mitigation)
12. [Resource Budget & Constraints](#12-resource-budget--constraints)
13. [Future Expansion](#13-future-expansion)
14. [References & Resources](#14-references--resources)
15. [Appendices](#appendix-a-glossary)
   - A: Glossary
   - B: Troubleshooting Guide
   - C: Debug & Logging Strategy

---

## 1. Executive Summary

### 1.1 Project Overview

This project aims to build a **secondary digital dashboard** for a heavy-duty truck that supports both **SAE J1939 (CAN bus)** and **SAE J1708 (serial)** data links. The system will use an ESP32-based microcontroller to read and decode engine, transmission, and ABS data from the truck's electronic control modules (ECMs), providing more insight than the stock dashboard.

### 1.2 Quick Reference Card

| Aspect | Value |
|--------|-------|
| **MCU** | ESP32 DevKit V1 (Dual-core 240MHz, 520KB RAM, 4MB Flash) |
| **J1939 Interface** | Waveshare SN65HVD230 CAN Board (3.3V, 250kbps) |
| **J1708 Interface** | ATNSINC TTL-RS485 (MAX485, 9600bps) |
| **Key Pins** | CAN: GPIO4/5, UART2: GPIO16/17, RS485_DE: GPIO25 |
| **Core PGNs** | 61444 (RPM), 65262 (Temp), 65265 (Speed), 65226 (Faults) |
| **Total Timeline** | 12-15 weeks to functional dashboard |
| **Phase 0 Focus** | Parser library development with test data (no hardware needed) |

### 1.3 Key Deliverables

- Real-time display of engine, transmission, and vehicle parameters
- Diagnostic trouble code (DTC) reading from all ECUs
- Data logging capability for analysis and troubleshooting
- Persistent trip/MPG data that survives power cycles
- Future-ready architecture for Home Assistant integration and remote start

### 1.4 Target Vehicle Configuration

| Component | Protocol | Notes |
|-----------|----------|-------|
| Engine ECM | J1939 (CAN) | Cummins or similar diesel |
| Transmission TCM | J1939 (CAN) | Allison 1000/3000/4000 series |
| ABS Module | J1708/J1587 | Older style serial bus |

---

## 2. Project Goals & Objectives

### 2.1 Primary Goals

1. **Capture J1939 CAN-Bus data** - Engine parameters, transmission status, vehicle speed
2. **Capture J1708 serial data** - ABS module diagnostics and fault codes
3. **Interface additional analog sensors** - Auxiliary fuel tank levels, dimmer, EGT, ambient temps
4. **Real-time display** - Key parameters visible to driver
5. **Data logging** - Record sessions for later analysis
6. **Persistent storage** - Trip data, fuel economy, and fault codes survive power cycles

### 2.2 Secondary Goals

- Wi-Fi connectivity for future Home Assistant integration
- Remote monitoring capability
- Remote start integration (future phase)
- Web-based dashboard interface

### 2.3 Success Criteria

| Criterion | Measurement |
|-----------|-------------|
| J1939 Data Capture | Successfully decode 90%+ of standard PGNs |
| J1708 Data Capture | Read ABS fault codes reliably |
| System Stability | Run 8+ hours without crashes/resets |
| Display Update Rate | Refresh critical data at least 2Hz |
| Power Consumption | < 500mA at 12V during operation |
| Data Persistence | Trip/MPG data retained after power cycle |

---

## 3. System Architecture

### 3.1 High-Level Block Diagram

```
┌─────────────────────────────────────────────────────────────────────────┐
│                           TRUCK BUS SYSTEMS                              │
├─────────────────────────────────┬───────────────────────────────────────┤
│   J1939 CAN Bus (250kbps)       │   J1708 Serial Bus (9600bps)          │
│   ┌─────────┐ ┌─────────┐       │   ┌─────────┐                         │
│   │ Engine  │ │  Trans  │       │   │   ABS   │                         │
│   │   ECM   │ │   TCM   │       │   │  Module │                         │
│   └────┬────┘ └────┬────┘       │   └────┬────┘                         │
│        │           │            │        │                              │
│        ▼           ▼            │        ▼                              │
│   ┌─────────────────────┐       │   ┌─────────────────────┐             │
│   │   CAN H / CAN L     │       │   │   J1708+ / J1708-   │             │
│   └──────────┬──────────┘       │   └──────────┬──────────┘             │
└──────────────┼──────────────────┴──────────────┼────────────────────────┘
               │                                  │
               ▼                                  ▼
┌──────────────────────────┐     ┌──────────────────────────┐
│  SN65HVD230 CAN Board    │     │  TTL-RS485 Adapter       │
│  (Waveshare)             │     │  (ATNSINC)               │
│  3.3V CAN Transceiver    │     │  MAX485 Compatible       │
└──────────────┬───────────┘     └──────────────┬───────────┘
               │                                 │
               │    ┌────────────────────────────┤
               │    │                            │
               ▼    ▼                            ▼
    ┌─────────────────────────────────────────────────────────┐
    │                    ESP32 DevKit V1                       │
    │  ┌─────────────────────────────────────────────────────┐ │
    │  │                  Dual-Core MCU                       │ │
    │  │  ┌───────────┐ ┌───────────┐ ┌───────────────────┐  │ │
    │  │  │  TWAI/CAN │ │   UART    │ │   ADC Channels    │  │ │
    │  │  │Controller │ │  (J1708)  │ │  (Fuel, Dimmer)   │  │ │
    │  │  └───────────┘ └───────────┘ └───────────────────┘  │ │
    │  │  ┌───────────┐ ┌───────────┐ ┌───────────────────┐  │ │
    │  │  │    SPI    │ │   1-Wire  │ │   Wi-Fi/BT        │  │ │
    │  │  │ (Display) │ │  (Temps)  │ │   (Future)        │  │ │
    │  │  └───────────┘ └───────────┘ └───────────────────┘  │ │
    │  └─────────────────────────────────────────────────────┘ │
    └──────────────────────────────────────────────────────────┘
               │
               ▼
    ┌─────────────────────────────────────────────────────────┐
    │                    OUTPUT DEVICES                        │
    │  ┌───────────┐ ┌───────────┐ ┌───────────────────────┐  │
    │  │  Display  │ │   USB     │ │   SD Card Logger      │  │
    │  │  (TFT/LCD)│ │  Serial   │ │   (Optional)          │  │
    │  └───────────┘ └───────────┘ └───────────────────────┘  │
    └─────────────────────────────────────────────────────────┘
```

### 3.2 Data Flow

```
                    ┌─────────────────┐
                    │   Raw CAN Frame │
                    │   (29-bit ID)   │
                    └────────┬────────┘
                             │
                    ┌────────▼────────┐
                    │  Extract PGN    │
                    │  (18-bit)       │
                    └────────┬────────┘
                             │
                    ┌────────▼────────┐
                    │  Lookup SPNs    │
                    │  (Parameters)   │
                    └────────┬────────┘
                             │
                    ┌────────▼────────┐
                    │  Apply Scale &  │
                    │  Offset         │
                    └────────┬────────┘
                             │
                    ┌────────▼────────┐
                    │  Store in Data  │
                    │  Manager        │
                    └────────┬────────┘
                             │
          ┌──────────────────┼──────────────────┐
          │                  │                  │
    ┌─────▼─────┐     ┌─────▼─────┐     ┌─────▼─────┐
    │  Display  │     │   Serial  │     │  SD Card  │
    │  Update   │     │   Output  │     │    Log    │
    └───────────┘     └───────────┘     └───────────┘
```

---

## 4. Hardware Specification

### 4.1 Bill of Materials (Available Hardware)

| Item | Model | Quantity | Purpose | Status |
|------|-------|----------|---------|--------|
| Microcontroller | ESP32 DevKit V1 (DOIT) | 1 | Main processor | **Ordered** |
| CAN Transceiver | Waveshare SN65HVD230 CAN Board | 1 | J1939 CAN bus interface | **Ordered** |
| RS485 Transceiver | ATNSINC TTL to RS485 Adapter (MAX485) | 2 | J1708 serial interface | **Ordered** |

### 4.2 ESP32 DevKit V1 Specifications

| Parameter | Specification |
|-----------|---------------|
| MCU | ESP32-WROOM-32 (Dual-core Tensilica LX6) |
| Clock Speed | 240 MHz |
| Flash | 4 MB |
| SRAM | 520 KB |
| GPIO Pins | 34 (usable: ~25) |
| ADC Channels | 18 (12-bit resolution) |
| UART | 3 interfaces |
| SPI | 4 interfaces |
| I2C | 2 interfaces |
| Built-in CAN | 1 controller (TWAI) |
| Wi-Fi | 802.11 b/g/n |
| Bluetooth | v4.2 BR/EDR and BLE |
| Operating Voltage | 3.3V (5V input via USB/VIN) |

### 4.3 Waveshare SN65HVD230 CAN Board Specifications

| Parameter | Specification |
|-----------|---------------|
| Chip | Texas Instruments SN65HVD230 |
| Operating Voltage | 3.3V (direct ESP32 compatibility) |
| Max Baud Rate | 1 Mbps |
| Features | ESD protection, pinout compatible with PCA82C250 |
| Interface | 3-pin TX, RX, GND + 2-pin CANH/CANL |

**Pinout:**

| Board Pin | Function | ESP32 Connection |
|-----------|----------|------------------|
| 3V3 | Power input | 3.3V |
| GND | Ground | GND |
| CTX (TX) | CAN Transmit | GPIO 5 (CAN_TX) |
| CRX (RX) | CAN Receive | GPIO 4 (CAN_RX) |
| CANH | CAN High (to truck) | J1939 Pin C |
| CANL | CAN Low (to truck) | J1939 Pin D |

### 4.4 ATNSINC TTL-RS485 Adapter Specifications

| Parameter | Specification |
|-----------|---------------|
| Chip | MAX485 or equivalent |
| Operating Voltage | 3.3V / 5V compatible |
| Baud Rate | Up to 2.5 Mbps |
| Mode | Half-duplex |
| Interface | DI, RO, DE, RE, VCC, GND + A/B differential |

**Pinout:**

| Board Pin | Function | ESP32 Connection |
|-----------|----------|------------------|
| VCC | Power (3.3V or 5V) | 3.3V recommended |
| GND | Ground | GND |
| DI | Driver Input (TX) | GPIO 17 (UART2_TX) |
| RO | Receiver Output (RX) | GPIO 16 (UART2_RX) |
| DE | Driver Enable | GPIO 25 (control) |
| RE | Receiver Enable (inverted) | GPIO 25 (tied to DE) |
| A | Non-inverting (J1708+) | J1939 Connector Pin F |
| B | Inverting (J1708-) | J1939 Connector Pin G |

### 4.5 Future Hardware (Phase 4+)

| Item | Model | Purpose | Priority |
|------|-------|---------|----------|
| Display | 2.8" or 3.5" SPI TFT (ILI9341) | Visual dashboard | High |
| Thermocouple | MAX31855K + K-type EGT probe | Exhaust gas temperature | Medium |
| Temperature | DS18B20 (x2) | Ambient/cabin temperature | Medium |
| SD Card Module | SPI SD Card reader | Data logging | Medium |
| Voltage Regulator | DC-DC Buck (12V→5V/3.3V) | Vehicle power | High |

---

## 5. Protocol Specifications

### 5.1 SAE J1939 Protocol

#### 5.1.1 Overview

| Parameter | Value |
|-----------|-------|
| Physical Layer | CAN 2.0B |
| Baud Rate | 250 kbps (standard) |
| Identifier | 29-bit extended |
| Data Length | 0-8 bytes (up to 1785 with TP) |
| Byte Order | Little-endian (Intel) |

#### 5.1.2 29-bit CAN ID Structure

```
┌───────────┬───────────┬───────────────────────────────────────┬───────────┐
│ Priority  │ Reserved  │              PGN (18 bits)            │  Source   │
│  (3 bits) │  DP (2)   │  PDU Format (8)  │  PDU Specific (8)  │ Addr (8)  │
└───────────┴───────────┴──────────────────┴────────────────────┴───────────┘
    Bits:    28-26       25-24              23-16                15-8         7-0
```

#### 5.1.3 Key PGNs for This Project

| PGN | Name | Update Rate | Key SPNs |
|-----|------|-------------|----------|
| 61444 (0xF004) | EEC1 - Electronic Engine Controller 1 | 10-20 ms | Engine Speed (190), Torque (513) |
| 65262 (0xFEEE) | ET1 - Engine Temperature 1 | 1000 ms | Coolant Temp (110) |
| 65263 (0xFEEF) | EFL/P1 - Engine Fluid Level/Pressure 1 | 500 ms | Oil Pressure (100) |
| 65265 (0xFEF1) | CCVS - Cruise Control/Vehicle Speed | 100 ms | Wheel Speed (84) |
| 65266 (0xFEF2) | LFE - Fuel Economy | 100 ms | Fuel Rate (183) |
| 65269 (0xFEF5) | AMB - Ambient Conditions | 1000 ms | Ambient Temp (171) |
| 65270 (0xFEF6) | IC1 - Intake/Exhaust Conditions 1 | 500 ms | Boost Pressure (102) |
| 65272 (0xFEF8) | TCO1 - Transmission Config. 1 | 100 ms | Current Gear (523) |
| 65226 (0xFECA) | DM1 - Active Diagnostic Trouble Codes | 1000 ms | DTCs |
| 65227 (0xFECB) | DM2 - Previously Active DTCs | On-request | DTCs |

#### 5.1.4 J1939 Transport Protocol

For messages > 8 bytes (e.g., DM1 with multiple DTCs):

| Message Type | PGN | Description |
|--------------|-----|-------------|
| BAM | 60416 (0xEC00) | Broadcast Announce Message |
| TP.DT | 60160 (0xEB00) | Transport Protocol Data Transfer |
| RTS | 60416 | Request to Send (peer-to-peer) |
| CTS | 60416 | Clear to Send (peer-to-peer) |

### 5.2 SAE J1708/J1587 Protocol

#### 5.2.1 Overview

| Parameter | Value |
|-----------|-------|
| Physical Layer | RS-485 (differential) |
| Baud Rate | 9600 bps |
| Data Format | 8N1 (8 data, no parity, 1 stop) |
| Max Message Length | 21 bytes |
| Checksum | Modulo-256 sum = 0 |

#### 5.2.2 Message Structure

```
┌─────┬─────────────────────────────────┬──────────┐
│ MID │         Data (PIDs/Values)      │ Checksum │
│(1B) │         (1-19 bytes)            │   (1B)   │
└─────┴─────────────────────────────────┴──────────┘
```

- **MID (Message ID):** Identifies the source ECU
- **PIDs:** Parameter IDs with associated data
- **Checksum:** Sum of all bytes (including checksum) = 0x00 (mod 256)

#### 5.2.3 Common MIDs

| MID | Source ECU |
|-----|------------|
| 128 | Engine #1 |
| 130 | Transmission |
| 136 | Brakes - Trailer #1 |
| 137 | Brakes - Trailer #2 |
| 140 | Instrument Cluster |
| 142 | Vehicle Management |
| 172 | Brakes - Tractor |

#### 5.2.4 ABS-Related PIDs (J1587)

| PID | Description | Data Length |
|-----|-------------|-------------|
| 65 | Wheel Speed | Variable |
| 84 | Road Speed | 2 bytes |
| 194 | Fault Codes | Variable |

---

## 6. Software Architecture

### 6.1 Development Environment

| Tool | Purpose | Version |
|------|---------|---------|
| VS Code | IDE | Latest |
| PlatformIO | Build system | Latest |
| Arduino Framework | ESP32 development | arduino-esp32 2.x |
| Git | Version control | Latest |

### 6.2 Project Structure

```
J1939_J1708_Dashboard/
├── README.md
├── Docs/
│   ├── PROJECT_SPECIFICATION.md (this file)
│   ├── HARDWARE_REFERENCE.md
│   ├── J1939_PGN_CATALOG.md
│   └── Custom Truck Data Logging & Display Project.pdf
├── firmware/
│   ├── platformio.ini
│   ├── src/
│   │   ├── main.cpp
│   │   ├── config.h
│   │   ├── can/
│   │   │   ├── can_driver.cpp
│   │   │   ├── can_driver.h
│   │   │   ├── j1939_parser.cpp
│   │   │   └── j1939_parser.h
│   │   ├── j1708/
│   │   │   ├── j1708_driver.cpp
│   │   │   ├── j1708_driver.h
│   │   │   ├── j1587_parser.cpp
│   │   │   └── j1587_parser.h
│   │   ├── sensors/
│   │   │   ├── analog_sensors.cpp
│   │   │   └── analog_sensors.h
│   │   ├── data/
│   │   │   ├── data_manager.cpp
│   │   │   ├── data_manager.h
│   │   │   ├── parameter_catalog.h
│   │   │   └── watch_list_manager.cpp/.h
│   │   ├── storage/
│   │   │   ├── nvs_manager.cpp
│   │   │   ├── nvs_manager.h
│   │   │   ├── trip_data.cpp
│   │   │   ├── trip_data.h
│   │   │   ├── fault_history.cpp
│   │   │   └── fault_history.h
│   │   ├── display/
│   │   │   ├── display_driver.cpp
│   │   │   └── display_driver.h
│   │   └── output/
│   │       ├── serial_output.cpp
│   │       └── serial_output.h
│   ├── lib/
│   │   └── (external libraries)
│   └── test/
│       ├── test_j1939_parser.cpp
│       ├── test_j1587_parser.cpp
│       ├── test_watch_list.cpp
│       └── test_nvs_storage.cpp
├── tools/
│   ├── test_data_generator/
│   │   ├── j1939_generator.py          # Main generator class
│   │   ├── scenarios.py                 # Driving scenario definitions
│   │   ├── frame_builders.py            # PGN frame constructors
│   │   └── requirements.txt             # Python dependencies
│   ├── log_converter/
│   │   ├── convert_logs.py              # Convert between log formats
│   │   └── formats/                     # Format parsers
│   ├── log_analyzer/
│   │   └── analyze_log.py               # Analyze recorded logs
│   └── validation/
│       ├── validate_parser.py           # Compare parsed vs expected
│       └── expected_values.json         # Known-good decoded values
├── test_data/
│   ├── real_logs/                       # Downloaded real truck logs
│   │   ├── cummins_isx/                 # Cummins ISX engine logs
│   │   │   └── README.md                # Data source documentation
│   │   └── generic_j1939/               # Generic heavy-duty logs
│   ├── synthetic/                       # Generated test scenarios
│   │   ├── idle_60min.log               # Engine idle scenario
│   │   ├── highway_30min.log            # Highway cruise scenario
│   │   ├── cold_start.log               # Cold start warmup
│   │   ├── acceleration.log             # Speed/RPM ramp
│   │   └── fault_injection.log          # DM1 fault codes
│   ├── expected_results/                # Validation data
│   │   ├── idle_expected.json           # Expected values for idle
│   │   └── highway_expected.json        # Expected values for highway
│   └── dbc_files/                       # DBC file resources
│       ├── j1939_basic.dbc              # Basic J1939 definitions
│       └── README.md                    # DBC source documentation
└── scripts/
    ├── generate_test_data.sh            # Generate all test scenarios
    ├── run_validation.sh                # Run parser validation
    └── setup_dev_env.sh                 # Set up development environment
```

### 6.3 Software Modules

#### 6.3.1 CAN Driver Module

**Responsibilities:**
- Initialize ESP32 TWAI controller at 250 kbps
- Configure acceptance filters for extended frames (29-bit)
- Receive CAN frames via interrupt/callback
- Provide thread-safe message queue

**Key Functions:**
```cpp
void can_init(gpio_num_t tx_pin, gpio_num_t rx_pin);
bool can_receive(twai_message_t* message, uint32_t timeout_ms);
bool can_transmit(const twai_message_t* message);
uint32_t can_get_error_count();
```

#### 6.3.2 J1939 Parser Module

**Responsibilities:**
- Extract PGN from 29-bit CAN ID
- Handle Transport Protocol (BAM) reassembly
- Decode SPNs with scaling and offset
- Manage DTC parsing from DM1/DM2

**Key Data Structures:**
```cpp
typedef struct {
    uint32_t pgn;
    uint8_t source_address;
    uint8_t priority;
    uint8_t data[8];
    uint8_t data_len;
} j1939_message_t;

typedef struct {
    uint32_t spn;
    uint32_t fmi;     // Failure Mode Identifier
    uint8_t oc;       // Occurrence Count
    uint8_t source;   // Source Address
} j1939_dtc_t;

typedef struct {
    uint16_t spn;
    float value;
    const char* name;
    const char* unit;
} j1939_parameter_t;
```

#### 6.3.3 J1708/J1587 Driver Module

**Responsibilities:**
- Configure UART at 9600 bps (8N1)
- Control RS485 DE/RE for half-duplex operation
- Assemble complete J1708 messages
- Validate checksums

**Key Functions:**
```cpp
void j1708_init(uint8_t rx_pin, uint8_t tx_pin, uint8_t de_pin);
bool j1708_receive(j1708_message_t* message, uint32_t timeout_ms);
bool j1708_validate_checksum(const uint8_t* data, uint8_t len);
```

#### 6.3.4 Data Manager Module

**Responsibilities:**
- Central storage for all vehicle parameters
- Thread-safe access to data
- Timestamping and data freshness tracking
- Alert/threshold management

**Key Functions:**
```cpp
void data_manager_init();
void data_manager_update_parameter(uint16_t id, float value);
float data_manager_get_parameter(uint16_t id);
bool data_manager_is_fresh(uint16_t id, uint32_t max_age_ms);
```

#### 6.3.5 Persistent Storage Module (NVS)

**Overview:**
The ESP32 includes Non-Volatile Storage (NVS) in flash memory that retains data when power is removed. This module manages persistent storage for trip data, fuel economy statistics, and fault code history.

**ESP32 NVS Specifications:**
| Parameter | Value |
|-----------|-------|
| Storage Location | Internal Flash (partition "nvs") |
| Default Size | 20 KB (can be increased) |
| Wear Leveling | Automatic |
| Data Types | integers, strings, blobs |
| Write Endurance | ~100,000 cycles per sector |
| Access | Key-value pairs in namespaces |

**Responsibilities:**
- Store trip odometer and fuel consumption data
- Maintain lifetime and per-trip statistics
- Save active/historical fault codes
- Persist user settings (display preferences, units)
- Handle graceful shutdown detection
- Manage storage wear by batching writes

**Data to Persist:**

| Category | Data Items | Update Frequency | Namespace |
|----------|------------|------------------|----------|
| **Trip A** | Distance, Fuel Used, Start Time | On trip reset or every 5 min | `trip_a` |
| **Trip B** | Distance, Fuel Used, Start Time | On trip reset or every 5 min | `trip_b` |
| **Lifetime** | Total Distance, Total Fuel, Engine Hours | Every 10 min | `lifetime` |
| **Fuel Economy** | Current MPG, Average MPG, Best MPG | Every 5 min | `fuel_econ` |
| **Fault Codes** | Active DTCs, Historical DTCs (last 20) | On DTC change | `fault_log` |
| **Settings** | Units (metric/imperial), brightness, pages | On change | `settings` |
| **System** | Last shutdown type, boot count | On boot/shutdown | `system` |

**Key Data Structures:**
```cpp
// Trip data structure (stored in NVS)
typedef struct {
    float distance_km;           // Trip distance
    float fuel_used_liters;      // Fuel consumed this trip
    uint32_t start_time;         // Unix timestamp of trip start
    uint32_t duration_seconds;   // Total driving time
    float avg_speed_kmh;         // Average speed
    float avg_fuel_economy;      // Average L/100km or MPG
} trip_data_t;

// Lifetime statistics
typedef struct {
    float total_distance_km;     // Odometer
    float total_fuel_liters;     // Lifetime fuel
    float engine_hours;          // Total engine hours
    uint32_t boot_count;         // Number of power cycles
    float best_mpg;              // Best recorded fuel economy
    float worst_mpg;             // Worst recorded fuel economy
} lifetime_stats_t;

// Stored fault code
typedef struct {
    uint32_t spn;                // Suspect Parameter Number
    uint8_t fmi;                 // Failure Mode Identifier
    uint8_t source_address;      // ECU source
    uint32_t first_seen;         // Timestamp first detected
    uint32_t last_seen;          // Timestamp last seen
    uint16_t occurrence_count;   // How many times seen
    bool is_active;              // Currently active?
} stored_dtc_t;
```

**Key Functions:**
```cpp
// Initialization
void storage_init();
bool storage_is_initialized();

// Trip Management
void trip_reset(uint8_t trip_id);  // 0=Trip A, 1=Trip B
void trip_update(uint8_t trip_id, float distance_delta, float fuel_delta);
trip_data_t trip_get(uint8_t trip_id);

// Lifetime Stats
lifetime_stats_t lifetime_get();
void lifetime_update(float distance_delta, float fuel_delta);

// Fuel Economy
float fuel_economy_get_instant();   // Current (calculated)
float fuel_economy_get_average();   // Session average
float fuel_economy_get_lifetime();  // Lifetime average

// Fault Code Storage
void dtc_store(uint32_t spn, uint8_t fmi, uint8_t source);
void dtc_clear_active();
void dtc_clear_all();
stored_dtc_t* dtc_get_history(uint8_t* count);

// Settings
void settings_save(const char* key, int32_t value);
int32_t settings_load(const char* key, int32_t default_val);

// Low-level NVS access
bool nvs_write_blob(const char* ns, const char* key, void* data, size_t len);
bool nvs_read_blob(const char* ns, const char* key, void* data, size_t len);
```

**Write Strategy (Wear Leveling):**

To minimize flash wear, data is not written on every update:

1. **Buffered Writes:** Accumulate changes in RAM
2. **Periodic Flush:** Write to NVS every 5 minutes during operation
3. **Threshold Flush:** Write immediately if significant change (e.g., >1km traveled)
4. **Shutdown Flush:** Attempt graceful save on power-down detection
5. **Boot Recovery:** Detect unclean shutdown and estimate missed data

**Power-Down Detection:**
```cpp
// Monitor battery voltage via ADC
// If voltage drops below threshold, trigger emergency save
#define POWER_LOSS_THRESHOLD_MV  11500  // 11.5V
#define POWER_LOSS_DEBOUNCE_MS   100

void power_monitor_task(void* param) {
    while(1) {
        uint32_t voltage_mv = read_battery_voltage();
        if (voltage_mv < POWER_LOSS_THRESHOLD_MV) {
            vTaskDelay(pdMS_TO_TICKS(POWER_LOSS_DEBOUNCE_MS));
            voltage_mv = read_battery_voltage();
            if (voltage_mv < POWER_LOSS_THRESHOLD_MV) {
                storage_emergency_save();  // Quick save critical data
            }
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
```

**NVS Namespace Organization:**
```
nvs_partition "nvs"
├── trip_a/
│   ├── distance      (float)
│   ├── fuel          (float)  
│   ├── start_time    (u32)
│   └── duration      (u32)
├── trip_b/
│   └── (same as trip_a)
├── lifetime/
│   ├── total_dist    (float)
│   ├── total_fuel    (float)
│   ├── eng_hours     (float)
│   ├── boot_count    (u32)
│   └── best_mpg      (float)
├── fuel_econ/
│   ├── avg_mpg       (float)
│   └── samples       (u32)
├── fault_log/
│   ├── dtc_count     (u8)
│   └── dtc_data      (blob - array of stored_dtc_t)
├── settings/
│   ├── units         (u8) - 0=metric, 1=imperial
│   ├── brightness    (u8)
│   └── page          (u8)
└── system/
    ├── clean_shutdown (u8) - 1=clean, 0=dirty
    ├── last_time      (u32)
    └── boot_count     (u32)
```

### 6.4 Task Architecture (FreeRTOS)

```
┌─────────────────────────────────────────────────────────────────────────┐
│                          FreeRTOS Scheduler                             │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐    │
│  │   CAN       │  │   J1708     │  │   Sensor    │  │   Display   │    │
│  │   Task      │  │   Task      │  │   Task      │  │   Task      │    │
│  │             │  │             │  │             │  │             │    │
│  │ Priority: 5 │  │ Priority: 4 │  │ Priority: 2 │  │ Priority: 3 │    │
│  │ Core: 0     │  │ Core: 0     │  │ Core: 1     │  │ Core: 1     │    │
│  └──────┬──────┘  └──────┬──────┘  └──────┬──────┘  └──────┬──────┘    │
│         │                │                │                │           │
│         └────────────────┼────────────────┼────────────────┘           │
│                          │                │                            │
│                          ▼                ▼                            │
│                    ┌─────────────────────────────────────┐              │
│                    │          Data Manager               │              │
│                    │    (Thread-Safe Shared State)       │              │
│                    └─────────────────────────────────────┘              │
└─────────────────────────────────────────────────────────────────────────┘
```

### 6.5 Libraries & Dependencies

| Library | Purpose | License |
|---------|---------|---------|
| ESP32 Arduino Core | Base framework | LGPL |
| ESP32 TWAI Driver | Native CAN support | Apache 2.0 |
| ESP32 NVS Library | Non-volatile storage | Apache 2.0 |
| Preferences | Arduino-friendly NVS wrapper | LGPL |
| ArduinoJson | JSON serialization (logging/WiFi) | MIT |
| TFT_eSPI | Display driver (Phase 4) | MIT |
| LVGL | GUI framework (Phase 5) | MIT |
| DallasTemperature | DS18B20 support (future) | MIT |
| OneWire | 1-Wire protocol (future) | MIT |

---

## 7. Pin Mapping & Wiring Diagrams

### 7.1 ESP32 DevKit V1 Pin Assignment

| GPIO | Function | Module | Notes |
|------|----------|--------|-------|
| **CAN Bus (J1939)** ||||
| GPIO 4 | CAN_RX | SN65HVD230 | Default TWAI RX |
| GPIO 5 | CAN_TX | SN65HVD230 | Default TWAI TX |
| **J1708 Serial** ||||
| GPIO 16 | UART2_RX | RS485 RO | J1708 receive |
| GPIO 17 | UART2_TX | RS485 DI | J1708 transmit |
| GPIO 25 | RS485_DE/RE | RS485 DE+RE | Direction control |
| **ADC Inputs (ADC1 only - WiFi safe)** ||||
| GPIO 36 | ADC1_CH0 | Fuel Tank 1 | Input only |
| GPIO 39 | ADC1_CH3 | Fuel Tank 2 | Input only |
| GPIO 34 | ADC1_CH6 | Spare Analog | Input only |
| GPIO 35 | ADC1_CH7 | Dimmer Input | Input only, with divider |
| GPIO 32 | ADC1_CH4 | Battery Voltage | With voltage divider |
| GPIO 33 | ADC1_CH5 | Spare | General purpose |
| **SPI Display (Phase 4)** ||||
| GPIO 18 | SPI_SCK | TFT Display | VSPI Clock |
| GPIO 23 | SPI_MOSI | TFT Display | VSPI Data |
| GPIO 19 | SPI_MISO | TFT Display | VSPI Data In |
| GPIO 15 | TFT_CS | TFT Display | Chip Select |
| GPIO 2 | TFT_DC | TFT Display | Data/Command |
| GPIO 21 | TFT_RST | TFT Display | Reset |
| **1-Wire (Phase 4)** ||||
| GPIO 27 | OneWire_Data | DS18B20 | With 4.7kΩ pull-up |
| **SPI Thermocouple (Phase 4)** ||||
| GPIO 14 | TC_CS | MAX31855 | Chip Select |
| **Spare/Future** ||||
| GPIO 26 | Relay_Control | Relay Module | Remote start (future) |
| GPIO 13 | Status_LED | LED | System status |

### 7.2 CAN Transceiver Wiring (SN65HVD230)

```
ESP32 DevKit V1                    Waveshare SN65HVD230
┌──────────────┐                   ┌──────────────────┐
│              │                   │                  │
│         3.3V ├───────────────────┤ 3V3              │
│              │                   │                  │
│          GND ├───────────────────┤ GND              │
│              │                   │                  │
│       GPIO 5 ├───────────────────┤ CTX (TX)         │       To Truck
│              │                   │                  │       J1939 Bus
│       GPIO 4 ├───────────────────┤ CRX (RX)         │    ┌───────────┐
│              │                   │                  │    │           │
│              │                   │           CANH   ├────┤ Pin C     │
│              │                   │                  │    │ (CAN High)│
│              │                   │           CANL   ├────┤ Pin D     │
│              │                   │                  │    │ (CAN Low) │
└──────────────┘                   └──────────────────┘    └───────────┘
```

### 7.3 RS485 Transceiver Wiring (J1708)

```
ESP32 DevKit V1                    TTL-RS485 Adapter
┌──────────────┐                   ┌──────────────────┐
│              │                   │                  │
│         3.3V ├───────────────────┤ VCC              │
│              │                   │                  │
│          GND ├───────────────────┤ GND              │
│              │                   │                  │
│      GPIO 17 ├───────────────────┤ DI (TX)          │       To Truck
│              │                   │                  │       J1708 Bus
│      GPIO 16 ├───────────────────┤ RO (RX)          │    ┌───────────┐
│              │                   │                  │    │           │
│      GPIO 25 ├───────────────────┤ DE               │    │           │
│              │        ┌──────────┤ RE (inverted)    │    │           │
│              │        │          │                  │    │           │
│              │        └───GND    │            A     ├────┤ Pin F     │
│              │                   │                  │    │ (J1708+)  │
│              │                   │            B     ├────┤ Pin G     │
│              │                   │                  │    │ (J1708-)  │
└──────────────┘                   └──────────────────┘    └───────────┘

Note: For read-only operation, tie DE to GND and RE to GND.
      For read/write, control DE with GPIO 25 (HIGH=transmit, LOW=receive)
```

### 7.4 9-Pin Deutsch Connector Pinout (J1939/J1708)

```
         ┌─────────────────────┐
        /                       \
       /   A     B     C     D   \
      │    ●     ●     ●     ●    │
      │         E     F     G     │
      │          ●     ●     ●    │
       \       H     J           /
        \       ●     ●         /
         └─────────────────────┘

Pin  | Function          | Color (typical)
-----|-------------------|----------------
A    | Ground (return)   | Black
B    | Battery + (12/24V)| Red
C    | J1939 CAN High    | Yellow
D    | J1939 CAN Low     | Green
E    | CAN Shield        | (optional)
F    | J1708+ (J1587)    | Orange
G    | J1708- (J1587)    | Blue
H    | CAN 2 High        | (if present)
J    | CAN 2 Low         | (if present)
```

---

## 7.5 Test Data Sources & Simulation Framework

This section defines resources for developing and testing the J1939/J1708 parsing libraries **without physical hardware**.

### 7.5.1 GitHub J1939 Test Data Resources

| Repository | Description | Data Type |
|------------|-------------|-----------|
| [DieselDuz42/Arduino-CAN-bus-SD-logger](https://github.com/DieselDuz42/Arduino-CAN-bus-SD-logger) | Example J1939 CAN logs from trucks | Raw CAN log files |
| [commaai/opendbc](https://github.com/commaai/opendbc) | DBC files for 200+ vehicles, CAN parsing library | DBC definitions |
| [commaai/commaCarSegments](https://huggingface.co/datasets/commaai/commaCarSegments) | 3,148 hours of CAN data from 230 vehicle platforms | Real driving data |
| [DanielMartensson/Open-SAE-J1939](https://github.com/DanielMartensson/Open-SAE-J1939) | Open source J1939 implementation with test examples | Reference implementation |
| [juergenH87/python-can-j1939](https://github.com/juergenH87/python-can-j1939) | Python J1939 library with test cases | Python test framework |
| [famez/J1939-Framework](https://github.com/famez/J1939-Framework) | C++ J1939 framework with test data | C++ implementation |
| [jackm/j1939decode](https://github.com/jackm/j1939decode) | J1939 decode C library | Decoder reference |

### 7.5.2 J1939 DBC File Resources

| Source | Description | License |
|--------|-------------|---------|
| CSS Electronics J1939 DBC | Official SAE J1939 Digital Annex conversion (1,800+ PGNs, 10,000+ SPNs) | Commercial ($150-500) |
| [opendbc/dbc](https://github.com/commaai/opendbc/tree/master/opendbc/dbc) | Open source vehicle DBC files | MIT |
| CSS Electronics Sample DBC | Free demo J1939 DBC with ~50 common PGNs | Free (for development/testing) |

**Licensing Note:** The full J1939 Digital Annex DBC requires a commercial license. For development and testing, use the free CSS Electronics demo DBC or create a custom DBC with only the PGNs you need based on the public J1939 documentation.

### 7.5.3 Test Data File Formats

#### SocketCAN Log Format (.log)
```
(1609459200.000000) can0 18FEF100#FFFF8068130000FF
(1609459200.100000) can0 0CF00400#0000FFFF0000FFFF
```

#### CSV Format
```csv
timestamp,can_id,data
1609459200.000,0x18FEF100,FF FF 80 68 13 00 00 FF
1609459200.100,0x0CF00400,00 00 FF FF 00 00 FF FF
```

#### Candump Format
```
can0  18FEF100   [8]  FF FF 80 68 13 00 00 FF
can0  0CF00400   [8]  00 00 FF FF 00 00 FF FF
```

### 7.5.4 Sample Cummins ISX Engine Data

Based on common Cummins ISX broadcast patterns at 250kbps:

```cpp
// Cummins ISX typical J1939 frames - use for testing
const test_frame_t cummins_test_frames[] = {
    // PGN 61444 (EEC1) - Engine Speed, Torque
    { 0x0CF00400, {0x00, 0x00, 0xFF, 0x68, 0x13, 0xFF, 0xFF, 0xFF}, 8, 100 },  // 621 RPM
    
    // PGN 65262 (ET1) - Engine Coolant Temperature  
    { 0x18FEEE00, {0x8C, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, 8, 1000 }, // 100°C
    
    // PGN 65263 (EFL/P1) - Engine Oil Pressure
    { 0x18FEEF00, {0x64, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, 8, 1000 }, // 400 kPa
    
    // PGN 65270 (IC1) - Intake Manifold Pressure/Temp
    { 0x18FEF600, {0xFF, 0x96, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, 8, 500 },  // 150 kPa
    
    // PGN 61443 (EEC2) - Accelerator Pedal Position
    { 0x0CF00300, {0x00, 0xFF, 0x64, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, 8, 50 },   // 40% pedal
    
    // PGN 65265 (CCVS) - Vehicle Speed
    { 0x18FEF100, {0xFF, 0xFF, 0x80, 0x68, 0x13, 0x00, 0x00, 0xFF}, 8, 100 },  // 104.9 km/h
    
    // PGN 65276 (DASH) - Fuel Level
    { 0x18FEFC00, {0xFF, 0x64, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, 8, 1000 }, // 40% fuel
    
    // PGN 65253 (HOURS) - Engine Hours
    { 0x18FEE500, {0xC0, 0xA8, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF}, 8, 10000 }, // 10800 hours
    
    // PGN 65266 (LFE) - Fuel Economy (Fuel Rate)
    { 0x18FEF200, {0x64, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, 8, 100 },   // 50 L/h
};
```

### 7.5.5 Mock Data Generator Architecture

```
┌──────────────────────────────────────────────────────────────────┐
│                    Test Data Generator                           │
├──────────────────────────────────────────────────────────────────┤
│  ┌─────────────────┐    ┌─────────────────┐    ┌──────────────┐  │
│  │  Scenario       │───▶│  Frame          │───▶│  Output      │  │
│  │  Definitions    │    │  Generator      │    │  Formatters  │  │
│  │                 │    │                 │    │              │  │
│  │  - Idle         │    │  - Timing       │    │  - SocketCAN │  │
│  │  - Highway      │    │  - Noise        │    │  - CSV       │  │
│  │  - Fault Sim    │    │  - Variation    │    │  - Binary    │  │
│  │  - Cold Start   │    │  - Correlations │    │  - Serial    │  │
│  └─────────────────┘    └─────────────────┘    └──────────────┘  │
└──────────────────────────────────────────────────────────────────┘
         │                        │                      │
         ▼                        ▼                      ▼
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│  scenarios/     │    │  Unit Tests     │    │  Integration    │
│  *.json         │    │  (pytest)       │    │  Tests (ESP32)  │
└─────────────────┘    └─────────────────┘    └─────────────────┘
```

### 7.5.6 Python Test Data Generator

```python
#!/usr/bin/env python3
"""
J1939 Test Data Generator for Truck Dashboard Project
Generates realistic J1939 CAN frames for testing without hardware
"""

import struct
import random
import json
from dataclasses import dataclass
from typing import List, Optional

@dataclass
class J1939Frame:
    pgn: int
    source_address: int
    priority: int
    data: bytes
    timestamp: float

class J1939TestDataGenerator:
    """Generate realistic J1939 test data based on driving scenarios."""
    
    def __init__(self):
        self.scenarios = {}
        self.current_state = {
            'engine_rpm': 0,
            'vehicle_speed_kmh': 0,
            'coolant_temp_c': 20,
            'fuel_level_pct': 100,
            'oil_pressure_kpa': 0,
            'boost_pressure_kpa': 101,
            'throttle_pct': 0,
            'engine_hours': 10800.0,
            'fuel_rate_lph': 0,
        }
    
    def create_pgn_61444_eec1(self) -> bytes:
        """Create Electronic Engine Controller 1 (Engine Speed, Torque)"""
        rpm_raw = int(self.current_state['engine_rpm'] / 0.125)
        return struct.pack('<BBBBBBBB',
            0x00,                           # Torque mode
            0xFF,                           # Driver demand torque
            0xFF,                           # Actual torque
            rpm_raw & 0xFF,                 # Engine speed low byte
            (rpm_raw >> 8) & 0xFF,          # Engine speed high byte
            0xFF,                           # Source address
            0xFF,                           # Reserved
            0xFF                            # Reserved
        )
    
    def create_pgn_65262_et1(self) -> bytes:
        """Create Engine Temperature 1 (Coolant Temp)"""
        temp_raw = int(self.current_state['coolant_temp_c'] + 40)  # Offset of -40
        return struct.pack('<BBBBBBBB',
            temp_raw,    # Coolant temp
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
        )
    
    def create_pgn_65265_ccvs(self) -> bytes:
        """Create Cruise Control/Vehicle Speed"""
        speed_raw = int(self.current_state['vehicle_speed_kmh'] / 0.00390625)
        return struct.pack('<BBBBBBBB',
            0xFF, 0xFF,
            speed_raw & 0xFF,
            (speed_raw >> 8) & 0xFF,
            0x00, 0x00, 0x00, 0xFF
        )
    
    def can_id_from_pgn(self, pgn: int, sa: int = 0x00, priority: int = 6) -> int:
        """Convert PGN to 29-bit CAN ID"""
        return (priority << 26) | (pgn << 8) | sa
    
    def generate_idle_scenario(self, duration_s: float, step_ms: int = 100) -> List[J1939Frame]:
        """Generate frames for engine idling scenario"""
        frames = []
        self.current_state['engine_rpm'] = 700
        self.current_state['vehicle_speed_kmh'] = 0
        self.current_state['throttle_pct'] = 0
        
        for ts in range(0, int(duration_s * 1000), step_ms):
            # Add some realistic variation
            self.current_state['engine_rpm'] = 700 + random.uniform(-20, 20)
            self.current_state['coolant_temp_c'] = min(90, self.current_state['coolant_temp_c'] + 0.01)
            
            frames.append(J1939Frame(
                pgn=61444, source_address=0x00, priority=3,
                data=self.create_pgn_61444_eec1(),
                timestamp=ts / 1000.0
            ))
            
            if ts % 1000 == 0:  # 1 Hz for temperature
                frames.append(J1939Frame(
                    pgn=65262, source_address=0x00, priority=6,
                    data=self.create_pgn_65262_et1(),
                    timestamp=ts / 1000.0
                ))
        
        return frames
    
    def generate_highway_scenario(self, duration_s: float, step_ms: int = 100) -> List[J1939Frame]:
        """Generate frames for highway driving scenario with realistic variations"""
        frames = []
        self.current_state['engine_rpm'] = 1400
        self.current_state['vehicle_speed_kmh'] = 105
        self.current_state['throttle_pct'] = 35
        self.current_state['coolant_temp_c'] = 85
        self.current_state['fuel_rate_lph'] = 25  # ~7 MPG at highway
        
        for ts in range(0, int(duration_s * 1000), step_ms):
            # Realistic highway variations (hills, wind, traffic)
            speed_variation = random.uniform(-3, 3)  # ±3 km/h
            rpm_variation = random.uniform(-50, 50)  # ±50 RPM
            
            self.current_state['vehicle_speed_kmh'] = 105 + speed_variation
            self.current_state['engine_rpm'] = 1400 + rpm_variation
            
            # Correlate fuel rate with throttle/speed
            self.current_state['fuel_rate_lph'] = 25 + (speed_variation * 0.5)
            
            # EEC1 at 10 Hz
            frames.append(J1939Frame(
                pgn=61444, source_address=0x00, priority=3,
                data=self.create_pgn_61444_eec1(),
                timestamp=ts / 1000.0
            ))
            
            # CCVS (vehicle speed) at 10 Hz
            frames.append(J1939Frame(
                pgn=65265, source_address=0x00, priority=6,
                data=self.create_pgn_65265_ccvs(),
                timestamp=ts / 1000.0
            ))
            
            # Temperature at 1 Hz
            if ts % 1000 == 0:
                frames.append(J1939Frame(
                    pgn=65262, source_address=0x00, priority=6,
                    data=self.create_pgn_65262_et1(),
                    timestamp=ts / 1000.0
                ))
        
        return frames
    
    def generate_fault_scenario(self, base_frames: List[J1939Frame]) -> List[J1939Frame]:
        """Inject DM1 fault codes into an existing frame sequence"""
        # Add a DM1 message with active fault at t=30s
        dm1_data = bytes([
            0x04,  # Malfunction lamp ON
            0x00, 0x00,  # Reserved
            0x6E, 0x00, 0x01,  # SPN 110 (coolant temp), FMI 0 (high)
            0x01,  # Occurrence count
            0xFF   # Padding
        ])
        
        base_frames.append(J1939Frame(
            pgn=65226, source_address=0x00, priority=6,
            data=dm1_data,
            timestamp=30.0
        ))
        return base_frames
    
    def export_to_socketcan_log(self, frames: List[J1939Frame], filename: str):
        """Export frames to SocketCAN log format"""
        with open(filename, 'w') as f:
            for frame in frames:
                can_id = self.can_id_from_pgn(frame.pgn, frame.source_address, frame.priority)
                data_hex = ''.join(f'{b:02X}' for b in frame.data)
                f.write(f"({frame.timestamp:.6f}) can0 {can_id:08X}#{data_hex}\n")
    
    def export_to_csv(self, frames: List[J1939Frame], filename: str):
        """Export frames to CSV format"""
        with open(filename, 'w') as f:
            f.write("timestamp,can_id,pgn,sa,data_hex\n")
            for frame in frames:
                can_id = self.can_id_from_pgn(frame.pgn, frame.source_address, frame.priority)
                data_hex = ' '.join(f'{b:02X}' for b in frame.data)
                f.write(f"{frame.timestamp:.6f},0x{can_id:08X},{frame.pgn},{frame.source_address},{data_hex}\n")


# Example usage
if __name__ == "__main__":
    gen = J1939TestDataGenerator()
    
    # Generate idle scenario
    idle_frames = gen.generate_idle_scenario(60.0)  # 1 minute
    gen.export_to_socketcan_log(idle_frames, "test_data/idle_scenario.log")
    gen.export_to_csv(idle_frames, "test_data/idle_scenario.csv")
    
    print(f"Generated {len(idle_frames)} test frames")
```

---

## 7.6 Watch List Manager (Display Parameter Selection)

The Watch List Manager allows users to select which decoded parameters should be displayed on the dashboard. This is the bridge between raw data decoding and the display layer.

### 7.6.1 Watch List Architecture

```
┌─────────────────────────────────────────────────────────────────────┐
│                         Watch List Manager                          │
├─────────────────────────────────────────────────────────────────────┤
│                                                                     │
│  ┌─────────────┐    ┌──────────────┐    ┌────────────────────────┐  │
│  │ Parameter   │    │ Watch List   │    │ Display Adapter        │  │
│  │ Catalog     │───▶│ Selection    │───▶│ (Dashboard Interface)  │  │
│  │             │    │              │    │                        │  │
│  │ - All PGNs  │    │ - User picks │    │ - Gauge assignments    │  │
│  │ - All SPNs  │    │ - Priorities │    │ - Warning thresholds   │  │
│  │ - Metadata  │    │ - Max 16-32  │    │ - Display formatting   │  │
│  └─────────────┘    └──────────────┘    └────────────────────────┘  │
│         │                  │                       │                │
│         ▼                  ▼                       ▼                │
│  ┌─────────────┐    ┌──────────────┐    ┌────────────────────────┐  │
│  │ NVS Storage │    │ Data Manager │    │ Display Driver         │  │
│  │ (Persist)   │    │ (Updates)    │    │ (Render)               │  │
│  └─────────────┘    └──────────────┘    └────────────────────────┘  │
└─────────────────────────────────────────────────────────────────────┘
```

### 7.6.2 Parameter Catalog Definition

```cpp
/**
 * @file parameter_catalog.h
 * @brief Complete catalog of all available parameters for display
 */

#ifndef PARAMETER_CATALOG_H
#define PARAMETER_CATALOG_H

#include <stdint.h>

// Parameter source types
typedef enum {
    PARAM_SOURCE_J1939,      // From J1939 CAN bus
    PARAM_SOURCE_J1708,      // From J1708 serial bus
    PARAM_SOURCE_COMPUTED,   // Calculated from other parameters
    PARAM_SOURCE_STORED,     // From NVS storage
} param_source_t;

// Display widget types
typedef enum {
    WIDGET_GAUGE_CIRCULAR,   // Round gauge (RPM, speed)
    WIDGET_GAUGE_LINEAR,     // Bar gauge (fuel, temp)
    WIDGET_NUMERIC,          // Plain number display
    WIDGET_INDICATOR,        // On/off lamp
    WIDGET_TEXT,             // Text status
    WIDGET_GRAPH,            // Trend line
} widget_type_t;

// Parameter definition structure
typedef struct {
    uint16_t param_id;               // Unique parameter ID
    const char* name;                // Human-readable name
    const char* short_name;          // Abbreviated name (for small displays)
    const char* unit;                // Display unit
    param_source_t source;           // Where data comes from
    uint32_t source_id;              // PGN (J1939) or MID/PID (J1708)
    uint16_t spn;                    // SPN for J1939
    float min_value;                 // Minimum valid value
    float max_value;                 // Maximum valid value
    float warn_low;                  // Low warning threshold
    float warn_high;                 // High warning threshold
    float crit_low;                  // Critical low threshold
    float crit_high;                 // Critical high threshold
    widget_type_t preferred_widget;  // Suggested display type
    uint8_t decimal_places;          // Number of decimal places
    uint16_t update_rate_ms;         // Expected update interval
} param_definition_t;

// Master parameter catalog
static const param_definition_t PARAMETER_CATALOG[] = {
    // Engine Parameters
    {
        .param_id = 0x0001,
        .name = "Engine Speed",
        .short_name = "RPM",
        .unit = "rpm",
        .source = PARAM_SOURCE_J1939,
        .source_id = 61444,  // PGN EEC1
        .spn = 190,
        .min_value = 0,
        .max_value = 3000,
        .warn_low = 500,
        .warn_high = 2200,
        .crit_low = 0,
        .crit_high = 2500,
        .preferred_widget = WIDGET_GAUGE_CIRCULAR,
        .decimal_places = 0,
        .update_rate_ms = 100
    },
    {
        .param_id = 0x0002,
        .name = "Engine Coolant Temperature",
        .short_name = "Coolant",
        .unit = "°F",
        .source = PARAM_SOURCE_J1939,
        .source_id = 65262,  // PGN ET1
        .spn = 110,
        .min_value = -40,
        .max_value = 250,
        .warn_low = 100,
        .warn_high = 210,
        .crit_low = 50,
        .crit_high = 230,
        .preferred_widget = WIDGET_GAUGE_LINEAR,
        .decimal_places = 0,
        .update_rate_ms = 1000
    },
    {
        .param_id = 0x0003,
        .name = "Vehicle Speed",
        .short_name = "Speed",
        .unit = "mph",
        .source = PARAM_SOURCE_J1939,
        .source_id = 65265,  // PGN CCVS
        .spn = 84,
        .min_value = 0,
        .max_value = 100,
        .warn_low = -1,      // No low warning
        .warn_high = 70,
        .crit_low = -1,
        .crit_high = 80,
        .preferred_widget = WIDGET_GAUGE_CIRCULAR,
        .decimal_places = 1,
        .update_rate_ms = 100
    },
    {
        .param_id = 0x0004,
        .name = "Engine Oil Pressure",
        .short_name = "Oil PSI",
        .unit = "psi",
        .source = PARAM_SOURCE_J1939,
        .source_id = 65263,  // PGN EFL/P1
        .spn = 100,
        .min_value = 0,
        .max_value = 100,
        .warn_low = 20,
        .warn_high = -1,
        .crit_low = 10,
        .crit_high = -1,
        .preferred_widget = WIDGET_GAUGE_LINEAR,
        .decimal_places = 0,
        .update_rate_ms = 500
    },
    {
        .param_id = 0x0005,
        .name = "Fuel Level",
        .short_name = "Fuel",
        .unit = "%",
        .source = PARAM_SOURCE_J1939,
        .source_id = 65276,  // PGN DASH
        .spn = 96,
        .min_value = 0,
        .max_value = 100,
        .warn_low = 15,
        .warn_high = -1,
        .crit_low = 5,
        .crit_high = -1,
        .preferred_widget = WIDGET_GAUGE_LINEAR,
        .decimal_places = 0,
        .update_rate_ms = 5000
    },
    {
        .param_id = 0x0006,
        .name = "Boost Pressure",
        .short_name = "Boost",
        .unit = "psi",
        .source = PARAM_SOURCE_J1939,
        .source_id = 65270,  // PGN IC1
        .spn = 102,
        .min_value = 0,
        .max_value = 50,
        .warn_low = -1,
        .warn_high = 40,
        .crit_low = -1,
        .crit_high = 45,
        .preferred_widget = WIDGET_GAUGE_CIRCULAR,
        .decimal_places = 1,
        .update_rate_ms = 100
    },
    {
        .param_id = 0x0010,
        .name = "Trip Fuel Economy",
        .short_name = "Trip MPG",
        .unit = "mpg",
        .source = PARAM_SOURCE_COMPUTED,
        .source_id = 0,
        .spn = 0,
        .min_value = 0,
        .max_value = 15,
        .warn_low = 5,
        .warn_high = -1,
        .crit_low = 3,
        .crit_high = -1,
        .preferred_widget = WIDGET_NUMERIC,
        .decimal_places = 1,
        .update_rate_ms = 1000
    },
    {
        .param_id = 0x0011,
        .name = "Instant Fuel Economy",
        .short_name = "Inst MPG",
        .unit = "mpg",
        .source = PARAM_SOURCE_COMPUTED,
        .source_id = 0,
        .spn = 0,
        .min_value = 0,
        .max_value = 25,
        .warn_low = -1,
        .warn_high = -1,
        .crit_low = -1,
        .crit_high = -1,
        .preferred_widget = WIDGET_NUMERIC,
        .decimal_places = 1,
        .update_rate_ms = 500
    },
    // Transmission Parameters
    {
        .param_id = 0x0020,
        .name = "Current Gear",
        .short_name = "Gear",
        .unit = "",
        .source = PARAM_SOURCE_J1939,
        .source_id = 61445,  // PGN ETC2 (Electronic Transmission Controller 2)
        .spn = 523,
        .min_value = -2,     // Reverse
        .max_value = 10,
        .warn_low = -1,
        .warn_high = -1,
        .crit_low = -1,
        .crit_high = -1,
        .preferred_widget = WIDGET_NUMERIC,
        .decimal_places = 0,
        .update_rate_ms = 100
    },
    {
        .param_id = 0x0021,
        .name = "Transmission Oil Temperature",
        .short_name = "Trans Temp",
        .unit = "°F",
        .source = PARAM_SOURCE_J1939,
        .source_id = 65272,  // PGN TRF1 (Transmission Fluids 1)
        .spn = 178,
        .min_value = -40,
        .max_value = 300,
        .warn_low = 80,
        .warn_high = 220,
        .crit_low = 40,
        .crit_high = 250,
        .preferred_widget = WIDGET_GAUGE_LINEAR,
        .decimal_places = 0,
        .update_rate_ms = 1000
    },
    // Battery & Electrical
    {
        .param_id = 0x0030,
        .name = "Battery Voltage",
        .short_name = "Battery",
        .unit = "V",
        .source = PARAM_SOURCE_J1939,
        .source_id = 65271,  // PGN VEP1
        .spn = 168,
        .min_value = 0,
        .max_value = 32,
        .warn_low = 12.0,
        .warn_high = 15.0,
        .crit_low = 11.5,
        .crit_high = 16.0,
        .preferred_widget = WIDGET_NUMERIC,
        .decimal_places = 1,
        .update_rate_ms = 1000
    },
    // Diagnostic
    {
        .param_id = 0x0040,
        .name = "Active Fault Count",
        .short_name = "Faults",
        .unit = "",
        .source = PARAM_SOURCE_J1939,
        .source_id = 65226,  // PGN DM1
        .spn = 0,            // Computed from DM1 parsing
        .min_value = 0,
        .max_value = 99,
        .warn_low = -1,
        .warn_high = 1,      // Warn if any faults
        .crit_low = -1,
        .crit_high = 5,      // Critical if many faults
        .preferred_widget = WIDGET_INDICATOR,
        .decimal_places = 0,
        .update_rate_ms = 1000
    },
    // Stored/Trip Data
    {
        .param_id = 0x0050,
        .name = "Trip Distance",
        .short_name = "Trip Mi",
        .unit = "mi",
        .source = PARAM_SOURCE_STORED,
        .source_id = 0,
        .spn = 0,
        .min_value = 0,
        .max_value = 9999,
        .warn_low = -1,
        .warn_high = -1,
        .crit_low = -1,
        .crit_high = -1,
        .preferred_widget = WIDGET_NUMERIC,
        .decimal_places = 1,
        .update_rate_ms = 1000
    },
    {
        .param_id = 0x0051,
        .name = "Engine Hours",
        .short_name = "Hours",
        .unit = "hrs",
        .source = PARAM_SOURCE_J1939,
        .source_id = 65253,  // PGN HOURS
        .spn = 247,
        .min_value = 0,
        .max_value = 999999,
        .warn_low = -1,
        .warn_high = -1,
        .crit_low = -1,
        .crit_high = -1,
        .preferred_widget = WIDGET_NUMERIC,
        .decimal_places = 1,
        .update_rate_ms = 10000
    },
};

#define PARAM_CATALOG_SIZE (sizeof(PARAMETER_CATALOG) / sizeof(param_definition_t))

#endif // PARAMETER_CATALOG_H
```

### 7.6.3 Watch List Manager Class

```cpp
/**
 * @file watch_list_manager.h
 * @brief Manages user-selected parameters for dashboard display
 */

#ifndef WATCH_LIST_MANAGER_H
#define WATCH_LIST_MANAGER_H

#include "parameter_catalog.h"
#include <vector>
#include <functional>

#define MAX_WATCH_LIST_SIZE 32

// Watch list entry with display configuration
typedef struct {
    uint16_t param_id;           // Reference to catalog entry
    uint8_t display_slot;        // Position on dashboard (0-15)
    widget_type_t widget_type;   // Override preferred widget
    float custom_min;            // Custom display min (or use catalog)
    float custom_max;            // Custom display max (or use catalog)
    bool enabled;                // Currently active
} watch_list_entry_t;

// Current value with status
typedef struct {
    uint16_t param_id;
    float value;
    uint32_t timestamp_ms;
    bool valid;
    uint8_t status;              // 0=OK, 1=WARN, 2=CRITICAL, 3=STALE
} watch_value_t;

class WatchListManager {
public:
    WatchListManager();
    
    // Watch list management
    bool addToWatchList(uint16_t param_id, uint8_t display_slot);
    bool removeFromWatchList(uint16_t param_id);
    bool moveToSlot(uint16_t param_id, uint8_t new_slot);
    void clearWatchList();
    
    // Configuration
    bool setCustomRange(uint16_t param_id, float min_val, float max_val);
    bool setWidgetType(uint16_t param_id, widget_type_t widget);
    bool setEnabled(uint16_t param_id, bool enabled);
    
    // Value updates (called by data manager)
    void updateValue(uint16_t param_id, float value, uint32_t timestamp);
    void markStale(uint16_t param_id);
    
    // Queries
    const watch_list_entry_t* getEntry(uint16_t param_id) const;
    const param_definition_t* getParamDefinition(uint16_t param_id) const;
    watch_value_t getCurrentValue(uint16_t param_id) const;
    std::vector<watch_value_t> getAllCurrentValues() const;
    size_t getWatchListSize() const;
    
    // Persistence
    bool saveToNVS();
    bool loadFromNVS();
    
    // Callbacks for display updates
    typedef std::function<void(const watch_value_t&)> ValueUpdateCallback;
    void registerUpdateCallback(ValueUpdateCallback cb);
    
    // Iteration
    void forEachEntry(std::function<void(const watch_list_entry_t&, const param_definition_t&)> fn);
    
private:
    watch_list_entry_t m_entries[MAX_WATCH_LIST_SIZE];
    watch_value_t m_values[MAX_WATCH_LIST_SIZE];
    size_t m_count;
    ValueUpdateCallback m_callback;
    
    int findEntry(uint16_t param_id) const;
    int findCatalogEntry(uint16_t param_id) const;
    uint8_t calculateStatus(uint16_t param_id, float value) const;
};

// Global instance
extern WatchListManager watchList;

#endif // WATCH_LIST_MANAGER_H
```

### 7.6.4 Watch List Usage Example

```cpp
// In main setup
void setup() {
    // Load saved watch list from NVS
    watchList.loadFromNVS();
    
    // Or create default watch list
    if (watchList.getWatchListSize() == 0) {
        watchList.addToWatchList(0x0001, 0);   // RPM -> slot 0
        watchList.addToWatchList(0x0003, 1);   // Speed -> slot 1  
        watchList.addToWatchList(0x0002, 2);   // Coolant -> slot 2
        watchList.addToWatchList(0x0004, 3);   // Oil pressure -> slot 3
        watchList.addToWatchList(0x0005, 4);   // Fuel level -> slot 4
        watchList.addToWatchList(0x0006, 5);   // Boost -> slot 5
        watchList.addToWatchList(0x0010, 6);   // Trip MPG -> slot 6
        watchList.saveToNVS();
    }
    
    // Register callback for display updates
    watchList.registerUpdateCallback([](const watch_value_t& val) {
        updateDashboardWidget(val.param_id, val.value, val.status);
    });
}

// In data manager when new J1939 data arrives
void onJ1939DataReceived(uint32_t pgn, const j1939_data_t& data) {
    // Map PGN/SPN to param_id and update watch list
    switch (pgn) {
        case 61444:  // EEC1
            watchList.updateValue(0x0001, data.engine_speed, millis());
            break;
        case 65262:  // ET1
            watchList.updateValue(0x0002, data.coolant_temp, millis());
            break;
        // ... etc
    }
}
```

### 7.6.5 Default Watch List Recommendations

| Slot | Parameter | PGN/SPN | Widget | Priority | Update Rate |
|------|-----------|---------|--------|----------|-------------|
| 0 | Engine Speed (RPM) | 61444/190 | Circular Gauge | Critical | 10 Hz |
| 1 | Vehicle Speed (mph) | 65265/84 | Circular Gauge | Critical | 10 Hz |
| 2 | Engine Coolant Temp | 65262/110 | Linear Bar | High | 1 Hz |
| 3 | Engine Oil Pressure | 65263/100 | Linear Bar | High | 2 Hz |
| 4 | Fuel Level | 65276/96 | Linear Bar | Medium | 0.2 Hz |
| 5 | Boost Pressure | 65270/102 | Circular Gauge | Medium | 5 Hz |
| 6 | Transmission Oil Temp | 65272/177 | Linear Bar | Medium | 1 Hz |
| 7 | Current Gear | 65272/523 | Numeric | High | 10 Hz |
| 8 | Trip MPG | Computed | Numeric | Info | 1 Hz |
| 9 | Instant MPG | Computed | Numeric | Info | 2 Hz |
| 10 | Trip Distance | Stored | Numeric | Info | 1 Hz |
| 11 | Engine Hours | 65253/247 | Numeric | Info | 0.1 Hz |
| 12 | Active Fault Count | DM1 | Indicator | High | 1 Hz |
| 13 | Battery Voltage | 65271/168 | Numeric | Medium | 1 Hz |
| 14 | Intake Air Temp | 65270/105 | Numeric | Low | 1 Hz |
| 15 | Throttle Position | 61443/91 | Linear Bar | Info | 10 Hz |

---

## 8. Phase Implementation Plan

### Phase 0: Pre-Hardware Preparation
**Duration:** 3-4 weeks (while waiting for hardware)  
**Goal:** Build and validate complete J1939/J1708 parsing libraries using real test data. This is the **core foundation** of the project.

#### Phase 0.1: Development Environment Setup
- [ ] Install VS Code with PlatformIO extension
- [ ] Create project structure as defined in Section 6.2
- [ ] Initialize Git repository
- [ ] Configure platformio.ini for ESP32 DevKit V1
- [ ] Set up Python virtual environment for test tools
- [ ] Install pytest for unit testing
- [ ] Create README with project overview

#### Phase 0.2: Test Data Acquisition
**Critical:** Acquire real J1939 CAN logs for library validation
- [ ] Clone [DieselDuz42/Arduino-CAN-bus-SD-logger](https://github.com/DieselDuz42/Arduino-CAN-bus-SD-logger) for example logs
- [ ] Download sample data from [CSS Electronics](https://www.csselectronics.com/) (free demo files)
- [ ] Create test_data/ directory structure:
  ```
  test_data/
  ├── real_logs/           # Downloaded real truck data
  │   ├── cummins_isx/
  │   └── generic_j1939/
  ├── synthetic/           # Generated test scenarios
  │   ├── idle_scenario.log
  │   ├── highway_scenario.log
  │   └── fault_scenario.log
  └── expected_results/    # Expected decoded values for validation
  ```
- [ ] Convert log formats (SocketCAN, CSV, candump) to common format
- [ ] Document data sources and any licensing requirements
- [ ] Identify PGNs/SPNs present in real logs for test coverage

#### Phase 0.3: J1939 Parser Library Development
**Goal:** Complete, tested J1939 decoding library
- [ ] Implement J1939 PGN extraction from 29-bit CAN ID
  ```cpp
  uint32_t extract_pgn(uint32_t can_id);
  uint8_t extract_source_address(uint32_t can_id);
  uint8_t extract_priority(uint32_t can_id);
  ```
- [ ] Implement SPN decoding with scale/offset
  ```cpp
  float decode_spn(const uint8_t* data, const spn_definition_t* def);
  ```
- [ ] Create PGN handler registry (function pointer table)
- [ ] Implement specific PGN decoders:
  - [ ] PGN 61444 (EEC1) - Engine Speed, Torque
  - [ ] PGN 65262 (ET1) - Coolant Temperature
  - [ ] PGN 65263 (EFL/P1) - Oil Pressure
  - [ ] PGN 65265 (CCVS) - Vehicle Speed
  - [ ] PGN 65270 (IC1) - Boost Pressure
  - [ ] PGN 65276 (DASH) - Fuel Level
  - [ ] PGN 65253 (HOURS) - Engine Hours
  - [ ] PGN 65203 (LFE) - Fuel Rate
  - [ ] PGN 65226 (DM1) - Active Fault Codes
- [ ] Implement Transport Protocol (BAM) reassembly
- [ ] Handle special values (0xFF = N/A, 0xFE = Error)
- [ ] Create unit tests for each PGN decoder
- [ ] **Validate against real log data** - compare decoded values to expected
- [ ] Achieve 100% test coverage on parser functions

#### Phase 0.4: J1708/J1587 Parser Library Development
- [ ] Document J1587 MID and PID assignments for ABS
- [ ] Define data structures for J1708 messages
- [ ] Implement checksum calculation and validation
- [ ] Implement message framing logic (inter-byte timing)
- [ ] Create PID decoders for common ABS parameters:
  - [ ] PID 168 - Battery Voltage
  - [ ] PID 171 - Ambient Air Temperature
  - [ ] PID 245 - Total Vehicle Distance
  - [ ] Diagnostic PIDs for Bendix ABS
- [ ] Create unit tests for parser functions
- [ ] Test with synthetic J1708 data

#### Phase 0.5: Python Test Data Generator
Implement the generator framework from Section 7.5.6:
- [ ] Create `J1939TestDataGenerator` class
- [ ] Implement driving scenarios:
  - [ ] `generate_idle_scenario()` - Engine at idle, vehicle stopped
  - [ ] `generate_highway_scenario()` - Steady cruise
  - [ ] `generate_acceleration_scenario()` - Speed/RPM ramp up
  - [ ] `generate_fault_scenario()` - Inject DM1 fault codes
  - [ ] `generate_cold_start_scenario()` - Temperature ramp up
- [ ] Implement output formatters:
  - [ ] SocketCAN log format
  - [ ] CSV format
  - [ ] Binary format (for ESP32 serial injection)
- [ ] Add realistic noise and variation to signals
- [ ] Create correlated signals (RPM ↔ Fuel Rate ↔ Speed)
- [ ] Generate comprehensive test dataset (10+ hours of data)

#### Phase 0.6: Watch List Manager Development
Implement the parameter selection system from Section 7.6:
- [ ] Create `parameter_catalog.h` with all available parameters
- [ ] Implement `WatchListManager` class:
  - [ ] `addToWatchList()` / `removeFromWatchList()`
  - [ ] `updateValue()` / `getCurrentValue()`
  - [ ] `saveToNVS()` / `loadFromNVS()`
  - [ ] Value update callbacks
- [ ] Define default watch list (12 primary parameters)
- [ ] Create widget type definitions
- [ ] Implement warning/critical threshold checking
- [ ] Unit test watch list persistence
- [ ] Create CLI tool to configure watch list

#### Phase 0.7: Integration Testing (Desktop Simulation)
- [ ] Create desktop test harness (runs parser code on PC)
- [ ] Feed real log files through parser
- [ ] Verify all decoded values match expected
- [ ] Test watch list updates with streaming data
- [ ] Profile performance (parsing rate, memory usage)
- [ ] Create automated test suite (pytest + GitHub Actions)

#### Phase 0.8: Documentation
- [ ] Complete hardware reference document
- [ ] Create wiring diagrams
- [ ] Document all PGN/SPN implementations
- [ ] Write API documentation for parser libraries
- [ ] Create test plan with acceptance criteria
- [ ] Document watch list configuration process

**Phase 0 Deliverables:**
| Deliverable | Description | Acceptance Criteria |
|-------------|-------------|---------------------|
| J1939 Parser Library | Complete PGN/SPN decoding | All unit tests pass, validated against real logs |
| J1708 Parser Library | J1587 message parsing | Unit tests pass, checksum validation works |
| Test Data Generator | Python scenario generator | Generates realistic multi-hour datasets |
| Parameter Catalog | Complete parameter definitions | All display parameters defined with thresholds |
| Watch List Manager | Parameter selection system | Add/remove/persist watch list items |
| Test Suite | Automated validation | 100% coverage, CI integration |
| Documentation | Complete technical docs | API docs, wiring diagrams, test plan |

**Success Metrics:**
- [ ] Parse 1000+ real J1939 frames from Cummins logs correctly
- [ ] All 9 core PGN decoders implemented and tested
- [ ] Watch list persists across simulated reboots
- [ ] Python generator creates valid test scenarios
- [ ] Zero parsing errors on 10-hour synthetic dataset

**Phase 0 Exit Criteria (Must complete before Phase 1):**
- [ ] `pytest` passes with 100% test coverage on parser modules
- [ ] At least one real J1939 log file fully decoded and validated
- [ ] Watch list manager compiles and runs on desktop simulator
- [ ] All documentation reviewed and complete

---

### Phase 1: Bench Testing with Simulated Data
**Duration:** 1-2 weeks  
**Goal:** Verify parsing logic on actual ESP32 hardware with mock data  
**Prerequisites:** Phase 0 complete, hardware received

#### Phase 1.1: Hardware Assembly
- [ ] Verify ESP32 DevKit V1 functionality
- [ ] Connect SN65HVD230 CAN transceiver
- [ ] Connect RS485 adapter
- [ ] Test basic GPIO functionality

#### Phase 1.2: CAN Driver Integration
- [ ] Initialize ESP32 TWAI at 250 kbps
- [ ] Configure for extended (29-bit) frames
- [ ] Test CAN loopback mode (TX→RX shorted)
- [ ] Verify interrupt-based reception

#### Phase 1.3: J1939 Parser Testing on ESP32
- [ ] Port desktop-tested parser to ESP32
- [ ] Inject test data via serial (from Python generator)
- [ ] Feed simulated PGN 61444 (Engine RPM)
- [ ] Feed simulated PGN 65262 (Coolant Temp)
- [ ] Feed simulated DM1 messages
- [ ] Verify all scaling calculations match desktop tests
- [ ] Test Transport Protocol (BAM) assembly

#### Phase 1.4: J1708 Driver Integration
- [ ] Initialize UART2 at 9600 bps
- [ ] Test RS485 direction control
- [ ] Verify message framing
- [ ] Test checksum validation

#### Phase 1.5: Serial Output & Watch List
- [ ] Create formatted serial output
- [ ] Display decoded parameters via watch list
- [ ] Show raw hex for debugging
- [ ] Implement logging format
- [ ] Test watch list NVS persistence

**Success Criteria:**
- CAN frames received and parsed correctly
- J1708 messages assembled properly
- All parameter values match expected from test data
- Watch list correctly filters displayed parameters
- No crashes during extended operation (1+ hour)

---

### Phase 2: Vehicle Integration & Initial Testing
**Duration:** 2-3 weeks  
**Goal:** Connect to actual truck and verify live data reception

#### Phase 2.1: Pre-Connection Verification
- [ ] Measure vehicle diagnostic port voltages
- [ ] Verify CAN bus termination (~60Ω between C and D)
- [ ] Confirm 12V ACC power availability
- [ ] Document vehicle-specific details

#### Phase 2.2: Initial Connection
- [ ] Connect CAN transceiver to vehicle (USB power initially)
- [ ] Connect J1708 transceiver to vehicle
- [ ] Monitor for CAN traffic
- [ ] Capture initial data samples

#### Phase 2.3: Data Verification
- [ ] Compare decoded RPM with dashboard
- [ ] Compare decoded temperature with dashboard
- [ ] Verify transmission gear reading
- [ ] Check for missing/unknown PGNs

#### Phase 2.4: J1708 Verification
- [ ] Monitor for ABS traffic
- [ ] Capture any fault codes
- [ ] Verify message structure
- [ ] Document observed MIDs

#### Phase 2.5: Stability Testing
- [ ] Run for extended periods (1+ hours)
- [ ] Monitor for buffer overflows
- [ ] Check error counters
- [ ] Verify no vehicle bus interference

**Success Criteria:**
- Stable connection to both buses
- Real vehicle data matching dashboard
- No adverse effects on vehicle operation
- System runs without crashes for extended period

---

### Phase 3: Complete Data Logging & Decoding
**Duration:** 2-3 weeks  
**Goal:** Comprehensive parameter coverage, data recording, and persistent storage

#### Phase 3.1: Extended PGN Coverage
- [ ] Identify all PGNs on vehicle bus
- [ ] Add decoders for relevant PGNs
- [ ] Handle unknown/proprietary PGNs
- [ ] Document coverage gaps

#### Phase 3.2: Diagnostic Code Handling
- [ ] Implement full DM1 parsing
- [ ] Implement DM2 request (if needed)
- [ ] Parse ABS codes from J1708
- [ ] Create fault code lookup tables

#### Phase 3.3: Data Logging
- [ ] Implement timestamped logging
- [ ] Create CSV output format
- [ ] Test with multi-hour recording
- [ ] Implement log rotation/management

#### Phase 3.4: Persistent Storage (NVS)
- [ ] Initialize NVS partition
- [ ] Implement trip data storage (Trip A/B)
- [ ] Implement lifetime statistics storage
- [ ] Implement fuel economy calculation and storage
- [ ] Implement fault code history storage
- [ ] Add power-loss detection for emergency save
- [ ] Test data persistence across power cycles
- [ ] Implement trip reset functionality

#### Phase 3.5: Performance Optimization
- [ ] Profile task execution times
- [ ] Optimize buffer sizes
- [ ] Reduce memory usage
- [ ] Improve parsing efficiency
- [ ] Tune NVS write intervals for wear leveling

**Success Criteria:**
- All key parameters decoded
- Active DTCs displayed correctly
- Data logging functional
- Trip/MPG data persists after power cycle
- Fault codes retained in history
- System stable for 8+ hours

---

### Phase 4: Basic Display Integration
**Duration:** 2-3 weeks  
**Goal:** Visual dashboard showing key parameters

#### Phase 4.1: Display Hardware
- [ ] Select and procure TFT display
- [ ] Wire display to ESP32 (SPI)
- [ ] Test display initialization
- [ ] Verify basic graphics

#### Phase 4.2: Display Layout
- [ ] Design screen layout(s)
- [ ] Create display update task
- [ ] Implement page navigation
- [ ] Add status indicators

#### Phase 4.3: Parameter Display
- [ ] Show RPM, speed, temps
- [ ] Show transmission info
- [ ] Show fuel levels
- [ ] Show active faults

#### Phase 4.4: Vehicle Installation
- [ ] Create enclosure/mounting
- [ ] Wire to ACC power
- [ ] Test in vehicle
- [ ] Refine layout based on usage

**Success Criteria:**
- Readable display in vehicle
- 2Hz+ refresh rate
- All key parameters visible
- Stable operation during driving

---

### Phase 5: Advanced Features (Future)
**Duration:** Ongoing  
**Goal:** Enhanced functionality and integration

#### Phase 5.1: Enhanced Display (LVGL)
- [ ] Migrate to LVGL framework
- [ ] Create graphical gauges
- [ ] Add historical graphs
- [ ] Improve visual design

#### Phase 5.2: Wi-Fi Integration
- [ ] Enable ESP32 Wi-Fi
- [ ] Create web interface
- [ ] Implement MQTT client
- [ ] Connect to Home Assistant

#### Phase 5.3: Additional Sensors
- [ ] Add EGT thermocouple
- [ ] Add ambient temperature sensors
- [ ] Add fuel level sensors
- [ ] Integrate readings into display

#### Phase 5.4: Remote Start Integration
- [ ] Design safety interlocks
- [ ] Implement relay control
- [ ] Create Home Assistant automation
- [ ] Test thoroughly

---

## 9. Data Parameters Catalog

### 9.1 Engine Parameters (J1939)

| Parameter | SPN | PGN | Scale | Offset | Unit | Range |
|-----------|-----|-----|-------|--------|------|-------|
| Engine Speed | 190 | 61444 | 0.125 | 0 | RPM | 0-8031.875 |
| Engine Load | 92 | 61443 | 1 | 0 | % | 0-125 |
| Throttle Position | 91 | 61443 | 0.4 | 0 | % | 0-100 |
| Coolant Temperature | 110 | 65262 | 1 | -40 | °C | -40 to 210 |
| Oil Temperature | 175 | 65262 | 0.03125 | -273 | °C | -273 to 1735 |
| Oil Pressure | 100 | 65263 | 4 | 0 | kPa | 0-1000 |
| Intake Manifold Temp | 105 | 65270 | 1 | -40 | °C | -40 to 210 |
| Boost Pressure | 102 | 65270 | 2 | 0 | kPa | 0-500 |
| Fuel Rate | 183 | 65266 | 0.05 | 0 | L/h | 0-3212.75 |
| Battery Voltage | 168 | 65271 | 0.05 | 0 | V | 0-3212.75 |
| Engine Hours | 247 | 65253 | 0.05 | 0 | hours | 0-210554060.75 |

### 9.2 Transmission Parameters (J1939)

| Parameter | SPN | PGN | Scale | Offset | Unit | Range |
|-----------|-----|-----|-------|--------|------|-------|
| Current Gear | 523 | 65272 | 1 | -125 | gear | -125 to 125 |
| Selected Gear | 524 | 65272 | 1 | -125 | gear | -125 to 125 |
| Torque Converter Lockup | 573 | 65272 | - | - | bit | 0/1 |
| Trans Oil Temperature | 177 | 65272 | 1 | -40 | °C | -40 to 210 |
| Output Shaft Speed | 191 | 61442 | 0.125 | 0 | RPM | 0-8031.875 |

### 9.3 Vehicle Parameters (J1939)

| Parameter | SPN | PGN | Scale | Offset | Unit | Range |
|-----------|-----|-----|-------|--------|------|-------|
| Wheel-Based Speed | 84 | 65265 | 1/256 | 0 | km/h | 0-250.996 |
| Cruise Control Speed | 86 | 65265 | 1 | 0 | km/h | 0-250 |
| Fuel Level | 96 | 65276 | 0.4 | 0 | % | 0-100 |
| Ambient Air Temp | 171 | 65269 | 0.03125 | -273 | °C | -273 to 1735 |
| Barometric Pressure | 108 | 65269 | 0.5 | 0 | kPa | 0-125 |

### 9.4 Diagnostic Parameters (J1939)

| Parameter | Description | PGN | Notes |
|-----------|-------------|-----|-------|
| Active DTCs | Currently active faults | 65226 (DM1) | Transport Protocol if > 1 |
| Previously Active DTCs | Stored/inactive faults | 65227 (DM2) | On-request only |

### 9.5 DTC Structure

```
┌───────────────┬─────────────┬─────────────┬─────────────┐
│ SPN (19 bits) │ FMI (5 bits)│ CM (1 bit)  │ OC (7 bits) │
│               │             │             │             │
│ Suspect Param │ Failure Mode│ Conversion  │ Occurrence  │
│   Number      │  Identifier │   Method    │   Count     │
└───────────────┴─────────────┴─────────────┴─────────────┘
       Bytes 1-2.5       2.5-3        3           4
```

**Common FMI Values:**
| FMI | Description |
|-----|-------------|
| 0 | Data Valid But Above Normal Operational Range |
| 1 | Data Valid But Below Normal Operational Range |
| 2 | Data Erratic, Intermittent Or Incorrect |
| 3 | Voltage Above Normal |
| 4 | Voltage Below Normal |
| 5 | Current Below Normal |
| 6 | Current Above Normal |
| 7 | Mechanical System Not Responding |

### 9.6 Stored/Computed Parameters

These parameters are calculated from raw bus data and persisted in NVS.

#### 9.6.1 Trip Data (Reset Each Trip)

| Parameter | Unit | Calculation | Storage |
|-----------|------|-------------|---------|
| Trip Distance | km | ∫ vehicle_speed × dt | trip_data.distance_km |
| Trip Fuel | L | ∫ fuel_rate × dt | trip_data.fuel_used_L |
| Trip Time | seconds | Elapsed ignition-on time | trip_data.trip_duration_s |
| Trip Idle Time | seconds | Time with speed < 1 km/h | trip_data.idle_time_s |
| Trip MPG | mpg | (trip_distance / trip_fuel) × 2.352 | Computed |
| Average Speed | km/h | trip_distance / (trip_time - idle_time) | Computed |

#### 9.6.2 Lifetime Statistics (Never Reset)

| Parameter | Unit | Description | Storage Key |
|-----------|------|-------------|-------------|
| Total Distance | km | Cumulative driven distance | lifetime.total_distance_km |
| Total Fuel | L | Cumulative fuel consumed | lifetime.total_fuel_L |
| Total Engine Hours | hours | Cumulative running time | lifetime.engine_hours |
| Total Idle Hours | hours | Cumulative idle time | lifetime.idle_hours |
| Lifetime MPG | mpg | Overall fuel economy | Computed |
| Last Odometer Sync | km | Vehicle odometer at sync | lifetime.odo_sync_km |

#### 9.6.3 Fault Code History

| Field | Type | Description |
|-------|------|-------------|
| SPN | uint32_t | Suspect Parameter Number |
| FMI | uint8_t | Failure Mode Identifier |
| Occurrence Count | uint16_t | Times fault has occurred |
| First Seen | uint32_t | Epoch timestamp first occurrence |
| Last Seen | uint32_t | Epoch timestamp last occurrence |
| Source Address | uint8_t | ECU that reported fault |
| Active Flag | bool | Currently active on bus |

#### 9.6.4 Fuel Economy Calculations

```c
/**
 * Convert between fuel economy units
 */
// L/100km to MPG (US)
float l100km_to_mpg(float l_per_100km) {
    return 235.215f / l_per_100km;
}

// MPG (US) to L/100km
float mpg_to_l100km(float mpg) {
    return 235.215f / mpg;
}

// Calculate instantaneous MPG from J1939 fuel rate
// PGN 65203 (LFE) fuel_rate in L/h, speed in km/h
float calc_instant_mpg(float fuel_rate_L_h, float speed_km_h) {
    if (fuel_rate_L_h < 0.1f) return 0.0f;  // Avoid divide by zero
    float l_per_100km = (fuel_rate_L_h * 100.0f) / speed_km_h;
    return l100km_to_mpg(l_per_100km);
}

// Calculate trip MPG
float calc_trip_mpg(float distance_km, float fuel_L) {
    if (fuel_L < 0.01f) return 0.0f;
    float l_per_100km = (fuel_L / distance_km) * 100.0f;
    return l100km_to_mpg(l_per_100km);
}
```

#### 9.6.5 Distance Accumulation

```c
/**
 * Integrate vehicle speed over time for distance calculation
 * Called periodically from data manager task
 */
void accumulate_distance(float speed_km_h, uint32_t delta_ms) {
    // Convert speed (km/h) and time (ms) to distance (km)
    // speed_km_h × (delta_ms / 3600000) = distance in km
    float distance_km = speed_km_h * ((float)delta_ms / 3600000.0f);
    
    trip_data.distance_km += distance_km;
    lifetime_stats.total_distance_km += distance_km;
    
    // Track idle vs moving time
    if (speed_km_h < 1.0f) {
        trip_data.idle_time_s += delta_ms / 1000;
    }
    trip_data.trip_duration_s += delta_ms / 1000;
}
```

---

## 10. Testing Strategy

### 10.1 Unit Testing

| Module | Test Cases |
|--------|------------|
| J1939 Parser | PGN extraction, SPN scaling, Transport Protocol |
| J1587 Parser | Checksum validation, message framing |
| Data Manager | Thread safety, data freshness |

### 10.2 Integration Testing

| Test | Description | Pass Criteria |
|------|-------------|---------------|
| CAN Loopback | TX/RX shorted, verify echo | Messages match |
| J1708 Loopback | Feed known message | Correct parsing |
| Multi-bus | Both buses active | No conflicts |

### 10.3 Vehicle Testing

| Test | Duration | Criteria |
|------|----------|----------|
| Idle Test | 30 minutes | Stable, no errors |
| Drive Test | 1 hour | All parameters valid |
| Cold Start | Varies | Proper initialization |
| Extended Run | 8 hours | No memory leaks/crashes |

### 10.4 Parser Validation Testing

| Test Category | Test Cases | Expected Result |
|--------------|------------|-----------------|
| PGN Extraction | 100 known CAN IDs | 100% correct PGN/SA/Priority |
| SPN Decoding | Each SPN with known data | Values within 0.1% of expected |
| Special Values | 0xFF, 0xFE bytes | Return "N/A" or "Error" status |
| Transport Protocol | Multi-packet DM1 | Correctly reassembled payload |
| Edge Cases | Zero values, max values | Handle without overflow |
| Invalid Data | Malformed frames | Graceful rejection, no crash |

### 10.5 NVS Persistence Testing

| Test | Procedure | Pass Criteria |
|------|-----------|---------------|
| Save/Load Trip | Save trip, reboot, load | Values match ±0.001 |
| Power Loss Simulation | Kill power during write | Data recoverable, no corruption |
| Wear Leveling | 10,000 write cycles | NVS still functional |
| Namespace Isolation | Corrupt one namespace | Other namespaces unaffected |
| Default Values | First boot, no prior data | Defaults applied correctly |

---

## 11. Risk Assessment & Mitigation

### 11.1 Technical Risks

| Risk | Probability | Impact | Mitigation Strategy |
|------|------------|--------|---------------------|
| **CAN bus interference** | Medium | High | Read-only mode, high-impedance input, extensive vehicle testing |
| **CAN bus termination error** | Low | High | NEVER add 120Ω termination resistor to your device - truck already has proper termination at bus ends. Adding termination will corrupt communications |
| **Incorrect PGN decoding** | Medium | Medium | Validate against real logs, cross-reference with CSS Electronics tools |
| **J1708 timing issues** | Medium | Medium | Use hardware timer for inter-byte timing, test with scope |
| **Flash wear (NVS)** | Low | High | Batch writes, 5-minute intervals, monitor wear indicators |
| **Memory exhaustion** | Medium | High | Static allocation, memory pools, regular profiling |
| **CAN bus overload** | Low | Medium | Passive mode only, no transmit except requests |
| **Transceiver damage** | Low | High | Proper ESD protection, isolated power supply |
| **Transport Protocol errors** | Medium | Low | Timeout handling, packet sequence validation |

### 11.2 Schedule Risks

| Risk | Probability | Impact | Mitigation Strategy |
|------|------------|--------|---------------------|
| **Hardware delivery delay** | Medium | Medium | Phase 0 designed to be hardware-independent |
| **Scope creep** | High | Medium | Strict phase boundaries, defer non-critical features |
| **Unknown PGNs** | Medium | Low | Implement generic PGN logger for analysis |
| **Vehicle access limited** | Medium | Medium | Maximize bench testing, portable setup |

### 11.3 Safety Considerations

| Hazard | Risk Level | Control Measure |
|--------|------------|-----------------|
| Vehicle bus disruption | Medium | Read-only by default, isolate via high-impedance transceivers |
| Distracted driving | Low | Simple display, audio alerts for critical issues |
| Electrical fire | Very Low | Fused connections, automotive-grade wiring |
| Data misinterpretation | Low | Validate all readings against stock gauges first |
| Remote start unintended activation | High (future) | Multiple interlocks, require park + brake + no faults |

---

## 12. Resource Budget & Constraints

### 12.1 ESP32 Memory Budget

| Resource | Total Available | Allocated | Remaining | Notes |
|----------|----------------|-----------|-----------|-------|
| **Flash (Code)** | 4 MB | ~800 KB | 3.2 MB | Arduino core + app |
| **RAM (DRAM)** | 320 KB | ~180 KB | 140 KB | See breakdown below |
| **NVS Partition** | 20 KB | ~8 KB | 12 KB | Trip + lifetime + DTCs |
| **PSRAM** | 0 (not equipped) | - | - | Use external if needed |

### 12.2 RAM Allocation Breakdown

| Component | Estimated Size | Notes |
|-----------|---------------|-------|
| FreeRTOS + Arduino | ~60 KB | OS overhead |
| CAN RX Buffer | 8 KB | 256 frames × 32 bytes |
| J1708 RX Buffer | 1 KB | 50 messages × 21 bytes |
| J1939 Parser State | 4 KB | Transport Protocol, PGN tables |
| Watch List Manager | 2 KB | 32 entries + values |
| Data Manager | 4 KB | All parameter values + timestamps |
| Display Buffer | 20 KB | For TFT framebuffer (Phase 4) |
| JSON Serialization | 8 KB | ArduinoJson document |
| Stack Space (Tasks) | 20 KB | 5 tasks × 4 KB each |
| WiFi/BT (if enabled) | 60 KB | Significant overhead |
| **Total Estimated** | ~180 KB | Leaves ~140 KB headroom |

### 12.3 Processing Budget

| Task | Priority | Max Execution Time | Frequency |
|------|----------|-------------------|-----------|
| CAN Receive | 5 (highest) | 100 µs | On message |
| J1939 Parse | 4 | 500 µs | Per frame |
| J1708 Receive | 4 | 200 µs | On byte |
| Data Manager Update | 3 | 100 µs | 10 Hz |
| Display Refresh | 3 | 10 ms | 5 Hz |
| NVS Write | 2 (lowest) | 50 ms | Every 5 min |

### 12.4 Power Budget

| Component | Current (mA) | Voltage | Power (mW) |
|-----------|-------------|---------|------------|
| ESP32 (active, WiFi off) | 80 | 3.3V | 264 |
| ESP32 (active, WiFi on) | 180 | 3.3V | 594 |
| SN65HVD230 CAN | 10 | 3.3V | 33 |
| MAX485 RS485 | 1 | 3.3V | 3 |
| TFT Display (3.5") | 80 | 3.3V | 264 |
| Voltage Regulator Loss | - | - | 200 |
| **Total (no WiFi)** | ~180 | - | 760 |
| **Total (with WiFi)** | ~280 | - | 1100 |

**At 12V vehicle supply:** ~100 mA typical, ~150 mA with WiFi

### 12.5 Timeline Summary

| Phase | Duration | Cumulative | Key Milestone |
|-------|----------|------------|---------------|
| Phase 0: Pre-Hardware | 3-4 weeks | Week 4 | Parser library validated |
| Phase 1: Bench Testing | 1-2 weeks | Week 6 | Hardware verified |
| Phase 2: Vehicle Integration | 2-3 weeks | Week 9 | Live data flowing |
| Phase 3: Complete Logging | 2-3 weeks | Week 12 | All features working |
| Phase 4: Display | 2-3 weeks | Week 15 | Visual dashboard |
| Phase 5: Advanced | Ongoing | - | Continuous improvement |

**Total to functional dashboard: 12-15 weeks**

---

## 13. Future Expansion

### 13.1 Home Assistant Integration

```yaml
# Example Home Assistant configuration
mqtt:
  sensor:
    - name: "Truck Engine RPM"
      state_topic: "truck/engine/rpm"
      unit_of_measurement: "RPM"
      
    - name: "Truck Coolant Temp"
      state_topic: "truck/engine/coolant_temp"
      unit_of_measurement: "°C"
      
    - name: "Truck Active Faults"
      state_topic: "truck/diagnostics/active_dtc_count"
```

### 13.2 Remote Start Considerations

**Safety Interlocks Required:**
- Transmission in Park/Neutral (J1939 verification)
- Parking brake engaged (J1939 verification)
- No active critical faults
- Coolant temperature limits
- Run time limits

### 13.3 Additional Sensors

| Sensor | Interface | Purpose |
|--------|-----------|---------|
| EGT Probe (K-type) | SPI (MAX31855) | Exhaust monitoring |
| DS18B20 | 1-Wire | Ambient/cabin temp |
| Fuel Level Sender | Analog (ADC) | Auxiliary tank |
| GPS Module | UART/I2C | Location logging |

---

## 14. References & Resources

### 14.1 Official Standards (SAE)

- SAE J1939-21: Data Link Layer
- SAE J1939-71: Vehicle Application Layer
- SAE J1939-73: Diagnostics
- SAE J1939-13: Off-Board Diagnostic Connector
- SAE J1708: Serial Data Communications
- SAE J1587: Electronic Data Interchange

### 14.2 Technical References

| Resource | URL |
|----------|-----|
| ESP32 Technical Reference | https://www.espressif.com/sites/default/files/documentation/esp32_technical_reference_manual_en.pdf |
| ESP32 TWAI Driver | https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/twai.html |
| SN65HVD230 Datasheet | https://www.ti.com/lit/ds/symlink/sn65hvd230.pdf |
| Waveshare SN65HVD230 Wiki | https://www.waveshare.com/wiki/SN65HVD230_CAN_Board |
| J1939 Explained | https://www.csselectronics.com/pages/j1939-explained-simple-intro-tutorial |
| Copperhill J1708 Guide | https://copperhilltech.com/blog/monitoring-sae-j1708j1587-data-traffic-using-the-arduino-mega2560-or-arduino-due/ |

### 14.3 Code Libraries

| Library | Repository | Purpose |
|---------|------------|---------|
| ESP32 Arduino Core | https://github.com/espressif/arduino-esp32 | ESP32 support |
| J1708 Library | https://github.com/jeremydaily/J1708 | J1708 parsing |
| TFT_eSPI | https://github.com/Bodmer/TFT_eSPI | Display driver |
| LVGL | https://github.com/lvgl/lvgl | GUI framework |

### 14.4 Project Files

- [Custom Truck Data Logging & Display Project.pdf](./Custom%20Truck%20Data%20Logging%20%26%20Display%20Project.pdf) - Original project document
- [HARDWARE_REFERENCE.md](./HARDWARE_REFERENCE.md) - Detailed hardware specifications (to be created)
- [J1939_PGN_CATALOG.md](./J1939_PGN_CATALOG.md) - Complete PGN/SPN reference (to be created)

---

## Appendix A: Glossary

| Term | Definition |
|------|------------|
| BAM | Broadcast Announce Message (J1939 Transport Protocol) |
| CAN | Controller Area Network |
| DTC | Diagnostic Trouble Code |
| ECM | Engine Control Module |
| ECU | Electronic Control Unit |
| FMI | Failure Mode Identifier |
| MID | Message Identifier (J1587) |
| OC | Occurrence Count |
| PDU | Protocol Data Unit |
| PGN | Parameter Group Number (J1939 message identifier) |
| PID | Parameter Identifier (J1587) |
| SPN | Suspect Parameter Number (J1939 signal identifier) |
| TCM | Transmission Control Module |
| TP | Transport Protocol |
| TWAI | Two-Wire Automotive Interface (ESP32 CAN controller) |

---

## Appendix B: Troubleshooting Guide

### B.1 Hardware Issues

| Issue | Possible Cause | Diagnostic Steps | Solution |
|-------|----------------|------------------|----------|
| No CAN traffic | Wrong baud rate | Check with oscilloscope | Verify 250 kbps in config |
| No CAN traffic | Transceiver wiring | Measure voltage on CANH/CANL | Should see differential ~2V |
| No CAN traffic | CAN not enabled on vehicle | Try key ON, engine running | Some ECUs sleep with key OFF |
| CAN errors | Missing termination | Measure resistance C↔D | Should be ~60Ω (two 120Ω in parallel) |
| CAN errors | CANH/CANL swapped | Check wire colors | Yellow=H, Green=L typically |
| J1708 no data | RS485 direction stuck | Verify DE/RE control | Tie DE to GND for RX-only |
| J1708 checksum fail | Wrong baud/format | Verify serial settings | Must be 9600-8N1 |
| J1708 partial data | Inter-byte gaps | Check timing | Need hardware timer for framing |
| ADC readings wrong | Using ADC2 with WiFi | Check pin assignments | Switch to ADC1 pins only |
| ADC readings noisy | Missing filtering | Add RC filter | 10K + 0.1µF on ADC input |
| ESP32 boot fails | GPIO 12 pulled high | Check boot strapping | Keep GPIO 12 low at boot |
| ESP32 brownout | Insufficient power | Check voltage rail | Use quality 5V supply |
| Display flicker | Slow update rate | Profile render time | Use DMA, reduce redraws |
| Display white/blank | SPI misconfigured | Verify pin assignments | Check TFT_eSPI User_Setup.h |

### B.2 Software Issues

| Issue | Possible Cause | Diagnostic Steps | Solution |
|-------|----------------|------------------|----------|
| PGN not decoded | Unknown PGN | Log raw CAN ID | Add decoder or log as unknown |
| SPN value wrong | Scale/offset error | Compare raw bytes to spec | Verify J1939-71 parameters |
| Values stuck | Data freshness | Check timestamps | Implement stale detection |
| Memory leak | Unbounded allocation | Monitor free heap | Use static allocation |
| Task crash | Stack overflow | Increase stack size | Profile actual usage |
| NVS write fails | Partition full | Check NVS stats | Clear old data or increase partition |
| NVS corrupt | Power loss during write | Check clean shutdown flag | Implement recovery |
| Transport Protocol fail | Timeout or sequence | Check TP.CM/TP.DT frames | Verify sequence numbers |

### B.3 Vehicle-Specific Issues

| Vehicle Type | Common Issue | Solution |
|--------------|--------------|----------|
| Pre-2007 trucks | J1708 primary, limited J1939 | Focus on J1708 interface |
| 2007+ Cummins | DPF regeneration PGNs proprietary | Log and reverse-engineer |
| Allison transmission | Some PGNs are Allison-specific | Use Allison documentation |
| ABS systems | May require specific request | Implement PGN request messages |
| Aftermarket ECMs | Non-standard PGN usage | Compare with stock calibration |

---

## Appendix C: Debug & Logging Strategy

### C.1 Debug Levels

```cpp
typedef enum {
    LOG_NONE = 0,     // No logging (production)
    LOG_ERROR = 1,    // Errors only
    LOG_WARN = 2,     // Warnings + errors
    LOG_INFO = 3,     // Informational + above
    LOG_DEBUG = 4,    // Debug messages + above
    LOG_VERBOSE = 5   // Everything including raw hex
} log_level_t;

#define LOG_LEVEL LOG_INFO  // Compile-time setting
```

### C.2 Module-Specific Logging

| Module | Tag | Example Messages |
|--------|-----|------------------|
| CAN Driver | `[CAN]` | Frame received, TX errors, bus status |
| J1939 Parser | `[J1939]` | PGN decoded, SPN values, TP status |
| J1708 Driver | `[J1708]` | Byte received, checksum result, MID |
| Data Manager | `[DATA]` | Parameter updates, stale detection |
| Watch List | `[WATCH]` | Value changes, threshold alerts |
| NVS Storage | `[NVS]` | Save/load operations, errors |
| Display | `[DISP]` | Render time, refresh rate |

### C.3 Serial Debug Output Format

```
[timestamp_ms][LEVEL][MODULE] Message
[12345][INFO ][J1939] PGN 61444 decoded: RPM=1234.5
[12346][DEBUG][CAN  ] RX: 0CF00400 [8] 00 00 FF 68 13 FF FF FF
[12500][WARN ][DATA ] SPN 110 stale (last update 5032ms ago)
[12501][ERROR][NVS  ] Write failed: namespace 'trip_a', key 'distance'
```

### C.4 Raw Data Logging (Verbose Mode)

```cpp
// Enable with: #define LOG_RAW_CAN 1
void log_raw_can_frame(const twai_message_t* msg) {
    if (LOG_LEVEL >= LOG_VERBOSE) {
        char buf[64];
        snprintf(buf, sizeof(buf), 
            "[%lu][VERB ][CAN  ] %s %08lX [%d] ",
            millis(),
            msg->extd ? "EXT" : "STD",
            msg->identifier,
            msg->data_length_code);
        Serial.print(buf);
        for (int i = 0; i < msg->data_length_code; i++) {
            Serial.printf("%02X ", msg->data[i]);
        }
        Serial.println();
    }
}
```

### C.5 Performance Profiling

```cpp
// Usage: PROFILE_START("J1939_parse"); ... PROFILE_END("J1939_parse");
#define PROFILE_ENABLED 1

#if PROFILE_ENABLED
#define PROFILE_START(name) uint32_t _profile_##name = micros()
#define PROFILE_END(name) \
    Serial.printf("[PROFILE] %s: %lu us\n", #name, micros() - _profile_##name)
#else
#define PROFILE_START(name)
#define PROFILE_END(name)
#endif
```

### C.6 Remote Debugging (Future WiFi)

```cpp
// UDP broadcast for wireless debugging
#define DEBUG_UDP_PORT 5555
#define DEBUG_UDP_ENABLED 0  // Enable when WiFi available

void log_to_udp(const char* msg) {
    #if DEBUG_UDP_ENABLED
    udp.beginPacket(IPAddress(255,255,255,255), DEBUG_UDP_PORT);
    udp.print(msg);
    udp.endPacket();
    #endif
}
```

### C.7 Diagnostic Commands (Serial CLI)

| Command | Description |
|---------|-------------|
| `status` | Print system status, uptime, memory |
| `can stats` | CAN bus statistics (RX/TX counts, errors) |
| `j1708 stats` | J1708 statistics |
| `params` | List all current parameter values |
| `watch` | Show watch list configuration |
| `nvs dump` | Dump NVS contents |
| `nvs reset` | Factory reset NVS |
| `log level <n>` | Set log level (0-5) |
| `reboot` | Software reset |

### C.8 LED Status Indicators

| Pattern | Meaning |
|---------|---------|
| Solid ON | System running, no errors |
| Slow blink (1 Hz) | Waiting for CAN data |
| Fast blink (4 Hz) | Active CAN/J1708 communication |
| Double blink | Warning condition |
| SOS pattern | Critical error |

---

**Document Version History:**

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 1.0 | 2026-01-22 | AI Assistant | Initial specification |
| 2.0 | 2026-01-22 | AI Assistant | Added test-driven Phase 0, watch list manager, test data sources, risk assessment, resource budgets, enhanced troubleshooting, debug strategy |

---

*This specification document is intended for use by AI agents and human developers for implementing the Custom Truck Data Logging & Display Dashboard project.*
