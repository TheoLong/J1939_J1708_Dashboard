#!/bin/bash
# Setup script for J1939/J1708 Dashboard development environment

set -e

echo "=========================================="
echo "  J1939/J1708 Dashboard Setup Script"
echo "=========================================="
echo

# Check for Python 3
if ! command -v python3 &> /dev/null; then
    echo "ERROR: Python 3 is required but not installed."
    exit 1
fi

# Check for PlatformIO
if ! command -v pio &> /dev/null; then
    echo "PlatformIO not found. Installing..."
    pip3 install platformio
fi

echo "✓ Python 3 found"
echo "✓ PlatformIO found"

# Create test data directories
echo
echo "Creating directory structure..."
mkdir -p test_data/synthetic
mkdir -p test_data/real_logs/cummins_isx
mkdir -p test_data/real_logs/generic_j1939
mkdir -p test_data/expected_results

# Generate sample test data
echo
echo "Generating sample test data..."
cd /workspaces/J1939_J1708_Dashboard

python3 tools/test_data_generator/j1939_generator.py \
    --scenario idle --duration 60 --seed 42 \
    --format asc --output test_data/synthetic/idle_60s.asc

python3 tools/test_data_generator/j1939_generator.py \
    --scenario highway --duration 60 --seed 42 \
    --format asc --output test_data/synthetic/highway_60s.asc

python3 tools/test_data_generator/j1939_generator.py \
    --scenario cold_start --duration 120 --seed 42 \
    --format asc --output test_data/synthetic/cold_start_120s.asc

python3 tools/test_data_generator/j1939_generator.py \
    --scenario acceleration --duration 30 --seed 42 \
    --format asc --output test_data/synthetic/acceleration_30s.asc

echo "✓ Generated idle scenario (60s)"
echo "✓ Generated highway scenario (60s)"
echo "✓ Generated cold start scenario (120s)"
echo "✓ Generated acceleration scenario (30s)"

# Run parser validation
echo
echo "Running parser validation..."
python3 tools/validation/validate_parser.py

# Install PlatformIO dependencies
echo
echo "Installing PlatformIO dependencies..."
cd firmware
pio pkg install

echo
echo "=========================================="
echo "  Setup Complete!"
echo "=========================================="
echo
echo "Next steps:"
echo "  1. Open the project in VS Code with PlatformIO"
echo "  2. Build: 'pio run'"
echo "  3. Run tests: 'pio test -e native'"
echo "  4. When hardware arrives: 'pio run -t upload'"
echo
