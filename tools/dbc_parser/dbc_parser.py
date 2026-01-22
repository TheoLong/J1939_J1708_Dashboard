#!/usr/bin/env python3
"""
DBC Parser for J1939
Parses Vector DBC files and generates C header files for ESP32 firmware.

Usage:
    python dbc_parser.py input.dbc --output generated_signals.h
"""

import re
import argparse
from dataclasses import dataclass, field
from typing import Dict, List, Optional, Tuple
from pathlib import Path


@dataclass
class Signal:
    """Represents a CAN signal within a message"""
    name: str
    start_bit: int
    bit_length: int
    byte_order: str  # '@1+' = little endian, '@0+' = big endian
    is_signed: bool
    scale: float
    offset: float
    min_value: float
    max_value: float
    unit: str
    receivers: List[str] = field(default_factory=list)
    comment: str = ""
    value_descriptions: Dict[int, str] = field(default_factory=dict)
    
    @property
    def c_type(self) -> str:
        """Determine appropriate C type for this signal"""
        if self.bit_length <= 8:
            return "int8_t" if self.is_signed else "uint8_t"
        elif self.bit_length <= 16:
            return "int16_t" if self.is_signed else "uint16_t"
        elif self.bit_length <= 32:
            return "int32_t" if self.is_signed else "uint32_t"
        else:
            return "int64_t" if self.is_signed else "uint64_t"
    
    @property
    def needs_float(self) -> bool:
        """Check if decoded value needs float type"""
        return self.scale != 1.0 or self.offset != 0.0
    
    @property
    def c_name(self) -> str:
        """Convert signal name to C-style identifier"""
        # Convert CamelCase to snake_case
        name = re.sub('(.)([A-Z][a-z]+)', r'\1_\2', self.name)
        name = re.sub('([a-z0-9])([A-Z])', r'\1_\2', name)
        return name.lower()


@dataclass 
class Message:
    """Represents a CAN message"""
    can_id: int
    name: str
    dlc: int
    transmitter: str
    signals: List[Signal] = field(default_factory=list)
    comment: str = ""
    cycle_time_ms: int = 0
    
    @property
    def pgn(self) -> int:
        """Extract J1939 PGN from CAN ID"""
        # CAN ID format: PPP.P.PPPP.PPPP.SSSS.SSSS (29-bit)
        # Priority (3), Reserved (1), Data Page (1), PDU Format (8), 
        # PDU Specific (8), Source Address (8)
        pdu_format = (self.can_id >> 16) & 0xFF
        pdu_specific = (self.can_id >> 8) & 0xFF
        data_page = (self.can_id >> 24) & 0x01
        
        if pdu_format < 240:
            # PDU1: PS is destination address, not part of PGN
            return (data_page << 16) | (pdu_format << 8)
        else:
            # PDU2: PS is part of PGN (group extension)
            return (data_page << 16) | (pdu_format << 8) | pdu_specific
    
    @property
    def c_name(self) -> str:
        """Convert message name to C-style constant"""
        return self.name.upper()


@dataclass
class DbcDatabase:
    """Complete DBC database"""
    version: str = ""
    messages: Dict[int, Message] = field(default_factory=dict)
    nodes: List[str] = field(default_factory=list)
    value_tables: Dict[str, Dict[int, str]] = field(default_factory=dict)
    comments: Dict[str, str] = field(default_factory=dict)


