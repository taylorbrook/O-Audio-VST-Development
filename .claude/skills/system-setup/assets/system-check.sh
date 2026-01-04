#!/bin/bash

# system-check.sh - Automated dependency validation for Plugin Freedom System
# Returns JSON output for each check
#
# Usage:
#   ./system-check.sh                    # Normal mode - detect actual system
#   ./system-check.sh --test=SCENARIO    # Test mode - return mock data
#
# Test scenarios:
#   fresh-system    - Nothing installed
#   missing-juce    - Everything except JUCE
#   old-versions    - All installed but below minimums
#   custom-paths    - JUCE in non-standard location
#   partial-python  - Missing Python only

set -e

# Check for test mode
TEST_MODE=""
if [[ "$1" == --test=* ]]; then
    TEST_MODE="${1#--test=}"
fi

# Colors for output (when not outputting JSON)
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Helper function to output JSON
json_output() {
    echo "$1"
}

# Helper function to check version comparison
version_compare() {
    # Returns 0 if $1 >= $2, 1 otherwise
    # Uses sort -V for version comparison
    local version1="$1"
    local version2="$2"

    if [ "$(printf '%s\n' "$version2" "$version1" | sort -V | head -n1)" = "$version2" ]; then
        return 0  # version1 >= version2
    else
        return 1  # version1 < version2
    fi
}

# Detect platform
detect_platform() {
    local platform=""
    local platform_version=""
    local arch=""

    if [[ "$OSTYPE" == "darwin"* ]]; then
        platform="darwin"
        platform_version=$(sw_vers -productVersion)
        arch=$(uname -m)
    elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
        platform="linux"
        platform_version=$(lsb_release -rs 2>/dev/null || echo "unknown")
        arch=$(uname -m)
    elif [[ "$OSTYPE" == "msys" || "$OSTYPE" == "cygwin" ]]; then
        platform="windows"
        platform_version=$(cmd.exe /c ver | grep -oP '\d+\.\d+\.\d+')
        arch="x64"
    else
        platform="unknown"
        platform_version="unknown"
        arch=$(uname -m)
    fi

    json_output "{\"platform\":\"$platform\",\"platform_version\":\"$platform_version\",\"arch\":\"$arch\"}"
}

