# J1939/J1708 Protocol Data Library

This directory contains protocol specification data for heavy-duty vehicle communication parsing.

## Files

### j1939_pgn_definitions.h
Complete J1939 (SAE J1939-71) PGN/SPN catalog for CAN-based heavy-duty vehicle communication.

**Contents:**
- Protocol constants (baud rate, addressing)
- PGN (Parameter Group Number) definitions with:
  - PGN numbers and names
  - Transmission rates
  - Data lengths
  - SPN lists
- SPN (Suspect Parameter Number) definitions with:
  - Byte/bit positions
  - Scale and offset values
  - Engineering units
  - Min/max ranges
- FMI (Failure Mode Identifier) codes
- Helper functions for PGN extraction and CAN ID building

**Key PGNs Included:**
| PGN | Acronym | Name | Rate |
|-----|---------|------|------|
| 61444 | EEC1 | Electronic Engine Controller 1 | 10ms |
| 61443 | EEC2 | Electronic Engine Controller 2 | 50ms |
| 65262 | ET1 | Engine Temperature 1 | 1s |
| 65263 | EFLP1 | Engine Fluid Level/Pressure 1 | 500ms |
| 65265 | CCVS | Cruise Control/Vehicle Speed | 100ms |
| 65270 | IC1 | Inlet/Exhaust Conditions 1 | 500ms |
| 65271 | VEP1 | Vehicle Electrical Power 1 | 1s |
| 65272 | TRF1 | Transmission Fluids 1 | 1s |
| 61445 | ETC2 | Electronic Transmission Controller 2 | 100ms |
| 65266 | LFE | Liquid Fuel Economy | 100ms |
| 65276 | DD | Dash Display | 1s |
| 65269 | AMB | Ambient Conditions | 1s |
| 65253 | HOURS | Engine Hours, Revolutions | 1s |

---

### j1708_j1587_definitions.h
Legacy J1708/J1587 protocol definitions for RS-485 based communication.

**Contents:**
- J1708 physical layer constants (9600 bps, timing)
- MID (Message Identifier) definitions - ECU source addresses
- PID (Parameter Identifier) definitions with:
  - Data lengths
  - Scale/offset
  - Engineering units
- ABS-specific definitions (Bendix, Meritor/WABCO)
- SID/FMI diagnostic codes
- Checksum calculation helpers

**Key MIDs:**
| MID | Device |
|-----|--------|
| 128 | Engine #1 |
| 130 | Transmission |
| 140 | Instrument Cluster |
| 172 | Tractor ABS |
| 136-137 | Trailer ABS |

---

### j1939_heavy_duty.dbc
Standard DBC (Vector CANdb) file for use with CAN analysis tools.

**Compatible Tools:**
- Vector CANalyzer/CANoe
- PEAK PCAN-View
- Kvaser CANKing
- python-can with cantools
- BusMaster
- SavvyCAN

**Usage with cantools (Python):**
```python
import cantools

db = cantools.database.load_file('j1939_heavy_duty.dbc')
msg = db.get_message_by_name('EEC1')
data = msg.decode(b'\x00\x00\x00\x1F\x40\x00\x00\x00')
print(f"Engine Speed: {data['EngineSpeed']} rpm")
```

---

## Protocol Reference

### J1939 CAN ID Structure (29-bit Extended)
```
Bits 28-26: Priority (3 bits) - 0-7, lower = higher priority
Bits 25-24: Reserved (2 bits) - Extended Data Page, Data Page
Bits 23-16: PDU Format (8 bits) - Determines PDU1 vs PDU2
Bits 15-8:  PDU Specific (8 bits) - Destination or Group Extension
Bits 7-0:   Source Address (8 bits) - Transmitting ECU
```

### J1708 Message Structure
```
MID (1 byte) + Data (0-19 bytes) + Checksum (1 byte)
Checksum = Two's complement of sum of all preceding bytes
```

---

## Sources & References

- SAE J1939-71 Vehicle Application Layer
- SAE J1939-21 Data Link Layer
- SAE J1587 Joint SAE/TMC Electronic Data Interchange
- SAE J1708 Serial Communication Between Devices
- Open-SAE-J1939 (GitHub: DanielMartensson)
- CSS Electronics J1939 Tutorial

## License

These protocol definitions are based on publicly available SAE standards and open-source implementations. For commercial use requiring full SAE document compliance, obtain official SAE specifications.