class DbcParser:
    """Parser for Vector DBC format files"""
    
    def __init__(self):
        self.db = DbcDatabase()
        
    def parse_file(self, filepath: str) -> DbcDatabase:
        """Parse a DBC file and return database"""
        with open(filepath, 'r', encoding='utf-8', errors='replace') as f:
            content = f.read()
        
        self._parse_version(content)
        self._parse_nodes(content)
        self._parse_value_tables(content)
        self._parse_messages(content)
        self._parse_signals(content)
        self._parse_comments(content)
        self._parse_value_descriptions(content)
        self._parse_attributes(content)
        
        return self.db
    
    def _parse_version(self, content: str):
        """Parse VERSION section"""
        match = re.search(r'VERSION\s+"([^"]*)"', content)
        if match:
            self.db.version = match.group(1)
    
    def _parse_nodes(self, content: str):
        """Parse BU_ (nodes) section"""
        match = re.search(r'BU_:\s*(.+?)(?:\n\n|\nBS_)', content, re.DOTALL)
        if match:
            nodes_str = match.group(1).strip()
            self.db.nodes = nodes_str.split()
    
    def _parse_value_tables(self, content: str):
        """Parse VAL_TABLE_ definitions"""
        pattern = r'VAL_TABLE_\s+(\w+)\s+([\s\S]*?);'
        for match in re.finditer(pattern, content):
            table_name = match.group(1)
            values_str = match.group(2)
            values = {}
            for val_match in re.finditer(r'(\d+)\s+"([^"]*)"', values_str):
                values[int(val_match.group(1))] = val_match.group(2)
            self.db.value_tables[table_name] = values
    
    def _parse_messages(self, content: str):
        """Parse BO_ (message) definitions"""
        # BO_ <CAN-ID> <MessageName>: <MessageLength> <Transmitter>
        pattern = r'BO_\s+(\d+)\s+(\w+)\s*:\s*(\d+)\s+(\w+)'
        for match in re.finditer(pattern, content):
            can_id = int(match.group(1))
            msg = Message(
                can_id=can_id,
                name=match.group(2),
                dlc=int(match.group(3)),
                transmitter=match.group(4)
            )
            self.db.messages[can_id] = msg
    
    def _parse_signals(self, content: str):
        """Parse SG_ (signal) definitions within messages"""
        # Find each message block and its signals
        msg_pattern = r'BO_\s+(\d+)\s+\w+\s*:\s*\d+\s+\w+\s*((?:\s*SG_\s+.+)+)'
        
        for msg_match in re.finditer(msg_pattern, content):
            can_id = int(msg_match.group(1))
            signals_block = msg_match.group(2)
            
            if can_id not in self.db.messages:
                continue
            
            # SG_ <SignalName> : <StartBit>|<Length>@<ByteOrder><ValueType> 
            #     (<Factor>,<Offset>) [<Min>|<Max>] "<Unit>" <Receiver>
            sig_pattern = (
                r'SG_\s+(\w+)\s*:\s*(\d+)\|(\d+)@([01])([+-])\s*'
                r'\(([^,]+),([^)]+)\)\s*\[([^|]+)\|([^\]]+)\]\s*'
                r'"([^"]*)"\s*(\S+)'
            )
            
            for sig_match in re.finditer(sig_pattern, signals_block):
                signal = Signal(
                    name=sig_match.group(1),
                    start_bit=int(sig_match.group(2)),
                    bit_length=int(sig_match.group(3)),
                    byte_order='little' if sig_match.group(4) == '1' else 'big',
                    is_signed=(sig_match.group(5) == '-'),
                    scale=float(sig_match.group(6)),
                    offset=float(sig_match.group(7)),
                    min_value=float(sig_match.group(8)),
                    max_value=float(sig_match.group(9)),
                    unit=sig_match.group(10),
                    receivers=sig_match.group(11).split(',')
                )
                self.db.messages[can_id].signals.append(signal)
    
    def _parse_comments(self, content: str):
        """Parse CM_ (comment) sections"""
        # Message comments: CM_ BO_ <CAN-ID> "<Comment>";
        for match in re.finditer(r'CM_\s+BO_\s+(\d+)\s+"([^"]+)"', content):
            can_id = int(match.group(1))
            if can_id in self.db.messages:
                self.db.messages[can_id].comment = match.group(2)
        
        # Signal comments: CM_ SG_ <CAN-ID> <SignalName> "<Comment>";
        for match in re.finditer(r'CM_\s+SG_\s+(\d+)\s+(\w+)\s+"([^"]+)"', content):
            can_id = int(match.group(1))
            sig_name = match.group(2)
            if can_id in self.db.messages:
                for sig in self.db.messages[can_id].signals:
                    if sig.name == sig_name:
                        sig.comment = match.group(3)
                        break
    
    def _parse_value_descriptions(self, content: str):
        """Parse VAL_ (signal value descriptions)"""
        # VAL_ <CAN-ID> <SignalName> <Value> "<Description>" ... ;
        pattern = r'VAL_\s+(\d+)\s+(\w+)\s+((?:\d+\s+"[^"]*"\s*)+);'
        for match in re.finditer(pattern, content):
            can_id = int(match.group(1))
            sig_name = match.group(2)
            values_str = match.group(3)
            
            if can_id in self.db.messages:
                for sig in self.db.messages[can_id].signals:
                    if sig.name == sig_name:
                        for val_match in re.finditer(r'(\d+)\s+"([^"]*)"', values_str):
                            sig.value_descriptions[int(val_match.group(1))] = val_match.group(2)
                        break
    
    def _parse_attributes(self, content: str):
        """Parse BA_ (attribute) sections"""
        # GenMsgCycleTime: BA_ "GenMsgCycleTime" BO_ <CAN-ID> <Value>;
        for match in re.finditer(r'BA_\s+"GenMsgCycleTime"\s+BO_\s+(\d+)\s+(\d+)', content):
            can_id = int(match.group(1))
            if can_id in self.db.messages:
                self.db.messages[can_id].cycle_time_ms = int(match.group(2))


