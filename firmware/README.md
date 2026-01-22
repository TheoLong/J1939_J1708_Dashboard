# J1939/J1708 Truck Dashboard - Firmware

ESP32-based firmware for reading and displaying heavy-duty truck data via J1939 (CAN) and J1708 (RS-485) protocols.

## Project Structure

```
firmware/
├── platformio.ini          # PlatformIO build configuration
├── src/
│   ├── main.cpp           # Main application entry point
│   ├── config.h           # Central configuration (pins, settings)
│   ├── can/
│   │   ├── can_driver.h   # ESP32 TWAI/CAN driver wrapper
│   │   ├── j1939_parser.h # J1939 message parser
│   │   └── j1939_parser.cpp
│   ├── j1708/
│   │   ├── j1708_parser.h # J1708/J1587 message parser
│   │   └── j1708_parser.cpp
│   ├── data/
│   │   ├── data_manager.h # Central parameter storage
│   │   ├── data_manager.cpp
│   │   ├── watch_list_manager.h # Display parameter selection
│   │   └── watch_list_manager.cpp
│   └── storage/
│       ├── nvs_storage.h  # Persistent storage (NVS)
│       └── nvs_storage.cpp
├── lib/
│   └── j1939_data/        # Protocol definitions
│       ├── j1939_pgn_definitions.h
│       ├── j1939_spn_extended.h
│       ├── j1708_j1587_definitions.h
│       └── test_data/     # Sample CAN logs
└── test/
    ├── test_j1939_parser.cpp
    └── test_j1708_parser.cpp
```

## Building

### Prerequisites

1. Install [PlatformIO](https://platformio.org/install)
2. VS Code with PlatformIO extension (recommended)

### Build Commands

```bash
# Build for ESP32
cd firmware
pio run

# Build and upload to ESP32
pio run -t upload

# Run unit tests (native)
pio test -e native

# Run tests on ESP32
pio test -e esp32dev_test

# Monitor serial output
pio device monitor -b 115200
```

## Hardware Connections

### CAN Bus (J1939)

| ESP32 Pin | SN65HVD230 | Function |
|-----------|------------|----------|
| GPIO 5    | CTX        | CAN TX   |
| GPIO 4    | CRX        | CAN RX   |
| 3.3V      | VCC        | Power    |
| GND       | GND        | Ground   |

### J1708 (RS-485)

| ESP32 Pin | RS485 Module | Function |
|-----------|--------------|----------|
| GPIO 17   | DI           | UART TX  |
| GPIO 16   | RO           | UART RX  |
| GPIO 25   | DE/RE        | Direction|
| 3.3V      | VCC          | Power    |
| GND       | GND          | Ground   |

## Configuration

Edit `src/config.h` to modify:
- Pin assignments
- Protocol settings (baud rates)
- Task priorities
- Debug options

## Testing Without Hardware

The project supports Phase 0 development without physical hardware:

1. **Parser Unit Tests**: Run with `pio test -e native`
2. **Test Data Generator**: Generate synthetic CAN data
   ```bash
   cd tools/test_data_generator
   python j1939_generator.py --scenario highway --duration 60 --output test.csv
   ```
3. **Parser Validation**: Validate against sample data
   ```bash
   python tools/validation/validate_parser.py
   ```

## Key Features

### J1939 Parser
- PGN extraction (PDU1 and PDU2 format handling)
- SPN decoding with scaling/offset
- Transport Protocol (BAM) reassembly for multi-packet messages
- DM1/DM2 diagnostic trouble code parsing

### J1708/J1587 Parser
- Message framing with checksum validation
- PID decoding for common parameters
- ABS fault code extraction

### Data Manager
- Thread-safe parameter storage
- Timestamping and freshness tracking
- Change callbacks for display updates

### NVS Storage
- Trip odometer (A/B)
- Lifetime statistics
- Fault code history
- User settings persistence

### Watch List Manager
- Configurable display parameters
- Threshold-based alerts
- Multi-page support

## Decoded Parameters

### Engine (from J1939)
- Engine Speed (RPM)
- Coolant Temperature
- Oil Pressure
- Boost Pressure
- Throttle Position
- Engine Hours
- Fuel Rate

### Transmission
- Trans Oil Temperature
- Current Gear
- Output Shaft Speed

### Vehicle
- Vehicle Speed
- Fuel Level
- Battery Voltage
- Ambient Temperature

## License

MIT License - See project root for details.