# Check Python
check_python() {
    local min_version="3.8"

    if command -v python3 &> /dev/null; then
        local version=$(python3 --version 2>&1 | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' | head -n1 || echo "0.0.0")
        local path=$(which python3)

        if version_compare "$version" "$min_version"; then
            json_output "{\"found\":true,\"version\":\"$version\",\"path\":\"$path\",\"meets_minimum\":true}"
        else
            json_output "{\"found\":true,\"version\":\"$version\",\"path\":\"$path\",\"meets_minimum\":false,\"required\":\"$min_version\"}"
        fi
    else
        json_output "{\"found\":false,\"required\":\"$min_version\"}"
    fi
}

# Check Xcode Command Line Tools (macOS only)
check_xcode() {
    if [[ "$OSTYPE" != "darwin"* ]]; then
        json_output "{\"platform\":\"not macOS\",\"found\":false}"
        return
    fi

    if xcode-select -p &> /dev/null; then
        local path=$(xcode-select -p)
        local version=$(pkgutil --pkg-info=com.apple.pkg.CLTools_Executables 2>/dev/null | grep version | awk '{print $2}' || echo "unknown")
        json_output "{\"found\":true,\"path\":\"$path\",\"version\":\"$version\"}"
    else
        json_output "{\"found\":false}"
    fi
}

# Check CMake
check_cmake() {
    local min_version="3.15"

    if command -v cmake &> /dev/null; then
        local version=$(cmake --version 2>&1 | head -n1 | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' | head -n1 || echo "0.0.0")
        local path=$(which cmake)

        if version_compare "$version" "$min_version"; then
            json_output "{\"found\":true,\"version\":\"$version\",\"path\":\"$path\",\"meets_minimum\":true}"
        else
            json_output "{\"found\":true,\"version\":\"$version\",\"path\":\"$path\",\"meets_minimum\":false,\"required\":\"$min_version\"}"
        fi
    else
        json_output "{\"found\":false,\"required\":\"$min_version\"}"
    fi
}

# Check JUCE installation
check_juce() {
    local min_major=8
    local min_minor=0

    # Standard search paths
    local search_paths=(
        "$HOME/JUCE"
        "/Applications/JUCE"
        "/usr/local/JUCE"
        "C:/JUCE"
    )

    local found_path=""

    # Search standard locations
    for path in "${search_paths[@]}"; do
        if [ -f "$path/modules/juce_core/juce_core.h" ]; then
            found_path="$path"
            break
        fi
    done

    if [ -z "$found_path" ]; then
        # Convert array to JSON array string
        local checked_json="["
        for ((i=0; i<${#search_paths[@]}; i++)); do
            checked_json+="\"${search_paths[$i]}\""
            if [ $i -lt $((${#search_paths[@]} - 1)) ]; then
                checked_json+=","
            fi
        done
        checked_json+="]"

        json_output "{\"found\":false,\"checked_paths\":$checked_json}"
        return
    fi

    # Extract version from juce_StandardHeader.h (JUCE 8+) or juce_core.h (JUCE 7)
    local header_file="$found_path/modules/juce_core/system/juce_StandardHeader.h"
    if [ ! -f "$header_file" ]; then
        header_file="$found_path/modules/juce_core/juce_core.h"
    fi

    local major=$(grep "define JUCE_MAJOR_VERSION" "$header_file" | grep -oE '[0-9]+' | head -n1 || echo "0")
    local minor=$(grep "define JUCE_MINOR_VERSION" "$header_file" | grep -oE '[0-9]+' | head -n1 || echo "0")
    local patch=$(grep "define JUCE_BUILDNUMBER" "$header_file" | grep -oE '[0-9]+' | head -n1 || echo "0")

    # Default to 0 if empty
    major=${major:-0}
    minor=${minor:-0}
    patch=${patch:-0}

    local version="${major}.${minor}.${patch}"

    # Check if version meets minimum
    if [ "$major" -ge "$min_major" ] 2>/dev/null; then
        if [ "$major" -eq "$min_major" ]; then
            if [ "$minor" -ge "$min_minor" ]; then
                json_output "{\"found\":true,\"path\":\"$found_path\",\"version\":\"$version\",\"meets_minimum\":true}"
            else
                json_output "{\"found\":true,\"path\":\"$found_path\",\"version\":\"$version\",\"meets_minimum\":false,\"required\":\"${min_major}.${min_minor}.0\"}"
            fi
        else
            json_output "{\"found\":true,\"path\":\"$found_path\",\"version\":\"$version\",\"meets_minimum\":true}"
        fi
    else
        json_output "{\"found\":true,\"path\":\"$found_path\",\"version\":\"$version\",\"meets_minimum\":false,\"required\":\"${min_major}.${min_minor}.0\"}"
    fi
}

# Validate custom JUCE path
validate_juce_path() {
    local custom_path="$1"

    if [ -z "$custom_path" ]; then
        json_output "{\"valid\":false,\"error\":\"No path provided\"}"
        return
    fi

    if [ ! -d "$custom_path" ]; then
        json_output "{\"valid\":false,\"error\":\"Path does not exist\"}"
        return
    fi

    # Check for JUCE 8+ location first
    local header_file="$custom_path/modules/juce_core/system/juce_StandardHeader.h"
    if [ ! -f "$header_file" ]; then
        # Fall back to JUCE 7 location
        header_file="$custom_path/modules/juce_core/juce_core.h"
        if [ ! -f "$header_file" ]; then
            json_output "{\"valid\":false,\"error\":\"Not a valid JUCE installation (missing version header)\"}"
            return
        fi
    fi

    # Extract version
    local major=$(grep "define JUCE_MAJOR_VERSION" "$header_file" | grep -oE '[0-9]+' | head -n1 || echo "0")
    local minor=$(grep "define JUCE_MINOR_VERSION" "$header_file" | grep -oE '[0-9]+' | head -n1 || echo "0")
    local patch=$(grep "define JUCE_BUILDNUMBER" "$header_file" | grep -oE '[0-9]+' | head -n1 || echo "0")

    # Default to 0 if empty
    major=${major:-0}
    minor=${minor:-0}
    patch=${patch:-0}

    local version="${major}.${minor}.${patch}"

    # Check minimum version
    local min_major=8
    if [ "$major" -ge "$min_major" ] 2>/dev/null; then
        json_output "{\"valid\":true,\"path\":\"$custom_path\",\"version\":\"$version\"}"
    else
        json_output "{\"valid\":false,\"error\":\"JUCE version $version found, but 8.0.0+ required\",\"version\":\"$version\"}"
    fi
}

# Check pluginval
check_pluginval() {
    # First check if in PATH
    if command -v pluginval &> /dev/null; then
        local path=$(which pluginval)
        local version=$(pluginval --version 2>&1 | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' | head -n1 || echo "unknown")
        json_output "{\"found\":true,\"version\":\"$version\",\"path\":\"$path\"}"
        return
    fi

    # Check standard locations
    local search_paths=(
        "/usr/local/bin/pluginval"
        "$HOME/bin/pluginval"
        "/Applications/pluginval.app/Contents/MacOS/pluginval"
        "C:/Program Files/pluginval/pluginval.exe"
    )

    for path in "${search_paths[@]}"; do
        if [ -f "$path" ] && [ -x "$path" ]; then
            local version=$("$path" --version 2>&1 | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' | head -n1 || echo "unknown")
            json_output "{\"found\":true,\"version\":\"$version\",\"path\":\"$path\"}"
            return
        fi
    done

    json_output "{\"found\":false}"
}

# Check all dependencies
check_all() {
    echo "{"
    echo "  \"platform\": $(detect_platform),"
    echo "  \"python\": $(check_python),"
    echo "  \"build_tools\": $(check_xcode),"
    echo "  \"cmake\": $(check_cmake),"
    echo "  \"juce\": $(check_juce),"
    echo "  \"pluginval\": $(check_pluginval)"
    echo "}"
}

# Test mode mock data
return_test_data() {
    local scenario="$1"

    case "$scenario" in
        fresh-system)
            cat << 'EOF'
{
  "platform": "darwin 15.6.1 (arm64)",
  "python": {
    "installed": false,
    "version": null,
    "path": null,
    "meets_minimum": false,
    "minimum_required": "3.8.0"
  },
  "build_tools": {
    "installed": false,
    "version": null,
    "path": null
  },
  "cmake": {
    "installed": false,
    "version": null,
    "path": null,
    "meets_minimum": false,
    "minimum_required": "3.15.0"
  },
  "juce": {
    "installed": false,
    "version": null,
    "path": null,
    "meets_minimum": false,
    "minimum_required": "8.0.0"
  },
  "pluginval": {
    "installed": false,
    "path": null
  }
}
EOF
            ;;
        missing-juce)
            cat << 'EOF'
{
  "platform": "darwin 15.6.1 (arm64)",
  "python": {
    "installed": true,
    "version": "3.11.5",
    "path": "/usr/local/bin/python3",
    "meets_minimum": true,
    "minimum_required": "3.8.0"
  },
  "build_tools": {
    "installed": true,
    "version": "16.0.0",
    "path": "/Library/Developer/CommandLineTools"
  },
  "cmake": {
    "installed": true,
    "version": "3.27.4",
    "path": "/usr/local/bin/cmake",
    "meets_minimum": true,
    "minimum_required": "3.15.0"
  },
  "juce": {
    "installed": false,
    "version": null,
    "path": null,
    "meets_minimum": false,
    "minimum_required": "8.0.0"
  },
  "pluginval": {
    "installed": true,
    "path": "/usr/local/bin/pluginval"
  }
}
EOF
            ;;
        old-versions)
            cat << 'EOF'
{
  "platform": "darwin 15.6.1 (arm64)",
  "python": {
    "installed": true,
    "version": "3.7.0",
    "path": "/usr/bin/python3",
    "meets_minimum": false,
    "minimum_required": "3.8.0"
  },
  "build_tools": {
    "installed": true,
    "version": "12.0.0",
    "path": "/Library/Developer/CommandLineTools"
  },
  "cmake": {
    "installed": true,
    "version": "3.10.0",
    "path": "/usr/local/bin/cmake",
    "meets_minimum": false,
    "minimum_required": "3.15.0"
  },
  "juce": {
    "installed": true,
    "version": "7.0.5",
    "path": "/Applications/JUCE",
    "meets_minimum": false,
    "minimum_required": "8.0.0"
  },
  "pluginval": {
    "installed": false,
    "path": null
  }
}
EOF
            ;;
        custom-paths)
            cat << 'EOF'
{
  "platform": "darwin 15.6.1 (arm64)",
  "python": {
    "installed": true,
    "version": "3.11.5",
    "path": "/usr/local/bin/python3",
    "meets_minimum": true,
    "minimum_required": "3.8.0"
  },
  "build_tools": {
    "installed": true,
    "version": "16.0.0",
    "path": "/Library/Developer/CommandLineTools"
  },
  "cmake": {
    "installed": true,
    "version": "3.27.4",
    "path": "/usr/local/bin/cmake",
    "meets_minimum": true,
    "minimum_required": "3.15.0"
  },
  "juce": {
    "installed": true,
    "version": "8.0.3",
    "path": "/Users/testuser/Development/Libraries/JUCE",
    "meets_minimum": true,
    "minimum_required": "8.0.0"
  },
  "pluginval": {
    "installed": true,
    "path": "/Users/testuser/bin/pluginval"
  }
}
EOF
            ;;
        partial-python)
            cat << 'EOF'
{
  "platform": "darwin 15.6.1 (arm64)",
  "python": {
    "installed": false,
    "version": null,
    "path": null,
    "meets_minimum": false,
    "minimum_required": "3.8.0"
  },
  "build_tools": {
    "installed": true,
    "version": "16.0.0",
    "path": "/Library/Developer/CommandLineTools"
  },
  "cmake": {
    "installed": true,
    "version": "3.27.4",
    "path": "/usr/local/bin/cmake",
    "meets_minimum": true,
    "minimum_required": "3.15.0"
  },
  "juce": {
    "installed": true,
    "version": "8.0.3",
    "path": "/Applications/JUCE",
    "meets_minimum": true,
    "minimum_required": "8.0.0"
  },
  "pluginval": {
    "installed": true,
    "path": "/usr/local/bin/pluginval"
  }
}
EOF
            ;;
        *)
            echo "{\"error\": \"Unknown test scenario: $scenario\"}"
            exit 1
            ;;
    esac
}

# Main command dispatcher
# Handle test mode first - need to check both first and last arg
TEST_ARG=""
for arg in "$@"; do
    if [[ "$arg" == --test=* ]]; then
        TEST_ARG="${arg#--test=}"
        break
    fi
done

if [[ -n "$TEST_ARG" ]]; then
    # In test mode, return appropriate mock data based on command
    case "${1:-}" in
        --detect-platform)
            echo '{"platform":"darwin","platform_version":"15.6.1","arch":"arm64"}'
            ;;
        --check-python)
            echo '{"found":true,"version":"3.11.5","path":"/usr/local/bin/python3","meets_minimum":true}'
            ;;
        --check-xcode)
            echo '{"found":true,"path":"/Library/Developer/CommandLineTools","version":"16.0.0"}'
            ;;
        --check-cmake)
            echo '{"found":true,"version":"3.27.4","path":"/usr/local/bin/cmake","meets_minimum":true}'
            ;;
        --check-juce)
            # This is where test scenarios differ
            case "$TEST_ARG" in
                missing-juce)
                    echo '{"found":false,"checked_paths":["/Users/'$USER'/JUCE","/Applications/JUCE","/usr/local/JUCE"]}'
                    ;;
                old-versions)
                    echo '{"found":true,"path":"/Applications/JUCE","version":"7.0.5","meets_minimum":false,"required":"8.0.0"}'
                    ;;
                custom-paths)
                    echo '{"found":false,"checked_paths":["/Users/'$USER'/JUCE","/Applications/JUCE","/usr/local/JUCE"]}'
                    ;;
                fresh-system)
                    echo '{"found":false,"checked_paths":["/Users/'$USER'/JUCE","/Applications/JUCE","/usr/local/JUCE"]}'
                    ;;
                partial-python)
                    echo '{"found":true,"path":"/Applications/JUCE","version":"8.0.3","meets_minimum":true}'
                    ;;
                *)
                    echo '{"found":true,"path":"/Applications/JUCE","version":"8.0.3","meets_minimum":true}'
                    ;;
            esac
            ;;
        --check-pluginval)
            echo '{"found":true,"version":"1.0.3","path":"/usr/local/bin/pluginval"}'
            ;;
        --validate-juce-path)
            # For test mode, validate the path was provided but return success
            if [ -n "$2" ]; then
                echo '{"valid":true,"path":"'"$2"'","version":"8.0.3"}'
            else
                echo '{"valid":false,"error":"No path provided"}'
            fi
            ;;
        *)
            return_test_data "$TEST_ARG"
            ;;
    esac
    exit 0
