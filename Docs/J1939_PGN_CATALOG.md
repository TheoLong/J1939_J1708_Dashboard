# J1939 PGN/SPN Catalog Reference
## Common Heavy-Duty Vehicle Parameters

**Version:** 1.0  
**Date:** January 22, 2026

---

## Table of Contents

1. [Understanding J1939 Identifiers](#1-understanding-j1939-identifiers)
2. [Engine Parameters](#2-engine-parameters)
3. [Transmission Parameters](#3-transmission-parameters)
4. [Vehicle/Speed Parameters](#4-vehiclespeed-parameters)
5. [Fuel & Economy Parameters](#5-fuel--economy-parameters)
6. [Environmental Parameters](#6-environmental-parameters)
7. [Diagnostic Messages](#7-diagnostic-messages)
8. [Transport Protocol](#8-transport-protocol)
9. [Decoding Examples](#9-decoding-examples)

---

## 1. Understanding J1939 Identifiers

### 1.1 29-bit CAN ID Structure

```
Bit:    28    26 25 24    23         16 15          8 7           0
       ┌──────┬─────┬──────────────────┬──────────────┬──────────────┐
       │ Pri  │ R/DP│    PDU Format    │ PDU Specific │ Source Addr  │
       │(3bit)│(2b) │     (8 bits)     │   (8 bits)   │   (8 bits)   │
       └──────┴─────┴──────────────────┴──────────────┴──────────────┘
              └─────────── PGN (18 bits) ──────────────┘
```

### 1.2 PGN Calculation

**PDU1 Format (PDU Format < 240):**
- PDU Specific = Destination Address
- PGN = Reserved/DP (2 bits) + PDU Format (8 bits) + 0x00

**PDU2 Format (PDU Format ≥ 240):**
- PDU Specific = Group Extension
- PGN = Reserved/DP (2 bits) + PDU Format (8 bits) + PDU Specific (8 bits)

### 1.3 Common Source Addresses

| Address | Typical Device |
|---------|----------------|
| 0x00 | Engine #1 |
| 0x01 | Engine #2 |
| 0x03 | Transmission |
| 0x0B | Brakes - System Controller |
| 0x17 | Instrument Cluster (alternate) |
| 0x21 | Body Controller |
| 0x27 | Trailer #1 Bridge |
| 0x28 | Body Controller (alternate) |
| 0x31 | Instrument Cluster |
| 0x49 | Retarder, Engine |
| 0xF9 | Off-Board Diagnostic Tool #1 |
| 0xFA | Off-Board Diagnostic Tool #2 |
| 0xFF | Global (Broadcast) |

**Note:** Source addresses 0xF9-0xFE are conventionally reserved for diagnostic tools and external devices. Use one of these for your dashboard to avoid conflicts.

### 1.4 Special Data Values

| Value (8-bit) | Meaning |
|---------------|---------|
| 0xFE | Error/Not Available |
| 0xFF | Not Applicable |

| Value (16-bit) | Meaning |
|----------------|---------|
| 0xFE00-0xFEFF | Error Range |
| 0xFF00-0xFFFF | Not Available Range |

---

## 2. Engine Parameters

### 2.1 PGN 61444 (0xF004) - EEC1 - Electronic Engine Controller 1

**Update Rate:** 10-20 ms (Engine broadcast)

| Byte | SPN | Parameter | Bits | Scale | Offset | Unit | Range |
|------|-----|-----------|------|-------|--------|------|-------|
| 1 | 899 | Engine Torque Mode | 4 | - | - | - | 0-15 |
| 2 | 512 | Driver's Demand Engine - Percent Torque | 8 | 1 | -125 | % | -125 to 125 |
| 3 | 513 | Actual Engine - Percent Torque | 8 | 1 | -125 | % | -125 to 125 |
| 4-5 | 190 | Engine Speed | 16 | 0.125 | 0 | rpm | 0-8031.875 |
| 6 | 1483 | Source Address of Controlling Device | 8 | 1 | 0 | - | 0-255 |
| 7 | 1675 | Engine Starter Mode | 4 | - | - | - | 0-15 |
| 8 | 2432 | Engine Demand - Percent Torque | 8 | 1 | -125 | % | -125 to 125 |

**Engine Speed Decoding:**
```
Raw Value (bytes 4-5, little-endian): 0x1368 = 4968 decimal
Physical Value = 4968 × 0.125 = 621 RPM
```

### 2.2 PGN 61443 (0xF003) - EEC2 - Electronic Engine Controller 2

**Update Rate:** 50 ms

| Byte | SPN | Parameter | Bits | Scale | Offset | Unit | Range |
|------|-----|-----------|------|-------|--------|------|-------|
| 1 | 558 | Accelerator Pedal 1 Low Idle Switch | 2 | - | - | - | 0-3 |
| 1 | 559 | Accelerator Pedal Kickdown Switch | 2 | - | - | - | 0-3 |
| 1 | 1437 | Road Speed Limit Status | 2 | - | - | - | 0-3 |
| 2 | 91 | Accelerator Pedal Position 1 | 8 | 0.4 | 0 | % | 0-100 |
| 3 | 92 | Engine Percent Load At Current Speed | 8 | 1 | 0 | % | 0-125 |
| 4 | 974 | Remote Accelerator Pedal Position | 8 | 0.4 | 0 | % | 0-100 |
| 5 | 29 | Accelerator Pedal Position 2 | 8 | 0.4 | 0 | % | 0-100 |

### 2.3 PGN 65262 (0xFEEE) - ET1 - Engine Temperature 1

**Update Rate:** 1000 ms

| Byte | SPN | Parameter | Bits | Scale | Offset | Unit | Range |
|------|-----|-----------|------|-------|--------|------|-------|
| 1 | 110 | Engine Coolant Temperature | 8 | 1 | -40 | °C | -40 to 210 |
| 2 | 174 | Fuel Temperature 1 | 8 | 1 | -40 | °C | -40 to 210 |
| 3-4 | 175 | Engine Oil Temperature 1 | 16 | 0.03125 | -273 | °C | -273 to 1735 |
| 5-6 | 176 | Turbo Oil Temperature | 16 | 0.03125 | -273 | °C | -273 to 1735 |
| 7 | 52 | Engine Intercooler Temperature | 8 | 1 | -40 | °C | -40 to 210 |
| 8 | 1134 | Engine Intercooler Thermostat Opening | 8 | 0.4 | 0 | % | 0-100 |

**Coolant Temperature Decoding:**
```
Raw Value (byte 1): 0x64 = 100 decimal
Physical Value = 100 + (-40) = 60°C
```

### 2.4 PGN 65263 (0xFEEF) - EFL/P1 - Engine Fluid Level/Pressure 1

**Update Rate:** 500 ms

| Byte | SPN | Parameter | Bits | Scale | Offset | Unit | Range |
|------|-----|-----------|------|-------|--------|------|-------|
| 1 | 94 | Fuel Delivery Pressure | 8 | 4 | 0 | kPa | 0-1000 |
| 2 | 22 | Extended Crankcase Blow-by Pressure | 8 | 0.05 | -250 | kPa | -250 to -237.5 |
| 3 | 98 | Engine Oil Level | 8 | 0.4 | 0 | % | 0-100 |
| 4 | 100 | Engine Oil Pressure | 8 | 4 | 0 | kPa | 0-1000 |
| 5-6 | 101 | Crankcase Pressure | 16 | 1/128 | -250 | kPa | -250 to 251.99 |
| 7 | 109 | Coolant Pressure | 8 | 2 | 0 | kPa | 0-500 |
| 8 | 111 | Coolant Level | 8 | 0.4 | 0 | % | 0-100 |

### 2.5 PGN 65270 (0xFEF6) - IC1 - Intake/Exhaust Conditions 1

**Update Rate:** 500 ms

| Byte | SPN | Parameter | Bits | Scale | Offset | Unit | Range |
|------|-----|-----------|------|-------|--------|------|-------|
| 1 | 81 | Particulate Trap Inlet Pressure | 8 | 0.5 | 0 | kPa | 0-125 |
| 2 | 102 | Boost Pressure | 8 | 2 | 0 | kPa | 0-500 |
| 3 | 105 | Intake Manifold 1 Temperature | 8 | 1 | -40 | °C | -40 to 210 |
| 4 | 106 | Air Inlet Pressure | 8 | 2 | 0 | kPa | 0-500 |
| 5 | 107 | Air Filter 1 Differential Pressure | 8 | 0.05 | 0 | kPa | 0-12.5 |
| 6-7 | 173 | Exhaust Gas Temperature | 16 | 0.03125 | -273 | °C | -273 to 1735 |
| 8 | 112 | Coolant Filter Differential Pressure | 8 | 0.5 | 0 | kPa | 0-125 |

**Boost Pressure Decoding:**
```
Raw Value (byte 2): 0x32 = 50 decimal
Physical Value = 50 × 2 = 100 kPa
Gauge Boost = 100 kPa - 101.325 kPa ≈ -1.3 kPa (or add barometric for actual)
```

### 2.6 PGN 65271 (0xFEF7) - VEP1 - Vehicle Electrical Power 1

**Update Rate:** 1000 ms

| Byte | SPN | Parameter | Bits | Scale | Offset | Unit | Range |
|------|-----|-----------|------|-------|--------|------|-------|
| 1-2 | 114 | Net Battery Current | 16 | 1 | -125 | A | -125 to 125 |
| 3-4 | 115 | Alternator Current | 16 | 1 | 0 | A | 0-250 |
| 5-6 | 167 | Charging System Potential (Voltage) | 16 | 0.05 | 0 | V | 0-3212.75 |
| 7-8 | 168 | Battery Potential (Voltage) - Switched | 16 | 0.05 | 0 | V | 0-3212.75 |

### 2.7 PGN 65253 (0xFEE5) - HOURS - Engine Hours, Revolutions

**Update Rate:** On-request or 1000 ms

| Byte | SPN | Parameter | Bits | Scale | Offset | Unit | Range |
|------|-----|-----------|------|-------|--------|------|-------|
| 1-4 | 247 | Engine Total Hours of Operation | 32 | 0.05 | 0 | hours | 0-210,554,060.75 |
| 5-8 | 249 | Engine Total Revolutions | 32 | 1000 | 0 | r | 0-4,211,081,215,000 |

---

## 3. Transmission Parameters

### 3.1 PGN 61442 (0xF002) - ETC1 - Electronic Transmission Controller 1

**Update Rate:** 10 ms

| Byte | SPN | Parameter | Bits | Scale | Offset | Unit | Range |
|------|-----|-----------|------|-------|--------|------|-------|
| 1 | 560 | Transmission Driveline Engaged | 2 | - | - | - | 0-3 |
| 1 | 573 | Torque Converter Lockup Engaged | 2 | - | - | - | 0-3 |
| 1 | 574 | Transmission Shift In Process | 2 | - | - | - | 0-3 |
| 2 | 191 | Transmission Output Shaft Speed | 16 | 0.125 | 0 | rpm | 0-8031.875 |
| 4 | 522 | Percent Clutch Slip | 8 | 0.4 | 0 | % | 0-100 |
| 5 | 606 | Engine Momentary Overspeed Enable | 2 | - | - | - | 0-3 |
| 5 | 607 | Progressive Shift Disable | 2 | - | - | - | 0-3 |
| 6 | 161 | Transmission Input Shaft Speed | 16 | 0.125 | 0 | rpm | 0-8031.875 |

### 3.2 PGN 61445 (0xF005) - ETC2 - Electronic Transmission Controller 2

**Update Rate:** 100 ms

| Byte | SPN | Parameter | Bits | Scale | Offset | Unit | Range |
|------|-----|-----------|------|-------|--------|------|-------|
| 1 | 524 | Selected Gear | 8 | 1 | -125 | - | -125 to 125 |
| 2-3 | 526 | Actual Gear Ratio | 16 | 0.001 | 0 | - | 0-64.255 |
| 4 | 523 | Current Gear | 8 | 1 | -125 | - | -125 to 125 |

**Gear Values:**
| Value | Meaning |
|-------|---------|
| -125 | Reverse (high ratio) |
| ... | ... |
| -1 | Reverse |
| 0 | Neutral |
| 1 | First |
| 2 | Second |
| ... | ... |
| 125 | Forward (high ratio) |
| 251 | Park |

### 3.3 PGN 65272 (0xFEF8) - TRF1 - Transmission Fluids 1

**Update Rate:** 1000 ms

| Byte | SPN | Parameter | Bits | Scale | Offset | Unit | Range |
|------|-----|-----------|------|-------|--------|------|-------|
| 1 | 124 | Transmission Clutch Pressure | 8 | 16 | 0 | kPa | 0-4000 |
| 2 | 126 | Transmission Oil Level | 8 | 0.4 | 0 | % | 0-100 |
| 3 | 127 | Transmission Filter Differential Pressure | 8 | 2 | 0 | kPa | 0-500 |
| 4 | 177 | Transmission Oil Pressure | 8 | 16 | 0 | kPa | 0-4000 |
| 5-6 | 178 | Transmission Oil Temperature | 16 | 0.03125 | -273 | °C | -273 to 1735 |
| 7 | 3027 | Transmission Oil Level High/Low | 8 | 0.4 | -50 | % | -50 to 51.2 |
| 8 | 3028 | Transmission Oil Level Countdown Timer | 8 | 1 | 0 | s | 0-250 |

---

## 4. Vehicle/Speed Parameters

### 4.1 PGN 65265 (0xFEF1) - CCVS - Cruise Control/Vehicle Speed

**Update Rate:** 100 ms

| Byte | SPN | Parameter | Bits | Scale | Offset | Unit | Range |
|------|-----|-----------|------|-------|--------|------|-------|
| 1 | 69 | Two Speed Axle Switch | 2 | - | - | - | 0-3 |
| 1 | 70 | Parking Brake Switch | 2 | - | - | - | 0-3 |
| 1 | 1633 | Cruise Control Pause Switch | 2 | - | - | - | 0-3 |
| 2-3 | 84 | Wheel-Based Vehicle Speed | 16 | 1/256 | 0 | km/h | 0-250.996 |
| 4 | 595 | Cruise Control Active | 2 | - | - | - | 0-3 |
| 4 | 596 | Cruise Control Enable Switch | 2 | - | - | - | 0-3 |
| 4 | 597 | Brake Switch | 2 | - | - | - | 0-3 |
| 4 | 598 | Clutch Switch | 2 | - | - | - | 0-3 |
| 5 | 599 | Cruise Control Set Switch | 2 | - | - | - | 0-3 |
| 5 | 600 | Cruise Control Coast (Decelerate) Switch | 2 | - | - | - | 0-3 |
| 5 | 601 | Cruise Control Resume Switch | 2 | - | - | - | 0-3 |
| 5 | 602 | Cruise Control Accelerate Switch | 2 | - | - | - | 0-3 |
| 6 | 86 | Cruise Control Set Speed | 8 | 1 | 0 | km/h | 0-250 |
| 7 | 976 | PTO Governor State | 5 | - | - | - | 0-31 |
| 7 | 527 | Cruise Control States | 3 | - | - | - | 0-7 |
| 8 | 968 | Engine Idle Increment Switch | 2 | - | - | - | 0-3 |
| 8 | 967 | Engine Idle Decrement Switch | 2 | - | - | - | 0-3 |
| 8 | 966 | Engine Test Mode Switch | 2 | - | - | - | 0-3 |
| 8 | 1237 | Engine Shutdown Override Switch | 2 | - | - | - | 0-3 |

**Vehicle Speed Decoding:**
```
Raw Value (bytes 2-3, little-endian): 0x4E20 = 20000 decimal
Physical Value = 20000 / 256 = 78.125 km/h
```

### 4.2 PGN 65217 (0xFEC1) - VDHR - High Resolution Vehicle Distance

**Update Rate:** 100 ms

| Byte | SPN | Parameter | Bits | Scale | Offset | Unit | Range |
|------|-----|-----------|------|-------|--------|------|-------|
| 1-4 | 917 | High Resolution Total Vehicle Distance | 32 | 5 | 0 | m | 0-21,055,406,080 |
| 5-8 | 918 | High Resolution Trip Distance | 32 | 5 | 0 | m | 0-21,055,406,080 |

---

## 5. Fuel & Economy Parameters

### 5.1 PGN 65266 (0xFEF2) - LFE - Fuel Economy (Liquid)

**Update Rate:** 100 ms

| Byte | SPN | Parameter | Bits | Scale | Offset | Unit | Range |
|------|-----|-----------|------|-------|--------|------|-------|
| 1-2 | 183 | Fuel Rate | 16 | 0.05 | 0 | L/h | 0-3212.75 |
| 3-4 | 184 | Instantaneous Fuel Economy | 16 | 1/512 | 0 | km/L | 0-125.5 |
| 5-6 | 185 | Average Fuel Economy | 16 | 1/512 | 0 | km/L | 0-125.5 |
| 7 | 51 | Throttle Position | 8 | 0.4 | 0 | % | 0-100 |

### 5.2 PGN 65276 (0xFEFC) - DD - Dash Display

**Update Rate:** 1000 ms

| Byte | SPN | Parameter | Bits | Scale | Offset | Unit | Range |
|------|-----|-----------|------|-------|--------|------|-------|
| 1 | 80 | Washer Fluid Level | 8 | 0.4 | 0 | % | 0-100 |
| 2 | 96 | Fuel Level 1 | 8 | 0.4 | 0 | % | 0-100 |
| 3 | 95 | Engine Fuel Filter Differential Pressure | 8 | 2 | 0 | kPa | 0-500 |
| 4 | 99 | Engine Oil Filter Differential Pressure | 8 | 0.5 | 0 | kPa | 0-125 |
| 5-6 | 169 | Cargo Ambient Temperature | 16 | 0.03125 | -273 | °C | -273 to 1735 |
| 7 | 38 | Fuel Level 2 | 8 | 0.4 | 0 | % | 0-100 |

### 5.3 PGN 65257 (0xFEE9) - LFC - Fuel Consumption (Liquid)

**Update Rate:** 1000 ms

| Byte | SPN | Parameter | Bits | Scale | Offset | Unit | Range |
|------|-----|-----------|------|-------|--------|------|-------|
| 1-4 | 250 | Total Fuel Used | 32 | 0.5 | 0 | L | 0-2,105,540,607.5 |
| 5-8 | 252 | Trip Fuel | 32 | 0.5 | 0 | L | 0-2,105,540,607.5 |

---

## 6. Environmental Parameters

### 6.1 PGN 65269 (0xFEF5) - AMB - Ambient Conditions

**Update Rate:** 1000 ms

| Byte | SPN | Parameter | Bits | Scale | Offset | Unit | Range |
|------|-----|-----------|------|-------|--------|------|-------|
| 1 | 108 | Barometric Pressure | 8 | 0.5 | 0 | kPa | 0-125 |
| 2-3 | 170 | Cab Interior Temperature | 16 | 0.03125 | -273 | °C | -273 to 1735 |
| 4-5 | 171 | Ambient Air Temperature | 16 | 0.03125 | -273 | °C | -273 to 1735 |
| 6 | 172 | Air Inlet Temperature | 8 | 1 | -40 | °C | -40 to 210 |
| 7-8 | 79 | Road Surface Temperature | 16 | 0.03125 | -273 | °C | -273 to 1735 |

---

## 7. Diagnostic Messages

### 7.1 PGN 65226 (0xFECA) - DM1 - Active Diagnostic Trouble Codes

**Update Rate:** 1000 ms  
**Note:** Uses Transport Protocol if more than one DTC

| Byte | SPN | Parameter | Description |
|------|-----|-----------|-------------|
| 1 | 1213 | Malfunction Indicator Lamp Status | Protect, amber warning, red stop, MIL |
| 2 | 1213 | Flash Malfunction Indicator Lamp Status | Flash states for above |
| 3-4 | 1214 | SPN of First DTC | Bits 0-18: SPN |
| 4 | 1214 | FMI of First DTC | Bits 19-23: FMI |
| 5 | 1214 | Occurrence Count | Bits 0-6: Count, Bit 7: CM |
| ... | ... | Additional DTCs | Repeat 4-byte pattern |

**DTC Structure (4 bytes each):**
```
Byte 1-2: SPN bits 7-0 and 15-8
Byte 3:   SPN bits 18-16 (bits 7-5) + FMI (bits 4-0)
Byte 4:   SPN Conversion Method (bit 7) + Occurrence Count (bits 6-0)
```

### 7.2 PGN 65227 (0xFECB) - DM2 - Previously Active Diagnostic Trouble Codes

**Transmission:** On-request only (use PGN 59904 Request)

Same format as DM1, but contains stored/inactive DTCs.

### 7.3 PGN 59904 (0xEA00) - Request Message

**Purpose:** Request specific PGN from ECUs

| Byte | Description |
|------|-------------|
| 1 | Requested PGN (byte 0 - LSB) |
| 2 | Requested PGN (byte 1) |
| 3 | Requested PGN (byte 2 - MSB) |

**Example - Request DM1:**
```
CAN ID: 0x18EAFFA (Priority 6, DA=0xFF global, SA=0xFA tool)
Data: 0xCA 0xFE 0x00 (PGN 65226 in little-endian)
```

### 7.4 Common Failure Mode Identifiers (FMI)

| FMI | Description |
|-----|-------------|
| 0 | Data Valid But Above Normal Operational Range - Most Severe Level |
| 1 | Data Valid But Below Normal Operational Range - Most Severe Level |
| 2 | Data Erratic, Intermittent Or Incorrect |
| 3 | Voltage Above Normal, Or Shorted To High Source |
| 4 | Voltage Below Normal, Or Shorted To Low Source |
| 5 | Current Below Normal Or Open Circuit |
| 6 | Current Above Normal Or Grounded Circuit |
| 7 | Mechanical System Not Responding Or Out Of Adjustment |
| 8 | Abnormal Frequency Or Pulse Width Or Period |
| 9 | Abnormal Update Rate |
| 10 | Abnormal Rate Of Change |
| 11 | Root Cause Not Known |
| 12 | Bad Intelligent Device Or Component |
| 13 | Out Of Calibration |
| 14 | Special Instructions |
| 15 | Data Valid But Above Normal Operating Range - Least Severe |
| 16 | Data Valid But Above Normal Operating Range - Moderately Severe |
| 17 | Data Valid But Below Normal Operating Range - Least Severe |
| 18 | Data Valid But Below Normal Operating Range - Moderately Severe |
| 19 | Received Network Data In Error |
| 20-30 | Reserved for SAE assignment |
| 31 | Condition Exists |

---

## 8. Transport Protocol

### 8.1 PGN 60416 (0xEC00) - TP.CM - Transport Protocol - Connection Management

**Purpose:** Control multi-packet message transfer

#### Broadcast Announce Message (BAM)
| Byte | Description |
|------|-------------|
| 1 | Control Byte = 32 (0x20) |
| 2-3 | Total Message Size (bytes) |
| 4 | Total Number of Packets |
| 5 | Reserved (0xFF) |
| 6 | PGN byte 0 (LSB) |
| 7 | PGN byte 1 |
| 8 | PGN byte 2 (MSB) |

#### Request To Send (RTS)
| Byte | Description |
|------|-------------|
| 1 | Control Byte = 16 (0x10) |
| 2-3 | Total Message Size (bytes) |
| 4 | Total Number of Packets |
| 5 | Max Packets per CTS |
| 6 | PGN byte 0 (LSB) |
| 7 | PGN byte 1 |
| 8 | PGN byte 2 (MSB) |

#### Clear To Send (CTS)
| Byte | Description |
|------|-------------|
| 1 | Control Byte = 17 (0x11) |
| 2 | Number of Packets To Send |
| 3 | Next Packet Number Expected |
| 4-5 | Reserved (0xFF) |
| 6 | PGN byte 0 (LSB) |
| 7 | PGN byte 1 |
| 8 | PGN byte 2 (MSB) |

### 8.2 PGN 60160 (0xEB00) - TP.DT - Transport Protocol - Data Transfer

| Byte | Description |
|------|-------------|
| 1 | Sequence Number (1-255) |
| 2-8 | Data Bytes (7 bytes per packet) |

---

## 9. Decoding Examples

### 9.1 Example: Engine Speed from EEC1

**Received CAN Frame:**
```
ID: 0x0CF00400
Data: FF FF FF 68 13 FF FF FF
```

**Decoding Steps:**

1. **Extract PGN from ID:**
   ```
   29-bit ID: 0x0CF00400
   Binary: 0000 1100 1111 0000 0000 0100 0000 0000
   
   Priority: 011 = 3
   Reserved/DP: 00
   PDU Format: 11110000 = 0xF0 = 240
   PDU Specific: 00000100 = 0x04
   Source Address: 00000000 = 0x00 (Engine)
   
   PGN = 0x00F004 = 61444 (EEC1)
   ```

2. **Extract Engine Speed (SPN 190):**
   ```
   Bytes 4-5 (0-indexed: bytes 3-4): 0x68, 0x13
   Little-endian: 0x1368 = 4968 decimal
   
   Scale: 0.125 RPM/bit
   Offset: 0
   
   Engine Speed = 4968 × 0.125 = 621 RPM
   ```

### 9.2 Example: Coolant Temperature from ET1

**Received CAN Frame:**
```
ID: 0x18FEEE00
Data: 64 FF FF FF FF FF FF FF
```

**Decoding Steps:**

1. **Verify PGN:**
   ```
   PGN = 0xFEEE = 65262 (ET1)
   Source Address = 0x00 (Engine)
   ```

2. **Extract Coolant Temperature (SPN 110):**
   ```
   Byte 1 (0-indexed: byte 0): 0x64 = 100 decimal
   
   Scale: 1 °C/bit
   Offset: -40 °C
   
   Coolant Temperature = 100 + (-40) = 60°C
   ```

### 9.3 Example: DM1 Multi-packet (via BAM)

**BAM Announcement:**
```
ID: 0x18ECFF00
Data: 20 27 00 06 FF CA FE 00
       │  │     │     └─────── PGN 65226 (0xFECA) = DM1
       │  │     └───────────── 6 packets
       │  └─────────────────── 39 bytes total
       └────────────────────── BAM control byte
```

**Data Transfer Packets:**
```
ID: 0x18EBFF00, Data: 01 00 00 00 00 00 00 00  (packet 1)
ID: 0x18EBFF00, Data: 02 XX XX XX XX XX XX XX  (packet 2)
...
ID: 0x18EBFF00, Data: 06 XX XX XX XX FF FF FF  (packet 6)
```

**Reassembly:**
Concatenate bytes 2-8 from each packet to form complete 39-byte DM1 message.

---

## Quick Reference Card

### Common PGNs at a Glance

| PGN | Hex | Name | Key Parameters |
|-----|-----|------|----------------|
| 61444 | 0xF004 | EEC1 | Engine Speed, Torque |
| 61443 | 0xF003 | EEC2 | Throttle, Load% |
| 61442 | 0xF002 | ETC1 | Trans Output Speed |
| 65262 | 0xFEEE | ET1 | Coolant Temp, Oil Temp |
| 65263 | 0xFEEF | EFL/P1 | Oil Pressure, Level |
| 65265 | 0xFEF1 | CCVS | Vehicle Speed, Cruise |
| 65266 | 0xFEF2 | LFE | Fuel Rate, Economy |
| 65270 | 0xFEF6 | IC1 | Boost, Intake Temp, EGT |
| 65271 | 0xFEF7 | VEP1 | Battery Voltage |
| 65272 | 0xFEF8 | TRF1 | Trans Temp, Pressure |
| 65269 | 0xFEF5 | AMB | Ambient Temp, Baro |
| 65226 | 0xFECA | DM1 | Active DTCs |
| 59904 | 0xEA00 | Request | Request PGN |
| 60416 | 0xEC00 | TP.CM | Transport Control |
| 60160 | 0xEB00 | TP.DT | Transport Data |

---

**Document Version History:**

| Version | Date | Changes |
|---------|------|---------|
| 1.0 | 2026-01-22 | Initial catalog |