class CHeaderGenerator:
    """Generate C header files from parsed DBC database"""
    
    def __init__(self, db: DbcDatabase):
        self.db = db
    
    def generate(self) -> str:
        """Generate complete C header file"""
        lines = []
        
        # Header guard and includes
        lines.extend([
            "/**",
            " * @file j1939_dbc_generated.h",
            " * @brief Auto-generated J1939 signal definitions from DBC file",
            " * ",
            " * DO NOT EDIT - Generated by dbc_parser.py",
            " */",
            "",
            "#ifndef J1939_DBC_GENERATED_H",
            "#define J1939_DBC_GENERATED_H",
            "",
            "#include <stdint.h>",
            "#include <stdbool.h>",
            "",
            "#ifdef __cplusplus",
            'extern "C" {',
            "#endif",
            "",
        ])
        
        # PGN definitions
        lines.append("/*" + "=" * 75 + "*/")
        lines.append("/*                        PGN DEFINITIONS                                  */")
        lines.append("/*" + "=" * 75 + "*/")
        lines.append("")
        
        for msg in sorted(self.db.messages.values(), key=lambda m: m.pgn):
            pgn = msg.pgn
            lines.append(f"#define PGN_{msg.c_name:<20} 0x{pgn:04X}  // {pgn} - {msg.comment or msg.name}")
        
        lines.append("")
        
        # Message cycle times
        lines.append("/*" + "=" * 75 + "*/")
        lines.append("/*                        MESSAGE CYCLE TIMES (ms)                        */")
        lines.append("/*" + "=" * 75 + "*/")
        lines.append("")
        
        for msg in sorted(self.db.messages.values(), key=lambda m: m.pgn):
            if msg.cycle_time_ms > 0:
                lines.append(f"#define {msg.c_name}_CYCLE_MS          {msg.cycle_time_ms}")
        
        lines.append("")
        
        # Signal structures for each message
        lines.append("/*" + "=" * 75 + "*/")
        lines.append("/*                        SIGNAL STRUCTURES                               */")
        lines.append("/*" + "=" * 75 + "*/")
        lines.append("")
        
        for msg in sorted(self.db.messages.values(), key=lambda m: m.pgn):
            if not msg.signals:
                continue
            
            lines.append(f"/** {msg.name} - PGN {msg.pgn} (0x{msg.pgn:04X}) */")
            if msg.comment:
                lines.append(f"/* {msg.comment} */")
            lines.append(f"typedef struct {{")
            
            for sig in msg.signals:
                sig_type = "float" if sig.needs_float else sig.c_type
                unit_comment = f" // {sig.unit}" if sig.unit else ""
                lines.append(f"    {sig_type:<10} {sig.c_name};{unit_comment}")
            
            lines.append(f"}} {msg.name.lower()}_t;")
            lines.append("")
        
        # Decoder function declarations
        lines.append("/*" + "=" * 75 + "*/")
        lines.append("/*                        DECODER FUNCTIONS                               */")
        lines.append("/*" + "=" * 75 + "*/")
        lines.append("")
        
        for msg in sorted(self.db.messages.values(), key=lambda m: m.pgn):
            if not msg.signals:
                continue
            lines.append(f"/** Decode {msg.name} message (PGN {msg.pgn}) */")
            lines.append(f"void decode_{msg.name.lower()}(const uint8_t* data, {msg.name.lower()}_t* out);")
            lines.append("")
        
        # Individual signal decoder macros/functions
        lines.append("/*" + "=" * 75 + "*/")
        lines.append("/*                        SIGNAL EXTRACTION MACROS                        */")
        lines.append("/*" + "=" * 75 + "*/")
        lines.append("")
        
        for msg in sorted(self.db.messages.values(), key=lambda m: m.pgn):
            for sig in msg.signals:
                lines.append(f"/* {msg.name}.{sig.name}: {sig.comment or 'No description'} */")
                lines.append(f"/* Scale: {sig.scale}, Offset: {sig.offset}, Range: [{sig.min_value}, {sig.max_value}] {sig.unit} */")
                
                # Generate extraction macro
                if sig.byte_order == 'little' and sig.bit_length <= 16:
                    start_byte = sig.start_bit // 8
                    if sig.bit_length == 8:
                        extract = f"(data[{start_byte}])"
                    elif sig.bit_length == 16:
                        extract = f"((uint16_t)data[{start_byte}] | ((uint16_t)data[{start_byte + 1}] << 8))"
                    else:
                        # Partial byte - need bit manipulation
                        bit_offset = sig.start_bit % 8
                        mask = (1 << sig.bit_length) - 1
                        extract = f"((data[{start_byte}] >> {bit_offset}) & 0x{mask:02X})"
                    
                    macro_name = f"GET_{msg.c_name}_{sig.name.upper()}"
                    if sig.needs_float:
                        lines.append(f"#define {macro_name}(data) (({extract}) * {sig.scale}f + ({sig.offset}f))")
                    else:
                        lines.append(f"#define {macro_name}(data) ({extract})")
                
                lines.append("")
        
        # Validity check macros
        lines.append("/*" + "=" * 75 + "*/")
        lines.append("/*                        VALIDITY CHECKS                                 */")
        lines.append("/*" + "=" * 75 + "*/")
        lines.append("")
        lines.append("/* J1939 'Not Available' values */")
        lines.append("#define J1939_NA_8BIT       0xFF")
        lines.append("#define J1939_NA_16BIT      0xFFFF")
        lines.append("#define J1939_NA_32BIT      0xFFFFFFFF")
        lines.append("")
        lines.append("#define IS_VALID_8(val)     ((val) != J1939_NA_8BIT)")
        lines.append("#define IS_VALID_16(val)    ((val) != J1939_NA_16BIT)")
        lines.append("#define IS_VALID_32(val)    ((val) != J1939_NA_32BIT)")
        lines.append("")
        
        # Footer
        lines.extend([
            "#ifdef __cplusplus",
            "}",
            "#endif",
            "",
            "#endif // J1939_DBC_GENERATED_H",
            ""
        ])
        
        return '\n'.join(lines)
    
    def generate_implementation(self) -> str:
        """Generate C implementation file with decoder functions"""
        lines = []
        
        lines.extend([
            "/**",
            " * @file j1939_dbc_generated.c",
            " * @brief Auto-generated J1939 decoder implementations from DBC file",
            " * ",
            " * DO NOT EDIT - Generated by dbc_parser.py",
            " */",
            "",
            '#include "j1939_dbc_generated.h"',
            "",
        ])
        
        for msg in sorted(self.db.messages.values(), key=lambda m: m.pgn):
            if not msg.signals:
                continue
            
            lines.append(f"void decode_{msg.name.lower()}(const uint8_t* data, {msg.name.lower()}_t* out) {{")
            lines.append("    if (data == NULL || out == NULL) return;")
            lines.append("")
            
            for sig in msg.signals:
                start_byte = sig.start_bit // 8
                bit_offset = sig.start_bit % 8
                
                # Generate extraction code
                if sig.byte_order == 'little':
                    if sig.bit_length == 8 and bit_offset == 0:
                        raw_expr = f"data[{start_byte}]"
                    elif sig.bit_length == 16 and bit_offset == 0:
                        raw_expr = f"((uint16_t)data[{start_byte}] | ((uint16_t)data[{start_byte + 1}] << 8))"
                    elif sig.bit_length == 32 and bit_offset == 0:
                        raw_expr = (f"((uint32_t)data[{start_byte}] | "
                                   f"((uint32_t)data[{start_byte + 1}] << 8) | "
                                   f"((uint32_t)data[{start_byte + 2}] << 16) | "
                                   f"((uint32_t)data[{start_byte + 3}] << 24))")
                    else:
                        # Handle partial bytes
                        mask = (1 << sig.bit_length) - 1
                        if sig.bit_length <= 8:
                            raw_expr = f"((data[{start_byte}] >> {bit_offset}) & 0x{mask:02X})"
                        else:
                            # Multi-byte with bit offset
                            raw_expr = f"/* TODO: complex bit extraction for {sig.name} */"
                else:
                    raw_expr = f"/* TODO: big-endian for {sig.name} */"
                
                # Apply scaling
                if sig.needs_float:
                    lines.append(f"    out->{sig.c_name} = ({raw_expr}) * {sig.scale}f + ({sig.offset}f);")
                else:
                    lines.append(f"    out->{sig.c_name} = {raw_expr};")
            
            lines.append("}")
            lines.append("")
        
        return '\n'.join(lines)