fi

case "${1:-}" in
    --detect-platform)
        detect_platform
        ;;
    --check-python)
        check_python
        ;;
    --check-xcode)
        check_xcode
        ;;
    --check-cmake)
        check_cmake
        ;;
    --check-juce)
        check_juce
        ;;
    --validate-juce-path)
        validate_juce_path "$2"
        ;;
    --check-pluginval)
        check_pluginval
        ;;
    --check-all)
        check_all
        ;;
    --help)
        cat << EOF
Usage: system-check.sh [COMMAND] [ARGS]

Commands:
  --detect-platform           Detect OS platform and architecture
  --check-python             Check for Python 3.8+
  --check-xcode              Check for Xcode Command Line Tools (macOS)
  --check-cmake              Check for CMake 3.15+
  --check-juce               Check for JUCE 8.0+ in standard locations
  --validate-juce-path PATH  Validate custom JUCE installation path
  --check-pluginval          Check for pluginval
  --check-all                Run all checks and output comprehensive JSON

Test Mode:
  --test=SCENARIO            Return mock data for testing

  Available scenarios:
    fresh-system    - Nothing installed
    missing-juce    - Everything except JUCE
    old-versions    - All installed but below minimums
    custom-paths    - JUCE in non-standard location
    partial-python  - Missing Python only
  --help                     Show this help message

All commands output JSON for programmatic parsing.

Examples:
  ./system-check.sh --detect-platform
  ./system-check.sh --check-python
  ./system-check.sh --validate-juce-path /Users/lex/Development/JUCE
  ./system-check.sh --check-all
EOF
        ;;
    *)
        echo "Error: Unknown command '${1:-}'" >&2
        echo "Run with --help for usage information" >&2
        exit 1
        ;;
esac
