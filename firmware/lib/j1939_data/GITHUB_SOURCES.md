# J1939/J1708 GitHub Resources

This document catalogs open-source J1939 and J1708 resources available on GitHub for reference, testing, and integration.

---

## 1. Complete J1939 Protocol Stacks

### DanielMartensson/Open-SAE-J1939
**URL:** https://github.com/DanielMartensson/Open-SAE-J1939

A comprehensive open-source J1939 stack implementation in C. This is the primary reference for our implementation.

**Key Files:**
- `Src/SAE_J1939/SAE_J1939_Enums/Enum_PGN.h` - Complete PGN definitions
- `Src/SAE_J1939/SAE_J1939_Enums/Enum_DM1_DM2.h` - 3000+ SPN definitions with FMI codes
- `Src/SAE_J1939/SAE_J1939_Enums/Enum_Control_Byte.h` - Transport Protocol control bytes
- `Src/Open_SAE_J1939/Structs.h` - Data structures for TP_CM, TP_DT, Name, Proprietary

**Features:**
- Transport Protocol (TP.CM, TP.DT) support
- Address Claimed (J1939-81)
- DM1/DM2 Diagnostic Messages
- Proprietary A and B messages
- ISO 11783 Auxiliary Valve support

**Key PGN Definitions Extracted:**
```c
PGN_REQUEST = 0x00EA00
PGN_TP_CM = 0x00EC00
PGN_TP_DT = 0x00EB00
PGN_ADDRESS_CLAIMED = 0x00EE00
PGN_DM1 = 0x00FECA
PGN_DM2 = 0x00FECB
PGN_SOFTWARE_IDENTIFICATION = 0x00FEDA
PGN_ECU_IDENTIFICATION = 0x00FDC5
PGN_COMPONENT_IDENTIFICATION = 0x00FEEB
PGN_ELECTRONIC_ENGINE_CONTROLLER_2_61443 = 0x00F003
PGN_ENGINE_FLUIDS_LEVEL_PRESSURE_1_65263 = 0x00FEEF
PGN_AMBIENT_CONDITIONS_65269 = 0x00FEF5
PGN_DASH_DISPLAY_65276 = 0x00FEFC
```

---

## 2. Python CAN Libraries

### hardbyte/python-can
**URL:** https://github.com/hardbyte/python-can

The de-facto Python CAN library with multiple interface support.

**Key Features for J1939:**
- ASC log file reader/writer (skips J1939TP messages)
- TRC file format support
- CAN log utilities compatible with candump
- CSV format support
- BLF (Binary Log Format) reader

**Log File Formats Supported:**
- `.asc` - Vector ASCII format
- `.blf` - Vector Binary Log Format
- `.trc` - PEAK TRC format
- `.csv` - Generic CSV
- `.log` - candump format

**Usage Example:**
```python
import can
from can.io import ASCReader, TRCReader

# Read ASC log
with ASCReader('truck_log.asc') as reader:
    for msg in reader:
        if msg.is_extended_id and (msg.arbitration_id >> 16) & 0xFF >= 0xF0:
            # This is likely a J1939 message
            pgn = (msg.arbitration_id >> 8) & 0xFFFF
            print(f"PGN: {pgn:04X}")
```

**Related J1939 Tools (from docs):**
- `can-j1939` - SAE J1939 protocol implementation for python-can
- `cantools` - CAN message database tools
- `pretty_j1939` - Post-process J1939 logs to human readable format

---

## 3. CAN Database Tools

### ebroecker/canmatrix
**URL:** https://github.com/ebroecker/canmatrix

Comprehensive CAN database format conversion library with J1939 support.

**Key Files:**
- `src/canmatrix/ArbitrationId.py` - J1939 ID parsing (priority, PGN, source, etc.)
- `src/canmatrix/j1939_decoder.py` - J1939 transport protocol decoder
- `src/canmatrix/formats/dbc.py` - DBC file support with VFrameFormat for J1939
- `examples/createJ1939Dbc.py` - Example J1939 DBC creation

**J1939 ArbitrationId Properties:**
```python
@property
def j1939_pgn(self):
    # Extract PGN from 29-bit CAN ID
    
@property
def j1939_source(self):
    return self.id & 0xFF

@property  
def j1939_priority(self):
    return (self.id >> 26) & 0x7

@property
def j1939_pf(self):
    return (self.id >> 16) & 0xFF

@property
def j1939_ps(self):
    return (self.id >> 8) & 0xFF
```

**Supported Formats:**
- DBC (Vector CANdb++)
- DBF (BUSMASTER)
- ARXML (AUTOSAR)
- KCD (Kayak CAN Database)
- SYM (Symbol Editor)
- XLSX (Excel)

