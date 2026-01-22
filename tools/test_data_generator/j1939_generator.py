#!/usr/bin/env python3
"""
J1939 Test Data Generator for Truck Dashboard Project

Generates realistic J1939 CAN frames for testing parser libraries
without physical hardware. Supports multiple output formats and
driving scenarios.

Usage:
    python j1939_generator.py --scenario highway --duration 60 --output test.log
"""

import struct
import random
import json
import argparse
from dataclasses import dataclass, field
from typing import List, Dict, Optional, Tuple
from datetime import datetime
import math


@dataclass
class J1939Frame:
    """Represents a single J1939 CAN frame"""
    pgn: int
    source_address: int
    priority: int
    data: bytes
    timestamp: float

    @property
    def can_id(self) -> int:
        """Build 29-bit CAN ID from components"""
        # For PDU2 format (PGN >= 0xF000), PGN is directly embedded
        # Priority (3) | Reserved (1) | Data Page (1) | PF (8) | PS (8) | SA (8)
        return (self.priority << 26) | (self.pgn << 8) | self.source_address


# PGN Definitions
PGN_EEC1 = 61444   # Electronic Engine Controller 1
PGN_EEC2 = 61443   # Electronic Engine Controller 2
PGN_ET1 = 65262    # Engine Temperature 1
PGN_EFLP1 = 65263  # Engine Fluid Level/Pressure 1
PGN_CCVS = 65265   # Cruise Control/Vehicle Speed
PGN_LFE = 65266    # Fuel Economy
PGN_AMB = 65269    # Ambient Conditions
PGN_IC1 = 65270    # Intake/Exhaust Conditions 1
PGN_VEP1 = 65271   # Vehicle Electrical Power 1
PGN_TRF1 = 65272   # Transmission Fluids 1
PGN_DD = 65276     # Dash Display
PGN_HOURS = 65253  # Engine Hours
PGN_ETC2 = 61445   # Electronic Transmission Controller 2
PGN_DM1 = 65226    # Active Diagnostic Trouble Codes


@dataclass
class VehicleState:
    """Current simulated vehicle state"""
    engine_rpm: float = 0.0
    vehicle_speed_kmh: float = 0.0
    coolant_temp_c: float = 20.0
    oil_temp_c: float = 20.0
    oil_pressure_kpa: float = 0.0
    fuel_temp_c: float = 20.0
    intake_temp_c: float = 20.0
    boost_pressure_kpa: float = 101.0  # Atmospheric
    throttle_pct: float = 0.0
    engine_load_pct: float = 0.0
    fuel_level_pct: float = 100.0
    fuel_rate_lph: float = 0.0
    battery_voltage: float = 12.6
    ambient_temp_c: float = 20.0
    trans_oil_temp_c: float = 20.0
    current_gear: int = 0  # Neutral
    engine_hours: float = 10000.0
    barometric_kpa: float = 101.3


