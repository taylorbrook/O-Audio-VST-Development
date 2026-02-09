#!/bin/bash
# Canary test script for Plugin Freedom System
# Tests O-SimpleReverb (standard plugin) and O-AnalogEQ (WebView plugin)
# Run after any system-level changes to verify no breakage

set -e

PROJ_DIR="/Users/taylorbrook/Dev/VST-development"
BUILD_DIR="$PROJ_DIR/build"

echo "=== PFS Canary Test ==="
echo "Date: $(date)"
echo ""

# Ensure build directory exists and has been configured
if [ ! -f "$BUILD_DIR/build.ninja" ]; then
  echo "ERROR: Build not configured. Run cmake first."
  exit 1
fi

cd "$BUILD_DIR"

PASS_COUNT=0
FAIL_COUNT=0

# --- Canary 1: O-SimpleReverb (standard plugin) ---
echo "--- Canary 1: O-SimpleReverb ---"
echo "Building VST3 + AU..."
if ninja O-SimpleReverb_VST3 O-SimpleReverb_AU 2>&1 | tail -5; then
  echo "Build: OK"
else
  echo "Build: FAILED"
  FAIL_COUNT=$((FAIL_COUNT + 1))
fi

echo "Clearing AU cache..."
killall -9 AudioComponentRegistrar 2>/dev/null || true
rm -rf ~/Library/Caches/AudioUnitCache/
rm -rf ~/Library/Caches/com.apple.audiounits.cache

echo "Installing..."
rm -rf ~/Library/Audio/Plug-Ins/VST3/O-SimpleReverb.vst3
rm -rf ~/Library/Audio/Plug-Ins/Components/O-SimpleReverb.component
cp -R plugins/O-SimpleReverb/O-SimpleReverb_artefacts/Release/VST3/O-SimpleReverb.vst3 ~/Library/Audio/Plug-Ins/VST3/
cp -R plugins/O-SimpleReverb/O-SimpleReverb_artefacts/Release/AU/O-SimpleReverb.component ~/Library/Audio/Plug-Ins/Components/

echo "Validating AU (aufx OuSr OuDv)..."
if auval -v aufx OuSr OuDv 2>&1 | grep -q "* * * PASS"; then
  echo "PASS: O-SimpleReverb AU validation passed"
  PASS_COUNT=$((PASS_COUNT + 1))
else
  echo "WARN: auval did not explicitly PASS -- check manually"
  FAIL_COUNT=$((FAIL_COUNT + 1))
fi

# --- Canary 2: O-AnalogEQ (WebView plugin) ---
echo ""
echo "--- Canary 2: O-AnalogEQ (WebView) ---"
echo "Building VST3 + AU..."
if ninja OuariconAnalogEQ_VST3 OuariconAnalogEQ_AU 2>&1 | tail -5; then
  echo "Build: OK"
else
  echo "Build: FAILED"
  FAIL_COUNT=$((FAIL_COUNT + 1))
fi

echo "Installing..."
killall -9 AudioComponentRegistrar 2>/dev/null || true
rm -rf ~/Library/Caches/AudioUnitCache/
rm -rf ~/Library/Caches/com.apple.audiounits.cache
rm -rf ~/Library/Audio/Plug-Ins/VST3/O-AnalogEQ.vst3
rm -rf ~/Library/Audio/Plug-Ins/Components/O-AnalogEQ.component
cp -R plugins/O-AnalogEQ/OuariconAnalogEQ_artefacts/Release/VST3/O-AnalogEQ.vst3 ~/Library/Audio/Plug-Ins/VST3/
cp -R plugins/O-AnalogEQ/OuariconAnalogEQ_artefacts/Release/AU/O-AnalogEQ.component ~/Library/Audio/Plug-Ins/Components/

echo "Validating AU (aufx OuAE OuDv)..."
if auval -v aufx OuAE OuDv 2>&1 | grep -q "* * * PASS"; then
  echo "PASS: O-AnalogEQ AU validation passed"
  PASS_COUNT=$((PASS_COUNT + 1))
else
  echo "WARN: auval did not explicitly PASS -- check manually"
  FAIL_COUNT=$((FAIL_COUNT + 1))
fi

echo ""
echo "=== Canary Test Complete ==="
echo "Passed: $PASS_COUNT / 2"
if [ $FAIL_COUNT -gt 0 ]; then
  echo "Failures: $FAIL_COUNT"
  exit 1
fi

echo "All canary tests passed."
exit 0