def main():
    parser = argparse.ArgumentParser(description='Parse DBC file and generate C headers')
    parser.add_argument('input', help='Input DBC file path')
    parser.add_argument('--output', '-o', default='j1939_dbc_generated.h',
                       help='Output header file path')
    parser.add_argument('--impl', '-i', default=None,
                       help='Output implementation file path (optional)')
    parser.add_argument('--summary', '-s', action='store_true',
                       help='Print summary of parsed content')
    
    args = parser.parse_args()
    
    print(f"Parsing {args.input}...")
    
    dbc_parser = DbcParser()
    db = dbc_parser.parse_file(args.input)
    
    if args.summary:
        print(f"\nDBC Summary:")
        print(f"  Version: {db.version or '(not specified)'}")
        print(f"  Nodes: {', '.join(db.nodes)}")
        print(f"  Messages: {len(db.messages)}")
        total_signals = sum(len(m.signals) for m in db.messages.values())
        print(f"  Total Signals: {total_signals}")
        print(f"\nMessages:")
        for msg in sorted(db.messages.values(), key=lambda m: m.pgn):
            print(f"  PGN {msg.pgn:5d} (0x{msg.pgn:04X}): {msg.name:<10} - {len(msg.signals)} signals, {msg.cycle_time_ms}ms cycle")
            for sig in msg.signals:
                print(f"      {sig.name:<35} [{sig.start_bit}:{sig.bit_length}] scale={sig.scale} offset={sig.offset} {sig.unit}")
    
    # Generate header
    generator = CHeaderGenerator(db)
    header_content = generator.generate()
    
    with open(args.output, 'w') as f:
        f.write(header_content)
    print(f"Generated header: {args.output}")
    
    # Generate implementation if requested
    if args.impl:
        impl_content = generator.generate_implementation()
        with open(args.impl, 'w') as f:
            f.write(impl_content)
        print(f"Generated implementation: {args.impl}")
    
    print(f"\nSuccess! Parsed {len(db.messages)} messages with {total_signals} signals.")


if __name__ == '__main__':
    main()
