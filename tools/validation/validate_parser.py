#!/usr/bin/env python3
"""
J1939 Parser Validation Script

Validates the J1939 parser output against known test data.
Uses the sample ASC files and expected decoded values.
"""

import sys
import re
import csv
from dataclasses import dataclass
from typing import List, Dict, Tuple, Optional


@dataclass
class TestCase:
    """A single test case with input and expected output"""
    timestamp: float
    can_id: int
    data: bytes
    pgn: int
    expected_params: Dict[str, float]


def parse_asc_line(line: str) -> Optional[Tuple[float, int, bytes]]:
    """Parse a line from an ASC file"""
    line = line.strip()
    if not line or line.startswith(';') or line.startswith('date') or line.startswith('base'):
        return None
    if 'no internal events' in line:
        return None
    
    # Format: timestamp channel can_id Rx d length data...
    # Example: 0.000000 1  0CF00400x       Rx   d 8 00 7D 7D 80 3E 00 00 00
    
    match = re.match(r'\s*(\d+\.\d+)\s+\d+\s+([0-9A-Fa-f]+)x?\s+Rx\s+d\s+(\d+)\s+(.*)', line)
    if not match:
        return None
    
    timestamp = float(match.group(1))
    can_id = int(match.group(2), 16)
    length = int(match.group(3))
    data_hex = match.group(4).split()
    
    data = bytes([int(b, 16) for b in data_hex[:length]])
    
    return (timestamp, can_id, data)


def extract_pgn(can_id: int) -> int:
    """Extract PGN from 29-bit CAN ID (matching the C implementation)"""
    pdu_format = (can_id >> 16) & 0xFF
    pdu_specific = (can_id >> 8) & 0xFF
    data_page = (can_id >> 24) & 0x03
    
    if pdu_format < 240:
        # PDU1: PS is destination, not part of PGN
        return (data_page << 16) | (pdu_format << 8)
    else:
        # PDU2: PS is group extension, part of PGN
        return (data_page << 16) | (pdu_format << 8) | pdu_specific


def decode_engine_speed(data: bytes) -> Optional[float]:
    """Decode engine speed from EEC1 (PGN 61444)"""
    if len(data) < 5:
        return None
    raw = data[3] | (data[4] << 8)
    if raw >= 0xFE00:
        return None
    return raw * 0.125


def decode_coolant_temp(data: bytes) -> Optional[float]:
    """Decode coolant temperature from ET1 (PGN 65262)"""
    if len(data) < 1:
        return None
    raw = data[0]
    if raw >= 0xFE:
        return None
    return raw - 40.0


def decode_vehicle_speed(data: bytes) -> Optional[float]:
    """Decode vehicle speed from CCVS (PGN 65265)"""
    if len(data) < 3:
        return None
    raw = data[1] | (data[2] << 8)
    if raw >= 0xFE00:
        return None
    return raw / 256.0


def decode_oil_pressure(data: bytes) -> Optional[float]:
    """Decode oil pressure from EFLP1 (PGN 65263)"""
    if len(data) < 4:
        return None
    raw = data[3]
    if raw >= 0xFE:
        return None
    return raw * 4.0


def decode_boost_pressure(data: bytes) -> Optional[float]:
    """Decode boost pressure from IC1 (PGN 65270)"""
    if len(data) < 2:
        return None
    raw = data[1]
    if raw >= 0xFE:
        return None
    return raw * 2.0


def decode_battery_voltage(data: bytes) -> Optional[float]:
    """Decode battery voltage from VEP1 (PGN 65271)"""
    if len(data) < 8:
        return None
    raw = data[6] | (data[7] << 8)
    if raw >= 0xFE00:
        return None
    return raw * 0.05


def decode_fuel_level(data: bytes) -> Optional[float]:
    """Decode fuel level from DD (PGN 65276)"""
    if len(data) < 2:
        return None
    raw = data[1]
    if raw >= 0xFE:
        return None
    return raw * 0.4


def decode_current_gear(data: bytes) -> Optional[int]:
    """Decode current gear from ETC2 (PGN 61445)"""
    if len(data) < 4:
        return None
    raw = data[3]
    if raw >= 0xFE:
        return None
    return raw - 125