class J1939TestDataGenerator:
    """Generate realistic J1939 test data based on driving scenarios"""

    def __init__(self, seed: Optional[int] = None):
        if seed is not None:
            random.seed(seed)
        self.state = VehicleState()

    def create_pgn_61444_eec1(self) -> bytes:
        """Create Electronic Engine Controller 1 (Engine Speed, Torque)"""
        rpm_raw = int(self.state.engine_rpm / 0.125)
        torque_raw = int(self.state.engine_load_pct + 125)  # Offset -125
        
        return struct.pack('<BBHBBBB',
            0x00,                           # Torque mode
            torque_raw & 0xFF,              # Driver demand torque
            rpm_raw & 0xFFFF,               # Engine speed (16-bit)
            0xFF,                           # Source address
            0xFF,                           # Starter mode
            0xFF,                           # Engine demand torque
            0xFF                            # Reserved
        )

    def create_pgn_61443_eec2(self) -> bytes:
        """Create Electronic Engine Controller 2 (Throttle, Load)"""
        throttle_raw = int(self.state.throttle_pct / 0.4)
        load_raw = int(self.state.engine_load_pct)
        
        return struct.pack('<BBBBBBBB',
            0x00,                           # Switches
            throttle_raw & 0xFF,            # Accelerator pedal 1
            load_raw & 0xFF,                # Engine load
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF
        )

    def create_pgn_65262_et1(self) -> bytes:
        """Create Engine Temperature 1 (Coolant, Oil Temps)"""
        coolant_raw = int(self.state.coolant_temp_c + 40)
        fuel_temp_raw = int(self.state.fuel_temp_c + 40)
        oil_temp_raw = int((self.state.oil_temp_c + 273) / 0.03125)
        
        return struct.pack('<BBHHHB',
            coolant_raw & 0xFF,
            fuel_temp_raw & 0xFF,
            oil_temp_raw & 0xFFFF,
            0xFFFF,                         # Turbo oil temp
            0xFF, 0xFF                      # Intercooler temp
        )

    def create_pgn_65263_eflp1(self) -> bytes:
        """Create Engine Fluid Level/Pressure 1 (Oil Pressure)"""
        oil_pressure_raw = int(self.state.oil_pressure_kpa / 4)
        
        return struct.pack('<BBBBBBBB',
            0xFF,                           # Fuel delivery pressure
            0xFF,                           # Crankcase pressure
            0xFF,                           # Oil level
            oil_pressure_raw & 0xFF,        # Oil pressure
            0xFF, 0xFF,                     # Crankcase pressure (16-bit)
            0xFF, 0xFF                      # Coolant pressure/level
        )

    def create_pgn_65265_ccvs(self) -> bytes:
        """Create Cruise Control/Vehicle Speed"""
        speed_raw = int(self.state.vehicle_speed_kmh * 256)
        
        return struct.pack('<BHBBBBB',
            0xFF,                           # Switches
            speed_raw & 0xFFFF,             # Vehicle speed
            0x00,                           # Cruise switches
            0x00, 0x00,                     # Reserved
            0xFF, 0xFF                      # PTO
        )

    def create_pgn_65266_lfe(self) -> bytes:
        """Create Fuel Economy (Fuel Rate)"""
        fuel_rate_raw = int(self.state.fuel_rate_lph / 0.05)
        
        return struct.pack('<HHBBBB',
            fuel_rate_raw & 0xFFFF,         # Fuel rate
            0xFFFF,                         # Instantaneous fuel economy
            0xFF, 0xFF,                     # Average fuel economy
            0xFF, 0xFF                      # Throttle
        )

    def create_pgn_65269_amb(self) -> bytes:
        """Create Ambient Conditions"""
        baro_raw = int(self.state.barometric_kpa / 0.5)
        ambient_raw = int((self.state.ambient_temp_c + 273) / 0.03125)
        
        return struct.pack('<BHHBHB',
            baro_raw & 0xFF,                # Barometric pressure
            0xFFFF,                         # Cab temp (skip)
            ambient_raw & 0xFFFF,           # Ambient temp
            0xFF,                           # Air inlet temp
            0xFFFF,                         # Road surface temp
            0xFF
        )

    def create_pgn_65270_ic1(self) -> bytes:
        """Create Intake/Exhaust Conditions 1 (Boost Pressure)"""
        boost_raw = int(self.state.boost_pressure_kpa / 2)
        intake_raw = int(self.state.intake_temp_c + 40)
        
        return struct.pack('<BBBBBBBB',
            0xFF,                           # Particulate trap pressure
            boost_raw & 0xFF,               # Boost pressure
            intake_raw & 0xFF,              # Intake manifold temp
            0xFF,                           # Air inlet pressure
            0xFF,                           # Air filter diff pressure
            0xFF, 0xFF,                     # EGT
            0xFF                            # Coolant filter diff
        )

    def create_pgn_65271_vep1(self) -> bytes:
        """Create Vehicle Electrical Power 1 (Battery Voltage)"""
        voltage_raw = int(self.state.battery_voltage / 0.05)
        
        return struct.pack('<HHHH',
            0xFFFF,                         # Net battery current
            0xFFFF,                         # Alternator current
            0xFFFF,                         # Charging voltage
            voltage_raw & 0xFFFF            # Battery voltage
        )

    def create_pgn_65272_trf1(self) -> bytes:
        """Create Transmission Fluids 1 (Trans Temp)"""
        trans_temp_raw = int((self.state.trans_oil_temp_c + 273) / 0.03125)
        
        return struct.pack('<BBBBHBB',
            0xFF,                           # Clutch pressure
            0xFF,                           # Trans oil level
            0xFF,                           # Filter diff
            0xFF,                           # Trans oil pressure
            trans_temp_raw & 0xFFFF,        # Trans oil temp (2 bytes)
            0xFF,                           # Oil level high/low
            0xFF                            # Timer
        )

    def create_pgn_65276_dd(self) -> bytes:
        """Create Dash Display (Fuel Level)"""
        fuel_raw = int(self.state.fuel_level_pct / 0.4)
        
        return struct.pack('<BBBBBBBB',
            0xFF,                           # Washer fluid
            fuel_raw & 0xFF,                # Fuel level 1
            0xFF,                           # Fuel filter diff
            0xFF,                           # Oil filter diff
            0xFF, 0xFF,                     # Cargo temp
            0xFF, 0xFF                      # Fuel level 2
        )

    def create_pgn_65253_hours(self) -> bytes:
        """Create Engine Hours, Revolutions"""
        hours_raw = int(self.state.engine_hours / 0.05)
        
        return struct.pack('<II',
            hours_raw & 0xFFFFFFFF,         # Engine hours
            0xFFFFFFFF                      # Engine revolutions
        )

    def create_pgn_61445_etc2(self) -> bytes:
        """Create Electronic Transmission Controller 2 (Gear)"""
        gear_raw = self.state.current_gear + 125
        
        return struct.pack('<BBBBBBBB',
            gear_raw & 0xFF,                # Selected gear
            0xFF, 0xFF,                     # Gear ratio
            gear_raw & 0xFF,                # Current gear
            0xFF, 0xFF, 0xFF, 0xFF
        )

    def create_dm1_fault(self, spn: int, fmi: int, oc: int = 1) -> bytes:
        """Create DM1 Active Diagnostic Trouble Code"""
        # DTC structure: SPN (19 bits) | FMI (5 bits) | CM (1 bit) | OC (7 bits)
        spn_low = spn & 0xFFFF
        spn_high = (spn >> 16) & 0x07
        dtc_byte3 = (spn_high << 5) | (fmi & 0x1F)
        dtc_byte4 = oc & 0x7F
        
        return struct.pack('<BBBBBBBB',
            0x00,                           # Lamp status 1 (protect/amber off)
            0x10,                           # Lamp status 2 (MIL on)
            spn_low & 0xFF,                 # SPN low byte
            (spn_low >> 8) & 0xFF,          # SPN mid byte
            dtc_byte3,                      # SPN high bits + FMI
            dtc_byte4,                      # Occurrence count
            0xFF, 0xFF                      # Padding
        )

    def update_state_idle(self, delta_s: float):
        """Update state for idle scenario"""
        target_rpm = 650 + random.uniform(-20, 20)
        self.state.engine_rpm += (target_rpm - self.state.engine_rpm) * 0.1
        self.state.vehicle_speed_kmh = 0
        self.state.throttle_pct = 0
        self.state.engine_load_pct = 15 + random.uniform(-2, 2)
        
        # Warm up temperatures
        target_coolant = 85.0
        self.state.coolant_temp_c += (target_coolant - self.state.coolant_temp_c) * 0.001 * delta_s
        self.state.oil_temp_c = self.state.coolant_temp_c - 5
        self.state.trans_oil_temp_c = self.state.coolant_temp_c - 10
        
        # Low boost at idle (slightly above atmospheric)
        self.state.boost_pressure_kpa = 102 + random.uniform(-1, 1)
        
        # Oil pressure at idle
        self.state.oil_pressure_kpa = 200 + random.uniform(-10, 10)
        
        # Low fuel consumption at idle
        self.state.fuel_rate_lph = 3.0 + random.uniform(-0.5, 0.5)
        
        # Battery charging
        self.state.battery_voltage = 13.8 + random.uniform(-0.2, 0.2)

    def update_state_highway(self, delta_s: float):
        """Update state for highway driving"""
        target_rpm = 1500 + random.uniform(-50, 50)
        target_speed = 105 + random.uniform(-3, 3)
        
        self.state.engine_rpm += (target_rpm - self.state.engine_rpm) * 0.05
        self.state.vehicle_speed_kmh += (target_speed - self.state.vehicle_speed_kmh) * 0.05
        self.state.throttle_pct = 40 + random.uniform(-5, 5)
        self.state.engine_load_pct = 50 + random.uniform(-10, 10)
        
        # Warm temperatures
        self.state.coolant_temp_c = 88 + random.uniform(-2, 2)
        self.state.oil_temp_c = 95 + random.uniform(-3, 3)
        self.state.trans_oil_temp_c = 80 + random.uniform(-5, 5)
        self.state.intake_temp_c = 45 + random.uniform(-3, 3)
        
        # Boost under load
        self.state.boost_pressure_kpa = 180 + random.uniform(-10, 10)
        
        # Normal oil pressure
        self.state.oil_pressure_kpa = 400 + random.uniform(-20, 20)
        
        # Highway fuel consumption
        self.state.fuel_rate_lph = 35 + random.uniform(-5, 5)
        
        # Battery voltage
        self.state.battery_voltage = 14.2 + random.uniform(-0.2, 0.2)
        
        # Set gear
        self.state.current_gear = 10  # Top gear

    def update_state_acceleration(self, delta_s: float, progress: float):
        """Update state for acceleration scenario"""
        # Progress 0-1 represents acceleration phase
        target_rpm = 800 + (1800 * progress) + random.uniform(-30, 30)
        target_speed = 10 + (90 * progress) + random.uniform(-2, 2)
        
        self.state.engine_rpm = target_rpm
        self.state.vehicle_speed_kmh = max(0, target_speed)
        self.state.throttle_pct = 80 + random.uniform(-5, 5)
        self.state.engine_load_pct = 85 + random.uniform(-5, 5)
        
        # Higher temps under load
        self.state.coolant_temp_c = 90 + random.uniform(-2, 2)
        self.state.oil_temp_c = 100 + random.uniform(-3, 3)
        
        # High boost
        self.state.boost_pressure_kpa = 220 + random.uniform(-10, 10)
        
        # High oil pressure
        self.state.oil_pressure_kpa = 500 + random.uniform(-30, 30)
        
        # High fuel consumption
        self.state.fuel_rate_lph = 80 + random.uniform(-10, 10)
        
        # Shift through gears
        self.state.current_gear = min(10, int(progress * 10) + 1)

    def update_state_cold_start(self, delta_s: float, elapsed_s: float):
        """Update state for cold start warmup scenario"""
        # Engine cranking then idle warmup
        if elapsed_s < 2:
            # Cranking
            self.state.engine_rpm = 200 + random.uniform(-20, 20)
            warmup_factor = 0.0  # No warmup yet during cranking
        else:
            # Fast idle warmup
            warmup_factor = min(1.0, (elapsed_s - 2) / 300)  # 5 min warmup
            target_rpm = 900 - (200 * warmup_factor)  # Fast idle drops as warming
            self.state.engine_rpm = target_rpm + random.uniform(-15, 15)
        
        self.state.vehicle_speed_kmh = 0
        self.state.throttle_pct = 0
        
        # Cold start temps warming
        warmup_rate = 0.1 * delta_s
        self.state.coolant_temp_c = min(85, -10 + (elapsed_s * 0.3))
        self.state.oil_temp_c = self.state.coolant_temp_c - 5
        
        # Low oil pressure when cold
        self.state.oil_pressure_kpa = 150 + (warmup_factor * 100) + random.uniform(-10, 10)
        
        # Battery voltage during cranking
        if elapsed_s < 2:
            self.state.battery_voltage = 10.5 + random.uniform(-0.3, 0.3)
        else:
            self.state.battery_voltage = 14.0 + random.uniform(-0.2, 0.2)

    def generate_frames(self, scenario: str, duration_s: float, 
                        step_ms: int = 100) -> List[J1939Frame]:
        """Generate frames for a given scenario"""
        frames = []
        elapsed_ms = 0
        duration_ms = int(duration_s * 1000)
        
        while elapsed_ms < duration_ms:
            timestamp = elapsed_ms / 1000.0
            delta_s = step_ms / 1000.0
            progress = elapsed_ms / duration_ms
            
            # Update state based on scenario
            if scenario == 'idle':
                self.update_state_idle(delta_s)
            elif scenario == 'highway':
                self.update_state_highway(delta_s)
            elif scenario == 'acceleration':
                self.update_state_acceleration(delta_s, progress)
            elif scenario == 'cold_start':
                self.update_state_cold_start(delta_s, timestamp)
            
            # Decrement fuel level slowly
            self.state.fuel_level_pct = max(0, self.state.fuel_level_pct - 
                                            (self.state.fuel_rate_lph / 36000) * delta_s)
            
            # Increment engine hours
            self.state.engine_hours += delta_s / 3600
            
            # EEC1 at 10-20 Hz
            frames.append(J1939Frame(
                pgn=PGN_EEC1, source_address=0x00, priority=3,
                data=self.create_pgn_61444_eec1(), timestamp=timestamp
            ))
            
            # EEC2 at 20 Hz
            if elapsed_ms % 50 == 0:
                frames.append(J1939Frame(
                    pgn=PGN_EEC2, source_address=0x00, priority=3,
                    data=self.create_pgn_61443_eec2(), timestamp=timestamp
                ))
            
            # CCVS at 10 Hz
            frames.append(J1939Frame(
                pgn=PGN_CCVS, source_address=0x00, priority=6,
                data=self.create_pgn_65265_ccvs(), timestamp=timestamp
            ))
            
            # LFE at 10 Hz
            frames.append(J1939Frame(
                pgn=PGN_LFE, source_address=0x00, priority=6,
                data=self.create_pgn_65266_lfe(), timestamp=timestamp
            ))
            
            # IC1 at 2 Hz
            if elapsed_ms % 500 == 0:
                frames.append(J1939Frame(
                    pgn=PGN_IC1, source_address=0x00, priority=6,
                    data=self.create_pgn_65270_ic1(), timestamp=timestamp
                ))
                frames.append(J1939Frame(
                    pgn=PGN_EFLP1, source_address=0x00, priority=6,
                    data=self.create_pgn_65263_eflp1(), timestamp=timestamp
                ))
            
            # ET1 at 1 Hz
            if elapsed_ms % 1000 == 0:
                frames.append(J1939Frame(
                    pgn=PGN_ET1, source_address=0x00, priority=6,
                    data=self.create_pgn_65262_et1(), timestamp=timestamp
                ))
                frames.append(J1939Frame(
                    pgn=PGN_VEP1, source_address=0x00, priority=6,
                    data=self.create_pgn_65271_vep1(), timestamp=timestamp
                ))
                frames.append(J1939Frame(
                    pgn=PGN_TRF1, source_address=0x03, priority=6,
                    data=self.create_pgn_65272_trf1(), timestamp=timestamp
                ))
                frames.append(J1939Frame(
                    pgn=PGN_DD, source_address=0x00, priority=6,
                    data=self.create_pgn_65276_dd(), timestamp=timestamp
                ))
                frames.append(J1939Frame(
                    pgn=PGN_AMB, source_address=0x00, priority=6,
                    data=self.create_pgn_65269_amb(), timestamp=timestamp
                ))
                frames.append(J1939Frame(
                    pgn=PGN_ETC2, source_address=0x03, priority=6,
                    data=self.create_pgn_61445_etc2(), timestamp=timestamp
                ))
            
            # HOURS at 0.1 Hz
            if elapsed_ms % 10000 == 0:
                frames.append(J1939Frame(
                    pgn=PGN_HOURS, source_address=0x00, priority=6,
                    data=self.create_pgn_65253_hours(), timestamp=timestamp
                ))
            
            elapsed_ms += step_ms
        
        return frames

    def add_fault_injection(self, frames: List[J1939Frame], 
                            fault_time_s: float, spn: int, fmi: int) -> List[J1939Frame]:
        """Add a DM1 fault code at specified time"""
        frames.append(J1939Frame(
            pgn=PGN_DM1, source_address=0x00, priority=6,
            data=self.create_dm1_fault(spn, fmi), timestamp=fault_time_s
        ))
        return sorted(frames, key=lambda f: f.timestamp)

    def export_to_socketcan_log(self, frames: List[J1939Frame], filename: str):
        """Export frames to SocketCAN log format"""
        with open(filename, 'w') as f:
            for frame in frames:
                data_hex = ''.join(f'{b:02X}' for b in frame.data)
                f.write(f"({frame.timestamp:.6f}) can0 {frame.can_id:08X}#{data_hex}\n")

    def export_to_csv(self, frames: List[J1939Frame], filename: str):
        """Export frames to CSV format with decoded values"""
        with open(filename, 'w') as f:
            f.write("timestamp,can_id,pgn,source_addr,data_hex\n")
            for frame in frames:
                data_hex = ' '.join(f'{b:02X}' for b in frame.data)
                f.write(f"{frame.timestamp:.6f},0x{frame.can_id:08X},{frame.pgn},"
                        f"{frame.source_address},{data_hex}\n")

    def export_to_asc(self, frames: List[J1939Frame], filename: str):
        """Export frames to Vector ASC format"""
        with open(filename, 'w') as f:
            f.write(f"; CANalyzer/CANoe ASC Log File\n")
            f.write(f"; Generated by J1939 Test Data Generator\n")
            f.write(f"; Date: {datetime.now().strftime('%a %b %d %I:%M:%S %p %Y')}\n")
            f.write(";\n")
            f.write("date " + datetime.now().strftime("%a %b %d %I:%M:%S %p %Y") + "\n")
            f.write("base hex  timestamps absolute\n")
            f.write("no internal events logged\n\n")
            
            for frame in frames:
                data_str = ' '.join(f'{b:02X}' for b in frame.data)
                f.write(f"  {frame.timestamp:10.6f} 1  {frame.can_id:08X}x       "
                        f"Rx   d {len(frame.data)} {data_str}\n")

    def export_decoded_csv(self, frames: List[J1939Frame], filename: str):
        """Export frames with decoded parameter values"""
        with open(filename, 'w') as f:
            f.write("timestamp,pgn,pgn_name,parameter,value,unit\n")
            
            for frame in frames:
                decoded = self._decode_frame(frame)
                for param_name, (value, unit) in decoded.items():
                    f.write(f"{frame.timestamp:.6f},{frame.pgn},"
                            f"{self._get_pgn_name(frame.pgn)},{param_name},"
                            f"{value:.2f},{unit}\n")

    def _get_pgn_name(self, pgn: int) -> str:
        """Get PGN name"""
        names = {
            PGN_EEC1: "EEC1", PGN_EEC2: "EEC2", PGN_ET1: "ET1",
            PGN_EFLP1: "EFLP1", PGN_CCVS: "CCVS", PGN_LFE: "LFE",
            PGN_AMB: "AMB", PGN_IC1: "IC1", PGN_VEP1: "VEP1",
            PGN_TRF1: "TRF1", PGN_DD: "DD", PGN_HOURS: "HOURS",
            PGN_ETC2: "ETC2", PGN_DM1: "DM1"
        }
        return names.get(pgn, f"PGN_{pgn}")

    def _decode_frame(self, frame: J1939Frame) -> Dict[str, Tuple[float, str]]:
        """Decode frame to parameter values"""
        decoded = {}
        data = frame.data
        
        if frame.pgn == PGN_EEC1:
            rpm_raw = struct.unpack('<H', data[2:4])[0]
            decoded['engine_speed'] = (rpm_raw * 0.125, 'rpm')
        elif frame.pgn == PGN_ET1:
            decoded['coolant_temp'] = (data[0] - 40, '°C')
        elif frame.pgn == PGN_CCVS:
            speed_raw = struct.unpack('<H', data[1:3])[0]
            decoded['vehicle_speed'] = (speed_raw / 256.0, 'km/h')
        elif frame.pgn == PGN_IC1:
            decoded['boost_pressure'] = (data[1] * 2, 'kPa')
        elif frame.pgn == PGN_VEP1:
            voltage_raw = struct.unpack('<H', data[6:8])[0]
            decoded['battery_voltage'] = (voltage_raw * 0.05, 'V')
        elif frame.pgn == PGN_DD:
            decoded['fuel_level'] = (data[1] * 0.4, '%')
        
        return decoded