---

## 4. J1939 DBC Files

### Built-in J1939 Database (canmatrix)
**Location:** `src/canmatrix/j1939.dbc`

Contains standard J1939 message definitions for common PGNs like:
- EEC1 (Electronic Engine Controller 1)
- ETC1 (Electronic Transmission Controller 1)
- ETC7
- TC1

---

## 5. CAN Log Data Repositories

### CSS-Electronics Sample Data
**URL:** Various sample files available through their tools

Provides:
- Heavy-duty truck CAN logs
- J1939 message samples
- DBC file examples

### commaai/opendbc (Limited J1939)
**URL:** https://github.com/commaai/opendbc

Primarily automotive (passenger vehicle) DBC files, but useful for:
- CAN message structure examples
- DBC syntax reference
- Signal definition patterns

---

## 6. Test Data Generation

### Creating Test Data from Our Definitions

Using our `j1939_pgn_definitions.h`, test data can be generated:

```python
import struct

# EEC1 (PGN 61444) Test Data
# Engine Speed = 2000 RPM (0x3E80 at 0.125 RPM/bit)
# Actual Torque = 50% (0x7D after offset -125)
eec1_data = bytes([
    0x00,        # Engine Torque Mode
    0x7D,        # Driver's Demand Torque (50%)
    0x7D,        # Actual Engine Torque (50%)  
    0x80, 0x3E,  # Engine Speed (2000 RPM)
    0x00,        # Source Address
    0x00, 0x00   # Reserved
])

# Build CAN ID: Priority 3, PGN 61444, SA 0x00
can_id = (3 << 26) | (61444 << 8) | 0x00  # 0x0CF00400
```

---

## 7. SAE J1708/J1587 Resources

### Legacy Protocol Support
Limited open-source support exists for J1708/J1587. Key resources:

- Our `j1708_j1587_definitions.h` contains MID/PID definitions
- Bendix/Meritor ABS systems still use J1708 for diagnostics
- RS-485 physical layer at 9600 bps

---

## 8. Tools and Utilities

### pretty_j1939
**URL:** Referenced in python-can documentation

Post-processor for J1939 CAN logs that converts to human-readable output.

### can-isotp
**URL:** https://can-isotp.readthedocs.io/

ISO 15765-2 (ISO-TP) implementation - similar multi-frame transport protocol used in UDS/OBD-II, useful for understanding J1939 TP concepts.

---

## 9. Commercial References (for validation)

These are mentioned for specification validation only:

- **SAE J1939-71** - Vehicle Application Layer (PGN/SPN definitions)
- **SAE J1939-21** - Data Link Layer
- **SAE J1939-81** - Network Management (Address Claiming)
- **SAE J1587** - Joint SAE/TMC Electronic Data Interchange
- **SAE J1708** - Serial Communication Between Devices

---

## 10. Integration Plan

### Phase 1: Core Parsing
Use definitions from Open-SAE-J1939 and our header files for basic PGN/SPN decoding.

### Phase 2: Log File Support  
Integrate python-can log readers for ASC/BLF/TRC test data import.

### Phase 3: DBC Integration
Use canmatrix for DBC file loading/saving to allow custom message definitions.

### Phase 4: Transport Protocol
Implement full TP.CM/TP.DT support based on Open-SAE-J1939 reference.

---

## Quick Reference: Key GitHub URLs

| Repository | Purpose | Stars |
|------------|---------|-------|
| [DanielMartensson/Open-SAE-J1939](https://github.com/DanielMartensson/Open-SAE-J1939) | Complete C implementation | 500+ |
| [hardbyte/python-can](https://github.com/hardbyte/python-can) | Python CAN library | 1500+ |
| [ebroecker/canmatrix](https://github.com/ebroecker/canmatrix) | Database conversion | 800+ |
| [CSS-Electronics](https://github.com/CSS-Electronics) | CAN tools and samples | - |

---

## Sample Data Files to Download

For testing the parser, download these sample files:

1. **ASC Format:**
   - Generate with: `python -m can.logger -f truck_test.asc`
   
2. **candump Format:**
   - Generate with: `candump -L can0 > truck_test.log`

3. **Create Synthetic Test Data:**
   ```bash
   # Using cangen (from can-utils)
   cangen vcan0 -I 18FEF100 -L 8 -D FFFFFFFFFFFF0000 -g 100
   ```

---

## License Notes

- **Open-SAE-J1939**: MIT License - free for commercial use
- **python-can**: LGPL v3 - careful with static linking
- **canmatrix**: BSD-3-Clause - permissive
- **Our headers**: Project-specific license

---

*Last Updated: January 2026*
*For: J1939_J1708_Dashboard Project*
