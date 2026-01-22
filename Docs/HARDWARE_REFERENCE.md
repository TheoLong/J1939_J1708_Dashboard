# Hardware Reference Guide
## J1939/J1708 Dashboard Project

**Version:** 1.0  
**Date:** January 22, 2026

---

## Table of Contents

1. [ESP32 DevKit V1](#1-esp32-devkit-v1)
2. [Waveshare SN65HVD230 CAN Board](#2-waveshare-sn65hvd230-can-board)
3. [ATNSINC TTL-RS485 Adapter](#3-atnsinc-ttl-rs485-adapter)
4. [Wiring & Connections](#4-wiring--connections)
5. [Vehicle Interface](#5-vehicle-interface)

---

## 1. ESP32 DevKit V1

### 1.1 Overview

The ESP32 DevKit V1 (DOIT variant) is a development board based on the ESP32-WROOM-32 module. It provides an excellent platform for this project due to its integrated CAN controller (TWAI), multiple UARTs, ADC channels, and WiFi/Bluetooth connectivity.

### 1.2 Key Specifications

| Parameter | Value |
|-----------|-------|
| Module | ESP32-WROOM-32 |
| CPU | Dual-core Xtensa LX6, 240 MHz |
| Flash Memory | 4 MB |
| SRAM | 520 KB |
| GPIO Pins | 34 programmable |
| ADC | 18 channels, 12-bit resolution |
| DAC | 2 channels, 8-bit |
| UART | 3 interfaces |
| SPI | 4 interfaces |
| I2C | 2 interfaces |
| CAN (TWAI) | 1 controller |
| Wi-Fi | 802.11 b/g/n, 2.4 GHz |
| Bluetooth | v4.2 BR/EDR + BLE |
| Operating Voltage | 3.3V |
| Input Voltage | 5V (via USB or VIN pin) |
| Flash Mode | QIO, 40 MHz |

### 1.3 Pin Categories

#### Input-Only Pins (No Internal Pull-up/Pull-down)
These pins can only be used as inputs:
- **GPIO 34** - ADC1_CH6
- **GPIO 35** - ADC1_CH7
- **GPIO 36** (VP) - ADC1_CH0
- **GPIO 39** (VN) - ADC1_CH3

#### SPI Flash Pins (DO NOT USE)
These are connected to internal flash:
- GPIO 6, 7, 8, 9, 10, 11

#### Strapping Pins (Use with Caution)
These affect boot behavior:
- **GPIO 0** - Must be LOW for flash mode
- **GPIO 2** - Must be LOW or floating for flash
- **GPIO 5** - Must be HIGH during boot (CAN TX idles high - compatible)
- **GPIO 12** - Must be LOW during boot
- **GPIO 15** - Must be HIGH during boot

**Note:** GPIO 5 is used for CAN_TX in this project. This is safe because the SN65HVD230 transceiver keeps the line in the correct state during boot.

#### ADC1 Channels (WiFi-Safe)
These work correctly when WiFi is enabled:
- GPIO 32 (ADC1_CH4)
- GPIO 33 (ADC1_CH5)
- GPIO 34 (ADC1_CH6)
- GPIO 35 (ADC1_CH7)
- GPIO 36 (ADC1_CH0)
- GPIO 37 (ADC1_CH1) - not exposed on DevKit
- GPIO 38 (ADC1_CH2) - not exposed on DevKit
- GPIO 39 (ADC1_CH3)

#### ADC2 Channels (DO NOT USE with WiFi)
These conflict with WiFi radio:
- GPIO 0, 2, 4, 12, 13, 14, 15, 25, 26, 27

### 1.4 TWAI (CAN) Controller

The ESP32 includes a built-in CAN controller called TWAI (Two-Wire Automotive Interface). It supports:

- CAN 2.0A (11-bit identifiers)
- CAN 2.0B (29-bit extended identifiers) ✓ Required for J1939
- Bit rates up to 1 Mbps
- Acceptance filtering
- Error detection and recovery

**Default TWAI Pins (configurable):**
| Function | Default GPIO |
|----------|--------------|
| CAN TX | GPIO 5 |
| CAN RX | GPIO 4 |

### 1.5 UART Interfaces

| UART | TX Pin | RX Pin | Notes |
|------|--------|--------|-------|
| UART0 | GPIO 1 | GPIO 3 | Used for programming/debug |
| UART1 | GPIO 10 | GPIO 9 | Connected to flash - reassign |
| UART2 | GPIO 17 | GPIO 16 | Free for use (J1708) |

### 1.6 Recommended Pin Assignment for This Project

| Function | GPIO | Notes |
|----------|------|-------|
| **CAN Bus** |||
| CAN_TX | 5 | To SN65HVD230 CTX |
| CAN_RX | 4 | To SN65HVD230 CRX |
| **J1708** |||
| UART2_TX | 17 | To RS485 DI |
| UART2_RX | 16 | To RS485 RO |
| RS485_DE | 25 | Direction control |
| **Analog Inputs** |||
| Fuel1 | 36 | ADC1_CH0, input only |
| Fuel2 | 39 | ADC1_CH3, input only |
| Dimmer | 35 | ADC1_CH7, with divider |
| Vbat | 32 | ADC1_CH4, with divider |
| **Display (SPI)** |||
| SPI_CLK | 18 | VSPI_SCK |
| SPI_MOSI | 23 | VSPI_MOSI |
| SPI_MISO | 19 | VSPI_MISO |
| TFT_CS | 15 | Display chip select |
| TFT_DC | 22 | Data/Command (GPIO 2 avoided - boot strapping) |
| TFT_RST | 21 | Display reset |
| **Status** |||
| LED | 2 | Built-in LED (boot strapping pin - use carefully) |
| **Future** |||
| 1-Wire | 27 | Temperature sensors |
| TC_CS | 14 | Thermocouple chip select |
| Relay | 26 | Remote start relay |

---

## 2. Waveshare SN65HVD230 CAN Board

### 2.1 Overview

The Waveshare SN65HVD230 CAN Board is a 3.3V CAN transceiver module based on the Texas Instruments SN65HVD230 chip. It converts the ESP32's TTL-level CAN signals to differential CAN bus signals.

### 2.2 Key Specifications

| Parameter | Value |
|-----------|-------|
| Transceiver IC | TI SN65HVD230 |
| Operating Voltage | 3.3V |
| I/O Voltage | 3.3V (ESP32 compatible) |
| Max Bit Rate | 1 Mbps |
| Bus Pins | CANH, CANL |
| ESD Protection | Yes |
| Standby Mode | Available via Rs pin |
| Slope Control | Available |

### 2.3 Pinout

```
    ┌───────────────────────────┐
    │    SN65HVD230 CAN Board   │
    │                           │
    │   ┌───┐  ┌───┐  ┌───┐    │
    │   │3V3│  │GND│  │CTX│    │  ← MCU Side
    │   └─┬─┘  └─┬─┘  └─┬─┘    │
    │     │      │      │      │
    │   ┌───┐  ┌───┐            │
    │   │CRX│  │ S │            │  ← MCU Side
    │   └─┬─┘  └─┬─┘            │
    │     │      │              │
    │   ┌────┐  ┌────┐          │
    │   │CANH│  │CANL│          │  ← Bus Side
    │   └──┬─┘  └──┬─┘          │
    └──────┼───────┼────────────┘
           │       │
           ▼       ▼
        To CAN Bus (Truck)
```

| Pin | Name | Function | Connect To |
|-----|------|----------|------------|
| 1 | 3V3 | Power input (3.3V) | ESP32 3.3V |
| 2 | GND | Ground | ESP32 GND |
| 3 | CTX | CAN Transmit (TTL input) | ESP32 GPIO 5 |
| 4 | CRX | CAN Receive (TTL output) | ESP32 GPIO 4 |
| 5 | S | Slope control (optional) | Leave floating or tie to GND |
| 6 | CANH | CAN High (differential) | Truck Pin C |
| 7 | CANL | CAN Low (differential) | Truck Pin D |

### 2.4 Important Notes

1. **No Termination Resistor** - The SN65HVD230 board does NOT include a built-in termination resistor. The truck's J1939 bus should already be terminated at both ends (120Ω each). Do NOT add termination.

2. **3.3V Only** - This board operates at 3.3V, which is perfect for direct ESP32 connection. Do not apply 5V.

3. **ESD Protection** - The board includes ESD protection, but additional TVS diodes on CANH/CANL are recommended for harsh automotive environments.

4. **Slope Control** - The 'S' pin can be left floating for normal operation or tied to GND for high-speed mode.

### 2.5 Wiring to ESP32

```
ESP32 DevKit V1           SN65HVD230 Board
    3.3V  ─────────────────► 3V3
    GND   ─────────────────► GND
    GPIO5 ─────────────────► CTX
    GPIO4 ─────────────────► CRX
                             CANH ──────► Truck Pin C
                             CANL ──────► Truck Pin D
```

---

## 3. ATNSINC TTL-RS485 Adapter

### 3.1 Overview

The ATNSINC TTL-RS485 adapter module is based on the MAX485 (or compatible SP485/SN75176) transceiver chip. It converts TTL UART signals to RS-485 differential signals, which is required for J1708 communication.

### 3.2 Key Specifications

| Parameter | Value |
|-----------|-------|
| Transceiver IC | MAX485 / SP485 compatible |
| Operating Voltage | 3.3V or 5V |
| Data Rate | Up to 2.5 Mbps |
| Mode | Half-duplex |
| Number of Nodes | Up to 32 on bus |
| Interface | TTL UART + DE/RE control |

### 3.3 Pinout

```
    ┌─────────────────────────────┐
    │   TTL-RS485 Adapter Module  │
    │                             │
    │   VCC  GND  DI  RO  DE  RE  │  ← MCU Side
    │    │    │   │   │   │   │   │
    │   ┌┴┐  ┌┴┐ ┌┴┐ ┌┴┐ ┌┴┐ ┌┴┐  │
    │   │●│  │●│ │●│ │●│ │●│ │●│  │
    │   └─┘  └─┘ └─┘ └─┘ └─┘ └─┘  │
    │                             │
    │          A      B           │  ← Bus Side
    │         ┌┴┐    ┌┴┐          │
    │         │●│    │●│          │
    │         └─┘    └─┘          │
    └──────────┼──────┼───────────┘
               │      │
               ▼      ▼
          To J1708 Bus (Truck)
```

| Pin | Name | Function | Connect To |
|-----|------|----------|------------|
| VCC | Power | 3.3V or 5V input | ESP32 3.3V |
| GND | Ground | Common ground | ESP32 GND |
| DI | Driver Input | TTL TX data input | ESP32 GPIO 17 |
| RO | Receiver Output | TTL RX data output | ESP32 GPIO 16 |
| DE | Driver Enable | HIGH = Transmit mode | ESP32 GPIO 25 |
| RE | Receiver Enable | LOW = Receive mode (inverted) | Tie to DE or GND |
| A | Non-inverting | RS-485 + (J1708+) | Truck Pin F |
| B | Inverting | RS-485 - (J1708-) | Truck Pin G |

**Important Note on A/B Polarity:** RS-485 A/B labeling varies by chip manufacturer. If J1708 communication doesn't work, try swapping the A and B connections. Use an oscilloscope to verify: J1708+ should idle approximately 1.2V higher than J1708-.

### 3.4 Direction Control

The MAX485 is a half-duplex device. The DE and RE pins control the direction:

| DE | RE | Mode |
|----|-----|------|
| LOW | LOW | Receive (listen) |
| HIGH | HIGH | Transmit (send) |
| LOW | HIGH | High impedance |
| HIGH | LOW | Not used |

**For this project (monitoring only):**
- Tie DE and RE both to GND for permanent receive mode
- OR control with GPIO for bi-directional communication

### 3.5 Wiring Options

#### Option A: Read-Only Mode (Recommended for Initial Development)

```
ESP32 DevKit V1           RS485 Module
    3.3V  ─────────────────► VCC
    GND   ─────────────────► GND
    GPIO16 ◄─────────────────  RO
    GPIO17 ─────────────────► DI (not used, but connected)
    GND   ─────────────────► DE (permanent receive)
    GND   ─────────────────► RE (permanent receive)
                              A  ──────► Truck Pin F (J1708+)
                              B  ──────► Truck Pin G (J1708-)
```

#### Option B: Bi-Directional Mode (For Future Use)

```
ESP32 DevKit V1           RS485 Module
    3.3V  ─────────────────► VCC
    GND   ─────────────────► GND
    GPIO16 ◄─────────────────  RO
    GPIO17 ─────────────────► DI
    GPIO25 ─────────────────► DE ─┐
                              RE ◄┘ (tied together)
                              A  ──────► Truck Pin F (J1708+)
                              B  ──────► Truck Pin G (J1708-)
```

Control logic in code:
```cpp
#define RS485_DE_PIN 25

void j1708_set_receive_mode() {
    digitalWrite(RS485_DE_PIN, LOW);
}

void j1708_set_transmit_mode() {
    digitalWrite(RS485_DE_PIN, HIGH);
}
```

### 3.6 Voltage Level Considerations

The ESP32 operates at 3.3V logic levels. Most MAX485 modules are designed for 5V but will work at 3.3V with reduced noise margin. For best results:

1. Power the module with 3.3V (not 5V) for direct compatibility
2. OR use a 3.3V variant (MAX3485 or similar)
3. If using 5V module, add level shifters or resistor dividers on RO

---

## 4. Wiring & Connections

### 4.1 Complete Wiring Diagram

```
                                    ┌─────────────────────────────────────┐
                                    │         9-Pin Deutsch               │
                                    │     Diagnostic Connector            │
                                    │                                     │
                                    │   A = GND (Black)                   │
                                    │   B = +12/24V Battery (Red)         │
                                    │   C = J1939 CAN High (Yellow)       │
                                    │   D = J1939 CAN Low (Green)         │
                                    │   F = J1708+ (Orange)               │
                                    │   G = J1708- (Blue)                 │
                                    └─────────┬───────────────────────────┘
                                              │
                    ┌─────────────────────────┼─────────────────────────┐
                    │                         │                         │
              ┌─────┴─────┐            ┌──────┴──────┐            ┌─────┴─────┐
              │   Pin A   │            │  Pin C/D    │            │  Pin F/G  │
              │   (GND)   │            │ (CAN H/L)   │            │ (J1708+/-)│
              └─────┬─────┘            └──────┬──────┘            └─────┬─────┘
                    │                         │                         │
                    │                   ┌─────┴─────┐             ┌─────┴─────┐
                    │                   │ SN65HVD230│             │  RS485    │
                    │                   │ CAN Board │             │  Adapter  │
                    │                   │           │             │           │
                    │                   │  CANH─────│─────────────│─► A       │
                    │                   │  CANL─────│─────────────│─► B       │
                    │                   │           │             │           │
                    │    ┌──────────────│──CTX      │             │  DI ◄─────│
                    │    │   ┌──────────│──CRX      │             │  RO ──────│
                    │    │   │   ┌──────│──3V3      │             │  DE ◄─────│
                    │    │   │   │   ┌──│──GND      │             │  RE ◄─────│
                    │    │   │   │   │  └───────────┘             │  VCC ◄────│
                    │    │   │   │   │                            │  GND ◄────│
                    │    │   │   │   │                            └───────────┘
                    │    │   │   │   │                                  │
                    │    │   │   │   │                                  │
                    ▼    ▼   ▼   ▼   ▼                                  ▼
              ┌──────────────────────────────────────────────────────────────┐
              │                      ESP32 DevKit V1                          │
              │                                                               │
              │  GND ◄───────────────────────────────────────────────────────│
              │  3.3V ───────────────────────────────────────────────────────│
              │  GPIO 5 (CAN_TX) ───────────────────────────────────────────►│
              │  GPIO 4 (CAN_RX) ◄──────────────────────────────────────────│
              │  GPIO 17 (UART2_TX) ────────────────────────────────────────►│
              │  GPIO 16 (UART2_RX) ◄───────────────────────────────────────│
              │  GPIO 25 (RS485_DE) ────────────────────────────────────────►│
              │                                                               │
              │  USB ────────► Computer (for programming/debug)               │
              │                                                               │
              └───────────────────────────────────────────────────────────────┘
```

### 4.2 Connection Table

| ESP32 Pin | Function | Connects To | Wire Color (Suggested) |
|-----------|----------|-------------|------------------------|
| 3.3V | Power | SN65HVD230 3V3, RS485 VCC | Red |
| GND | Ground | All GND pins, Truck Pin A | Black |
| GPIO 5 | CAN_TX | SN65HVD230 CTX | Orange |
| GPIO 4 | CAN_RX | SN65HVD230 CRX | Yellow |
| GPIO 17 | UART2_TX | RS485 DI | Blue |
| GPIO 16 | UART2_RX | RS485 RO | Green |
| GPIO 25 | RS485_DE | RS485 DE + RE | White |

### 4.3 Power Supply Considerations

#### Development/Bench Testing
- Power ESP32 via USB from computer
- 3.3V output supplies both transceivers
- Total current: ~200mA typical

#### Vehicle Installation
For permanent installation, use a DC-DC converter:

```
Vehicle 12/24V ──► DC-DC Buck Converter ──► 5V ──► ESP32 VIN
                   (e.g., LM2596)                    │
                                                     ▼
                                              3.3V Output
                                                     │
                                        ┌────────────┼────────────┐
                                        │            │            │
                                   SN65HVD230    RS485 Mod    Other
```

**Recommended DC-DC Specifications:**
| Parameter | Value |
|-----------|-------|
| Input Voltage | 8-36V (covers 12V and 24V systems) |
| Output Voltage | 5V |
| Output Current | ≥ 1A |
| Features | Over-voltage, short-circuit protection |
| Example Parts | LM2596, MP1584, XL4015 |

### 4.4 EMI/Noise Considerations

1. **Keep wires short** - Minimize wire length between ESP32 and transceivers
2. **Twist pairs** - Twist CAN and J1708 wire pairs
3. **Add filtering** - Consider 100nF capacitors near transceiver power pins
4. **Shielding** - Use shielded cable for vehicle runs > 1 meter
5. **TVS diodes** - Add automotive TVS diodes on bus lines for ESD protection

---

## 5. Vehicle Interface

### 5.1 9-Pin Deutsch Diagnostic Connector

The standard SAE J1939/J1708 diagnostic connector is a 9-pin Deutsch HD10 series:

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
         
         (Front view - socket side)
```

### 5.2 Pin Functions

| Pin | Name | Function | Wire Color (Common) |
|-----|------|----------|---------------------|
| A | Ground | Signal/power return | Black |
| B | Battery+ | Unswitched +12/24V | Red |
| C | CAN H | J1939 CAN High | Yellow |
| D | CAN L | J1939 CAN Low | Green |
| E | Shield | CAN shield (optional) | Bare/Drain |
| F | J1708+ | J1587 positive | Orange |
| G | J1708- | J1587 negative | Blue/Gray |
| H | CAN 2 H | Secondary CAN High (if present) | Yellow/White |
| J | CAN 2 L | Secondary CAN Low (if present) | Green/White |

### 5.3 Connector Options

#### Option 1: Y-Splitter Cable
Use a Y-adapter to share the connector with existing diagnostic tools (Nexiq, etc.):
- One branch to your device
- One branch to existing tool
- Allows concurrent use

#### Option 2: Direct Connection
Create a dedicated cable with appropriate pins:
- Use Deutsch HD10-9-1939P male connector
- Wire only needed pins (A, C, D, F, G minimum)

#### Option 3: Hardwire
Tap directly into vehicle wiring:
- Locate CAN bus wires (typically twisted yellow/green)
- Locate J1708 wires (typically twisted orange/blue)
- Use T-tap or solder connections
- More permanent but less portable

### 5.4 Pre-Connection Verification

Before connecting your device to the vehicle:

1. **Measure Voltages:**
   - Pin B to Pin A: Should read +12V or +24V (battery voltage)
   - Pin C to Pin D: Should read < 0.5V (CAN differential idle)
   - Pin F to Pin G: Should read < 0.5V (J1708 differential idle)

2. **Check CAN Termination:**
   - With ignition OFF, measure resistance between Pin C and Pin D
   - Should read approximately 60Ω (two 120Ω terminators in parallel)
   - If >100Ω, bus may not be properly terminated

3. **Verify Ground:**
   - Pin A should be connected to vehicle chassis ground
   - Verify continuity to known chassis ground point

### 5.5 Installation Location

For permanent installation, consider:

1. **Under dashboard** - Protected, accessible, close to diagnostic port
2. **Behind panel** - Hidden but accessible for service
3. **Sealed enclosure** - For environments with dust/moisture

**Requirements:**
- Adequate ventilation for heat dissipation
- Protection from vibration
- Cable routing that avoids high-current wires, ignition components

---

## Document History

| Version | Date | Changes |
|---------|------|---------|
| 1.0 | 2026-01-22 | Initial release |