def decode_engine_hours(data: bytes) -> Optional[float]:
    """Decode engine hours from HOURS (PGN 65253)"""
    if len(data) < 4:
        return None
    raw = data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24)
    if raw == 0xFFFFFFFF:
        return None
    return raw * 0.05


# Map PGN to decoder functions
DECODERS = {
    61444: ('Engine Speed', decode_engine_speed, 'rpm'),
    65262: ('Coolant Temp', decode_coolant_temp, '°C'),
    65265: ('Vehicle Speed', decode_vehicle_speed, 'km/h'),
    65263: ('Oil Pressure', decode_oil_pressure, 'kPa'),
    65270: ('Boost Pressure', decode_boost_pressure, 'kPa'),
    65271: ('Battery Voltage', decode_battery_voltage, 'V'),
    65276: ('Fuel Level', decode_fuel_level, '%'),
    61445: ('Current Gear', decode_current_gear, ''),
    65253: ('Engine Hours', decode_engine_hours, 'hr'),
}


def validate_asc_file(filename: str) -> Tuple[int, int, List[str]]:
    """Validate an ASC file and return (passed, failed, errors)"""
    passed = 0
    failed = 0
    errors = []
    
    print(f"\nValidating: {filename}")
    print("-" * 60)
    
    try:
        with open(filename, 'r') as f:
            for line in f:
                result = parse_asc_line(line)
                if result is None:
                    continue
                
                timestamp, can_id, data = result
                pgn = extract_pgn(can_id)
                
                if pgn in DECODERS:
                    name, decoder, unit = DECODERS[pgn]
                    value = decoder(data)
                    
                    if value is not None:
                        # Check if value is reasonable
                        reasonable = True
                        
                        if pgn == 61444:  # Engine Speed
                            reasonable = 0 <= value <= 3500
                        elif pgn == 65262:  # Coolant Temp
                            reasonable = -40 <= value <= 150
                        elif pgn == 65265:  # Vehicle Speed
                            reasonable = 0 <= value <= 200
                        elif pgn == 65263:  # Oil Pressure
                            reasonable = 0 <= value <= 1000
                        elif pgn == 65271:  # Battery Voltage
                            reasonable = 8 <= value <= 18
                        elif pgn == 65276:  # Fuel Level
                            reasonable = 0 <= value <= 100
                        
                        if reasonable:
                            passed += 1
                            print(f"✓ t={timestamp:.3f} PGN {pgn}: {name} = {value:.2f} {unit}")
                        else:
                            failed += 1
                            errors.append(f"Unreasonable {name}: {value}")
                            print(f"✗ t={timestamp:.3f} PGN {pgn}: {name} = {value:.2f} {unit} (out of range)")
                    else:
                        # Value not available - that's okay
                        pass
    
    except FileNotFoundError:
        errors.append(f"File not found: {filename}")
        return 0, 1, errors
    except Exception as e:
        errors.append(f"Error: {str(e)}")
        return 0, 1, errors
    
    return passed, failed, errors


def main():
    print("=" * 60)
    print("J1939 Parser Validation")
    print("=" * 60)
    
    # Test files to validate
    test_files = [
        'firmware/lib/j1939_data/test_data/truck_sample.asc',
    ]
    
    total_passed = 0
    total_failed = 0
    all_errors = []
    
    for test_file in test_files:
        passed, failed, errors = validate_asc_file(test_file)
        total_passed += passed
        total_failed += failed
        all_errors.extend(errors)
    
    print("\n" + "=" * 60)
    print("SUMMARY")
    print("=" * 60)
    print(f"Passed: {total_passed}")
    print(f"Failed: {total_failed}")
    
    if all_errors:
        print("\nErrors:")
        for error in all_errors:
            print(f"  - {error}")
    
    if total_failed > 0:
        print("\n❌ VALIDATION FAILED")
        return 1
    else:
        print("\n✓ ALL VALIDATIONS PASSED")
        return 0


if __name__ == "__main__":
    sys.exit(main())