def main():
    parser = argparse.ArgumentParser(description='Generate J1939 test data')
    parser.add_argument('--scenario', type=str, default='highway',
                        choices=['idle', 'highway', 'acceleration', 'cold_start'],
                        help='Driving scenario to simulate')
    parser.add_argument('--duration', type=float, default=60.0,
                        help='Duration in seconds')
    parser.add_argument('--output', type=str, default='test_data.log',
                        help='Output filename')
    parser.add_argument('--format', type=str, default='socketcan',
                        choices=['socketcan', 'csv', 'asc', 'decoded'],
                        help='Output format')
    parser.add_argument('--seed', type=int, default=None,
                        help='Random seed for reproducibility')
    parser.add_argument('--fault', action='store_true',
                        help='Inject a sample fault code')
    
    args = parser.parse_args()
    
    gen = J1939TestDataGenerator(seed=args.seed)
    
    print(f"Generating {args.scenario} scenario for {args.duration}s...")
    frames = gen.generate_frames(args.scenario, args.duration)
    
    if args.fault:
        # Inject coolant over-temp fault at 30s
        frames = gen.add_fault_injection(frames, 30.0, 110, 0)
        print("  Added fault code injection at t=30s")
    
    print(f"Generated {len(frames)} frames")
    
    if args.format == 'socketcan':
        gen.export_to_socketcan_log(frames, args.output)
    elif args.format == 'csv':
        gen.export_to_csv(frames, args.output)
    elif args.format == 'asc':
        gen.export_to_asc(frames, args.output)
    elif args.format == 'decoded':
        gen.export_decoded_csv(frames, args.output)
    
    print(f"Saved to {args.output}")


if __name__ == "__main__":
    main()
