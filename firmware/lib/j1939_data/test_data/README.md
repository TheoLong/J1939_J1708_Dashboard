# Test Data Documentation

This directory contains test data for validating the J1939 parser implementation.

## File Naming Convention

- Files ending in `_generated.*` are synthetic data created for testing purposes. Do not treat these as real world logs.
- Files WITHOUT `_generated` (e.g., `truck_capture.log`) are sourced from real hardware or web resources.

## Current Files

### `truck_sample_generated.log`
**Source:** Synthetic / Generated
**Format:** candump -L (Timestamp interface CAN_ID#DATA)
**Description:**
A generated sequence simulates key J1939 messages to verify parsing logic.
Contains:
- EEC1 (PGN 61444): Engine Speed, Torque
- ET1 (PGN 65262): Engine Coolant Temp
- CCVS (PGN 65265): Vehicle Speed

### `truck_sample_generated.asc`
**Source:** Synthetic / Generated
**Format:** Vector ASC
**Description:**
Same data as the `.log` file but in Vector ASC format, suitable for viewing in CANalyzer or other tools.

### `truck_sample_decoded_generated.csv`
**Source:** Generated from `truck_sample_generated.log`
**Description:**
Decoded values corresponding to the log file. Used as the "Truth" for unit tests to verify the parser output against expected physical values.
Columns:
- Timestamp: Log timestamp
- PGN: Parameter Group Number
- SPN: Suspect Parameter Number
- Value: Physical value (float)
- Unit: Unit of measurement
