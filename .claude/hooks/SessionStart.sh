#!/bin/bash
# SessionStart hook - Validate development environment with proactive dependency checks
# Runs once at session start, provides warnings and actionable fixes

echo "━━━ Plugin Freedom System - Environment Validation ━━━"
echo ""

# Clean expired cache entries
if [ -f ".claude/utils/validation-cache.sh" ]; then
  source .claude/utils/validation-cache.sh
  clean_cache 2>/dev/null
fi

ERRORS=0
WARNINGS=0

# CRITICAL: Python 3 (required for validators)
if ! command -v python3 &> /dev/null; then
  echo "❌ CRITICAL: python3 not found" >&2
  echo "   Validators won't work - all plugin validation will fail" >&2
  echo "   FIX: brew install python3" >&2
  ERRORS=$((ERRORS + 1))
else
  PYTHON_VERSION=$(python3 --version 2>&1)
  echo "✓ $PYTHON_VERSION"
fi

# CRITICAL: jq (required for JSON parsing in hooks)
if ! command -v jq &> /dev/null; then
  echo "❌ CRITICAL: jq not found" >&2
  echo "   Hooks will fail - state management broken" >&2
  echo "   FIX: brew install jq" >&2
  ERRORS=$((ERRORS + 1))
else
  JQ_VERSION=$(jq --version 2>&1)
  echo "✓ $JQ_VERSION"
fi

# CRITICAL: CMake (required for builds)
if command -v cmake &> /dev/null; then
  CMAKE_VERSION=$(cmake --version | head -1)
  CMAKE_VER_NUM=$(cmake --version | head -1 | grep -oE '[0-9]+\.[0-9]+' | head -1)
  echo "✓ $CMAKE_VERSION"

  # Check minimum version (3.15+)
  CMAKE_MAJOR=$(echo "$CMAKE_VER_NUM" | cut -d. -f1)
  CMAKE_MINOR=$(echo "$CMAKE_VER_NUM" | cut -d. -f2)
  if [ "$CMAKE_MAJOR" -lt 3 ] || { [ "$CMAKE_MAJOR" -eq 3 ] && [ "$CMAKE_MINOR" -lt 15 ]; }; then
    echo "⚠️  WARNING: CMake $CMAKE_VER_NUM detected (JUCE 8 requires 3.15+)" >&2
    echo "   FIX: brew upgrade cmake" >&2
    WARNINGS=$((WARNINGS + 1))
  fi
else
  echo "❌ CRITICAL: cmake not found" >&2
  echo "   All plugin builds will fail" >&2
  echo "   FIX: brew install cmake" >&2
  ERRORS=$((ERRORS + 1))
fi

# CRITICAL: Build tools (platform-specific)
if [[ "$OSTYPE" == "darwin"* ]]; then
  # macOS: Xcode required
  if command -v xcodebuild &> /dev/null; then
    XCODE_VERSION=$(xcodebuild -version 2>&1 | head -1)
    echo "✓ $XCODE_VERSION"
  else
    echo "❌ CRITICAL: Xcode not found" >&2
    echo "   macOS builds will fail" >&2
    echo "   FIX: xcode-select --install" >&2
    ERRORS=$((ERRORS + 1))
  fi

  # Check code signing tools
  if ! command -v codesign &> /dev/null; then
    echo "❌ CRITICAL: codesign not found" >&2
    echo "   Plugins won't load in DAWs (signature validation required)" >&2
    echo "   FIX: Install Xcode Command Line Tools" >&2
    ERRORS=$((ERRORS + 1))
  else
    echo "✓ codesign available"
  fi
elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
  # Linux: gcc/g++ or clang required
  if command -v g++ &> /dev/null; then
    GCC_VERSION=$(g++ --version | head -1)
    echo "✓ $GCC_VERSION"
  elif command -v clang++ &> /dev/null; then
    CLANG_VERSION=$(clang++ --version | head -1)
    echo "✓ $CLANG_VERSION"
  else
    echo "❌ CRITICAL: No C++ compiler found (g++ or clang++)" >&2
    echo "   Builds will fail" >&2
    echo "   FIX: sudo apt install build-essential" >&2
    ERRORS=$((ERRORS + 1))
  fi
fi

# CRITICAL: JUCE (required for plugin builds)
JUCE_FOUND=0
if [ -d "/Applications/JUCE" ]; then
  echo "✓ JUCE found at /Applications/JUCE"
  JUCE_FOUND=1
elif [ -d "$HOME/JUCE" ]; then
  echo "✓ JUCE found at $HOME/JUCE"
  JUCE_FOUND=1
elif [ -d "$HOME/Developer/JUCE" ]; then
  echo "✓ JUCE found at $HOME/Developer/JUCE"
  JUCE_FOUND=1
fi

if [ $JUCE_FOUND -eq 0 ]; then
  echo "❌ CRITICAL: JUCE not found at standard locations" >&2
  echo "   All plugin builds will fail" >&2
  echo "   FIX: git clone https://github.com/juce-framework/JUCE.git ~/JUCE" >&2
  echo "        OR download from https://juce.com/" >&2
  ERRORS=$((ERRORS + 1))
fi

# HIGH PRIORITY: Git (required for version control)
if command -v git &> /dev/null; then
  GIT_VERSION=$(git --version 2>&1)
  echo "✓ $GIT_VERSION"
else
  echo "⚠️  WARNING: git not found" >&2
  echo "   Version control disabled - workflow state won't persist" >&2
  echo "   FIX: brew install git" >&2
  WARNINGS=$((WARNINGS + 1))
fi

# MEDIUM PRIORITY: Ninja (optional but recommended)
if command -v ninja &> /dev/null; then
  NINJA_VERSION=$(ninja --version 2>&1)
  echo "✓ ninja $NINJA_VERSION (fast builds enabled)"
else
  echo "ℹ️  INFO: ninja not found (builds will use default generator)" >&2
  echo "   FIX (optional): brew install ninja" >&2
fi

# LOW PRIORITY: pluginval (optional for validation testing)
if command -v pluginval &> /dev/null; then
  echo "✓ pluginval available (validation testing enabled)"
elif [ -f "$HOME/pluginval" ]; then
  echo "✓ pluginval found at $HOME/pluginval"
elif [ -f "/Applications/pluginval.app/Contents/MacOS/pluginval" ]; then
  echo "✓ pluginval found in /Applications"
else
  echo "ℹ️  INFO: pluginval not found (validation testing unavailable)" >&2
  echo "   FIX (optional): Download from https://github.com/Tracktion/pluginval/releases" >&2
fi

echo ""
echo "━━━ Validation Summary ━━━"
if [ $ERRORS -gt 0 ]; then
  echo "❌ $ERRORS critical error(s) found"
  echo "   Workflow will fail - fix critical issues before running /plan or /implement"
  echo ""
  echo "Quick fix command:"
  if [[ "$OSTYPE" == "darwin"* ]]; then
    echo "  brew install python3 jq cmake && xcode-select --install"
  else
    echo "  sudo apt install python3 jq cmake build-essential"
  fi
  echo ""
  echo "Session will continue, but workflows will fail until dependencies are installed"
fi

if [ $WARNINGS -gt 0 ]; then
  echo "⚠️  $WARNINGS warning(s) - system usable but degraded functionality"
fi

if [ $ERRORS -eq 0 ] && [ $WARNINGS -eq 0 ]; then
  echo "✅ All dependencies validated - system ready"
fi

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

# Never block session start (allow user to see errors and fix)
exit 0
