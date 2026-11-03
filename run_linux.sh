#!/usr/bin/env bash
set -e

echo "========================================================"
echo " Football Match Simulator & Tracking System (Linux)"
echo "========================================================"

if [ -f "build/FootballSim" ]; then
    echo "[INFO] Running existing build..."
    ./build/FootballSim
    exit 0
fi

echo "[INFO] Building project..."
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)

echo "[INFO] Launching simulation..."
./build/FootballSim
