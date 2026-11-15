#!/bin/sh
set -eu

cd "$(dirname "$0")"

if [ "$(uname -s)" != "Darwin" ]; then
    echo "This script must run on macOS with Xcode and CMake installed." >&2
    exit 1
fi

JUCE_SOURCE="${JUCE_DIR:-$PWD/external/JUCE-src}"
if [ ! -f "$JUCE_SOURCE/CMakeLists.txt" ]; then
    echo "JUCE source not found at $JUCE_SOURCE. Set JUCE_DIR to a JUCE 8.0.12 checkout." >&2
    exit 1
fi

cmake -S . -B build-macos -G Xcode \
    -DJUCE_DIR="$JUCE_SOURCE" \
    -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0 \
    '-DCMAKE_OSX_ARCHITECTURES=arm64;x86_64'
cmake --build build-macos --config Release \
    --target LowEndPilot_VST3 LowEndPilotSmoke --parallel 4
./build-macos/Release/LowEndPilotSmoke

PLUGIN='build-macos/LowEndPilot_artefacts/Release/VST3/DU - Butt-Kicker.vst3'
if [ ! -d "$PLUGIN" ]; then
    echo "Build completed but the VST3 bundle was not found at $PLUGIN" >&2
    exit 1
fi

mkdir -p dist
ditto -c -k --sequesterRsrc --keepParent "$PLUGIN" \
    'dist/DU - Butt-Kicker-macOS-universal.vst3.zip'
echo "Built dist/DU - Butt-Kicker-macOS-universal.vst3.zip"
