#!/bin/bash
set -e

# ============================================================================
# build-and-install.sh - JUCE Plugin Build & Installation Script
# ============================================================================
# 7-Phase Pipeline:
#   1. Pre-flight Validation
#   2. Parallel Build (VST3 + AU)
#   3. Extract PRODUCT_NAME
#   4. Remove Old Versions
#   5. Install New Versions
#   6. Clear DAW Caches
#   7. Verification
# ============================================================================

# Color output functions
success() {
    echo -e "\033[0;32m✓ $1\033[0m"
}

warning() {
    echo -e "\033[0;33m⚠ $1\033[0m"
}

error() {
    echo -e "\033[0;31m✗ $1\033[0m" >&2
}

info() {
    echo -e "\033[0;36m→ $1\033[0m"
}

# Global variables
PLUGIN_NAME=""
PRODUCT_NAME=""
DRY_RUN=false
NO_INSTALL=false
VERBOSE=false
RECONFIGURE=false
LOG_FILE=""
START_TIME=$(date +%s)

# Parse arguments
parse_args() {
    if [ $# -eq 0 ]; then
        echo "Usage: $0 <PluginName> [--dry-run] [--no-install] [--verbose] [--reconfigure]"
        echo ""
        echo "Arguments:"
        echo "  <PluginName>   Name of plugin directory in plugins/"
        echo "  --dry-run      Show commands without executing"
        echo "  --no-install   Build only, skip installation"
        echo "  --verbose      Show detailed output"
        echo "  --reconfigure  Force CMake reconfiguration (delete build/ first)"
        exit 1
    fi

    PLUGIN_NAME="$1"
    shift

    while [ $# -gt 0 ]; do
        case "$1" in
            --dry-run)
                DRY_RUN=true
                ;;
            --no-install)
                NO_INSTALL=true
                ;;
            --verbose)
                VERBOSE=true
                ;;
            --reconfigure)
                RECONFIGURE=true
                ;;
            *)
                error "Unknown flag: $1"
                exit 1
                ;;
        esac
        shift
    done
}

# Setup logging
setup_logging() {
    local log_dir="logs/$PLUGIN_NAME"
    mkdir -p "$log_dir"
    LOG_FILE="$log_dir/build_$(date +%Y%m%d_%H%M%S).log"

    info "Log file: $LOG_FILE"
    echo "Build started at $(date)" > "$LOG_FILE"
    echo "Plugin: $PLUGIN_NAME" >> "$LOG_FILE"
    echo "Flags: DRY_RUN=$DRY_RUN NO_INSTALL=$NO_INSTALL VERBOSE=$VERBOSE RECONFIGURE=$RECONFIGURE" >> "$LOG_FILE"
    echo "---" >> "$LOG_FILE"
}

# Execute command (respects dry-run flag)
execute() {
    local cmd="$*"

    if [ "$VERBOSE" = true ]; then
        info "Execute: $cmd"
    fi

    echo "$ $cmd" >> "$LOG_FILE"

    if [ "$DRY_RUN" = true ]; then
        echo "[DRY-RUN] $cmd"
        return 0
    fi

    if [ "$VERBOSE" = true ]; then
        eval "$cmd" 2>&1 | tee -a "$LOG_FILE"
    else
        eval "$cmd" >> "$LOG_FILE" 2>&1
    fi

    return ${PIPESTATUS[0]}
}

# ============================================================================
# Phase 1: Pre-flight Validation
# ============================================================================
phase_1_preflight_validation() {
    echo ""
    info "Phase 1: Pre-flight Validation"
    echo "Phase 1: Pre-flight Validation" >> "$LOG_FILE"

    # Check plugin directory exists
    info "  - Checking plugin directory..."
    if [ ! -d "plugins/$PLUGIN_NAME" ]; then
        error "Plugin directory not found: plugins/$PLUGIN_NAME"
        echo "ERROR: Plugin directory not found" >> "$LOG_FILE"
        exit 1
    fi

    # Check CMakeLists.txt exists
    info "  - Checking CMakeLists.txt..."
    if [ ! -f "plugins/$PLUGIN_NAME/CMakeLists.txt" ]; then
        error "CMakeLists.txt not found in plugins/$PLUGIN_NAME/"
        echo "ERROR: CMakeLists.txt not found" >> "$LOG_FILE"
        exit 1
    fi

    # Check PRODUCT_NAME in CMakeLists.txt
    info "  - Checking PRODUCT_NAME..."
    if ! grep -q "PRODUCT_NAME" "plugins/$PLUGIN_NAME/CMakeLists.txt"; then
        warning "PRODUCT_NAME not found in CMakeLists.txt (will use directory name as fallback)"
        echo "WARNING: PRODUCT_NAME not found" >> "$LOG_FILE"
    fi

    # Check CMake available
    info "  - Checking CMake..."
    if ! command -v cmake &> /dev/null; then
        error "CMake not found. Install with: brew install cmake"
        echo "ERROR: CMake not found" >> "$LOG_FILE"
        exit 1
    fi

    # Check Ninja available
    info "  - Checking Ninja..."
    if ! command -v ninja &> /dev/null; then
        error "Ninja not found. Install with: brew install ninja"
        echo "ERROR: Ninja not found" >> "$LOG_FILE"
        exit 1
    fi

    # Optional: Check JUCE (usually found via CMake)
    if [ -n "$JUCE_DIR" ]; then
        info "  - Using JUCE_DIR: $JUCE_DIR"
        echo "JUCE_DIR: $JUCE_DIR" >> "$LOG_FILE"
    fi

    success "Pre-flight validation passed"
    echo "Pre-flight validation: PASS" >> "$LOG_FILE"
}

# ============================================================================
# Phase 2: Parallel Build
# ============================================================================
phase_2_build() {
    echo ""
    info "Phase 2: Parallel Build"
    echo "Phase 2: Parallel Build" >> "$LOG_FILE"

    local build_dir="build"

    # Handle --reconfigure flag
    if [ "$RECONFIGURE" = true ] && [ -d "$build_dir" ]; then
        info "  - Removing build directory for reconfiguration..."
        if [ "$DRY_RUN" = true ]; then
            echo "[DRY-RUN] rm -rf \"$build_dir\""
        else
            rm -rf "$build_dir"
            echo "Removed build directory for reconfiguration" >> "$LOG_FILE"
        fi
    fi

    # One-time configure at root (only if build/ doesn't exist)
    if [ ! -d "$build_dir" ]; then
        info "  - Configuring CMake at root with Ninja generator..."
        if ! execute cmake -B "$build_dir" -G Ninja -DCMAKE_BUILD_TYPE=Release; then
            error "CMake configuration failed"
            echo "ERROR: CMake configuration failed" >> "$LOG_FILE"
            exit 1
        fi
    else
        info "  - Build directory exists, skipping configure (use --reconfigure to force)"
    fi

    # Build specific plugin using --target flags
    info "  - Building ${PLUGIN_NAME} (VST3 + AU) in parallel..."
    if ! execute cmake --build "$build_dir" --config Release --target "${PLUGIN_NAME}_VST3" --target "${PLUGIN_NAME}_AU" --parallel; then
        error "Build failed"
        echo "ERROR: Build failed" >> "$LOG_FILE"
        exit 1
    fi

    success "Build complete"
    echo "Build: SUCCESS" >> "$LOG_FILE"
}

# ============================================================================
# Phase 3: Extract PRODUCT_NAME
# ============================================================================
phase_3_extract_product_name() {
    echo ""
    info "Phase 3: Extract PRODUCT_NAME"
    echo "Phase 3: Extract PRODUCT_NAME" >> "$LOG_FILE"

    # Parse PRODUCT_NAME from CMakeLists.txt
    info "  - Parsing CMakeLists.txt..."
    PRODUCT_NAME=$(grep 'PRODUCT_NAME' "plugins/$PLUGIN_NAME/CMakeLists.txt" | \
                   sed -E 's/.*PRODUCT_NAME[[:space:]]+"([^"]+)".*/\1/' | \
                   head -1)

    # Validate extraction succeeded
    if [ -z "$PRODUCT_NAME" ]; then
        warning "Could not extract PRODUCT_NAME, using directory name as fallback"
        PRODUCT_NAME="$PLUGIN_NAME"
        echo "WARNING: Using fallback PRODUCT_NAME=$PRODUCT_NAME" >> "$LOG_FILE"
    else
        info "  - Product name: $PRODUCT_NAME"
        echo "PRODUCT_NAME: $PRODUCT_NAME" >> "$LOG_FILE"
    fi

    success "Product name extracted: $PRODUCT_NAME"
}

# ============================================================================
# Phase 4: Remove Old Versions
# ============================================================================
phase_4_remove_old_versions() {
    echo ""
    info "Phase 4: Remove Old Versions"
    echo "Phase 4: Remove Old Versions" >> "$LOG_FILE"

    local vst3_dir="$HOME/Library/Audio/Plug-Ins/VST3"
    local au_dir="$HOME/Library/Audio/Plug-Ins/Components"

    # Search for existing VST3
    info "  - Searching for old VST3..."
    if [ -d "$vst3_dir/$PRODUCT_NAME.vst3" ]; then
        if [ "$DRY_RUN" = true ]; then
            echo "[DRY-RUN] Would remove: $vst3_dir/$PRODUCT_NAME.vst3"
        else
            info "  - Removing old VST3: $vst3_dir/$PRODUCT_NAME.vst3"
            rm -rf "$vst3_dir/$PRODUCT_NAME.vst3"
            echo "Removed old VST3: $vst3_dir/$PRODUCT_NAME.vst3" >> "$LOG_FILE"
        fi
    else
        info "  - No old VST3 found"
        echo "No old VST3 found" >> "$LOG_FILE"
    fi

    # Search for existing AU
    info "  - Searching for old AU..."
    if [ -d "$au_dir/$PRODUCT_NAME.component" ]; then
        if [ "$DRY_RUN" = true ]; then
            echo "[DRY-RUN] Would remove: $au_dir/$PRODUCT_NAME.component"
        else
            info "  - Removing old AU: $au_dir/$PRODUCT_NAME.component"
            rm -rf "$au_dir/$PRODUCT_NAME.component"
            echo "Removed old AU: $au_dir/$PRODUCT_NAME.component" >> "$LOG_FILE"
        fi
    else
        info "  - No old AU found"
        echo "No old AU found" >> "$LOG_FILE"
    fi

    success "Old versions removed"
}

# ============================================================================
# Phase 5: Install New Versions
# ============================================================================
phase_5_install_new_versions() {
    echo ""
    info "Phase 5: Install New Versions"
    echo "Phase 5: Install New Versions" >> "$LOG_FILE"

    local vst3_dir="$HOME/Library/Audio/Plug-Ins/VST3"
    local au_dir="$HOME/Library/Audio/Plug-Ins/Components"
    local vst3_build="build/plugins/$PLUGIN_NAME/${PLUGIN_NAME}_artefacts/Release/VST3/$PRODUCT_NAME.vst3"
    local au_build="build/plugins/$PLUGIN_NAME/${PLUGIN_NAME}_artefacts/Release/AU/$PRODUCT_NAME.component"

    # Verify VST3 artifact exists (skip in dry-run)
    info "  - Locating VST3 build artifact..."
    if [ "$DRY_RUN" = false ] && [ ! -d "$vst3_build" ]; then
        error "VST3 build artifact not found: $vst3_build"
        echo "ERROR: VST3 artifact not found" >> "$LOG_FILE"
        exit 1
    fi

    # Verify AU artifact exists (skip in dry-run)
    info "  - Locating AU build artifact..."
    if [ "$DRY_RUN" = false ] && [ ! -d "$au_build" ]; then
        error "AU build artifact not found: $au_build"
        echo "ERROR: AU artifact not found" >> "$LOG_FILE"
        exit 1
    fi

    # Install VST3
    info "  - Installing VST3 to $vst3_dir/"
    if [ "$DRY_RUN" = true ]; then
        echo "[DRY-RUN] cp -R \"$vst3_build\" \"$vst3_dir/\""
        echo "[DRY-RUN] chmod -R 755 \"$vst3_dir/$PRODUCT_NAME.vst3\""
    else
        cp -R "$vst3_build" "$vst3_dir/"
        chmod -R 755 "$vst3_dir/$PRODUCT_NAME.vst3"
        echo "Installed VST3: $vst3_dir/$PRODUCT_NAME.vst3" >> "$LOG_FILE"
    fi

    # Install AU
    info "  - Installing AU to $au_dir/"
    if [ "$DRY_RUN" = true ]; then
        echo "[DRY-RUN] cp -R \"$au_build\" \"$au_dir/\""
        echo "[DRY-RUN] chmod -R 755 \"$au_dir/$PRODUCT_NAME.component\""
    else
        cp -R "$au_build" "$au_dir/"
        chmod -R 755 "$au_dir/$PRODUCT_NAME.component"
        echo "Installed AU: $au_dir/$PRODUCT_NAME.component" >> "$LOG_FILE"
    fi

    success "New versions installed"
}

# ============================================================================
# Phase 6: Clear DAW Caches
# ============================================================================
phase_6_clear_daw_caches() {
    echo ""
    info "Phase 6: Clear DAW Caches"
    echo "Phase 6: Clear DAW Caches" >> "$LOG_FILE"

    local ableton_prefs="$HOME/Library/Preferences/Ableton"
    local au_cache="$HOME/Library/Caches/AudioUnitCache"

    # Clear Ableton plugin database (forces rescan)
    info "  - Clearing Ableton plugin database..."
    if [ "$DRY_RUN" = true ]; then
        echo "[DRY-RUN] rm -f \"$ableton_prefs\"/*/PluginScanDb.txt"
    else
        if [ -d "$ableton_prefs" ]; then
            rm -f "$ableton_prefs"/*/PluginScanDb.txt 2>/dev/null || true
            echo "Cleared Ableton plugin database" >> "$LOG_FILE"
        else
            info "  - Ableton preferences directory not found (skipping)"
            echo "Ableton preferences not found" >> "$LOG_FILE"
        fi
    fi

    # Clear Audio Unit cache
    info "  - Clearing Audio Unit cache..."
    if [ "$DRY_RUN" = true ]; then
        echo "[DRY-RUN] rm -rf \"$au_cache\"/*"
    else
        if [ -d "$au_cache" ]; then
            rm -rf "$au_cache"/* 2>/dev/null || true
            echo "Cleared AU cache" >> "$LOG_FILE"
        else
            info "  - AU cache directory not found (skipping)"
            echo "AU cache not found" >> "$LOG_FILE"
        fi
    fi

    # Kill AudioComponentRegistrar process
    info "  - Killing AudioComponentRegistrar..."
    if [ "$DRY_RUN" = true ]; then
        echo "[DRY-RUN] killall -9 AudioComponentRegistrar 2>/dev/null || true"
    else
        killall -9 AudioComponentRegistrar 2>/dev/null || true
        echo "Killed AudioComponentRegistrar" >> "$LOG_FILE"
    fi

    success "DAW caches cleared"
}

# ============================================================================
# Phase 7: Verification
# ============================================================================
phase_7_verification() {
    echo ""
    info "Phase 7: Verification"
    echo "Phase 7: Verification" >> "$LOG_FILE"

    local vst3_dir="$HOME/Library/Audio/Plug-Ins/VST3"
    local au_dir="$HOME/Library/Audio/Plug-Ins/Components"
    local vst3_path="$vst3_dir/$PRODUCT_NAME.vst3"
    local au_path="$au_dir/$PRODUCT_NAME.component"

    # Check VST3 exists (skip in dry-run)
    info "  - Checking VST3 exists..."
    if [ "$DRY_RUN" = false ] && [ ! -d "$vst3_path" ]; then
        error "VST3 not found at: $vst3_path"
        echo "ERROR: VST3 verification failed" >> "$LOG_FILE"
        exit 1
    fi

    # Check AU exists (skip in dry-run)
    info "  - Checking AU exists..."
    if [ "$DRY_RUN" = false ] && [ ! -d "$au_path" ]; then
        error "AU not found at: $au_path"
        echo "ERROR: AU verification failed" >> "$LOG_FILE"
        exit 1
    fi

    if [ "$DRY_RUN" = true ]; then
        # In dry-run mode, just show what would be verified
        info "  - Would check timestamps..."
        info "  - Would check file sizes..."
        echo ""
        success "Verification complete (dry-run)"
        echo ""
        info "Would verify:"
        echo "  VST3: $vst3_path"
        echo "  AU:   $au_path"
    else
        # Check timestamps (modified within last 60 seconds)
        info "  - Checking timestamps..."
        local now=$(date +%s)
        local vst3_mtime=$(stat -f %m "$vst3_path" 2>/dev/null || echo 0)
        local au_mtime=$(stat -f %m "$au_path" 2>/dev/null || echo 0)
        local vst3_age=$((now - vst3_mtime))
        local au_age=$((now - au_mtime))

        if [ $vst3_age -gt 60 ]; then
            warning "VST3 timestamp is older than 60 seconds (${vst3_age}s) - may not be fresh build"
            echo "WARNING: VST3 timestamp: ${vst3_age}s old" >> "$LOG_FILE"
        fi

        if [ $au_age -gt 60 ]; then
            warning "AU timestamp is older than 60 seconds (${au_age}s) - may not be fresh build"
            echo "WARNING: AU timestamp: ${au_age}s old" >> "$LOG_FILE"
        fi

        # Get file sizes
        info "  - Checking file sizes..."
        local vst3_size=$(du -sh "$vst3_path" | cut -f1)
        local au_size=$(du -sh "$au_path" | cut -f1)

        echo ""
        success "Verification complete"
        echo ""
        info "Installed plugins:"
        echo "  VST3: $vst3_path"
        echo "        Size: $vst3_size, Age: ${vst3_age}s"
        echo "  AU:   $au_path"
        echo "        Size: $au_size, Age: ${au_age}s"

        echo "" >> "$LOG_FILE"
        echo "Verification: PASS" >> "$LOG_FILE"
        echo "VST3: $vst3_path ($vst3_size, ${vst3_age}s old)" >> "$LOG_FILE"
        echo "AU: $au_path ($au_size, ${au_age}s old)" >> "$LOG_FILE"
    fi
}

# ============================================================================
# Main execution
# ============================================================================
main() {
    parse_args "$@"
    setup_logging

    echo "============================================================================"
    echo "JUCE Plugin Build & Installation: $PLUGIN_NAME"
    echo "============================================================================"

    # Phase 1: Pre-flight Validation
    phase_1_preflight_validation

    # Phase 2: Build
    phase_2_build

    # If --no-install, exit early
    if [ "$NO_INSTALL" = true ]; then
        echo ""
        success "Build complete (--no-install flag set, skipping installation)"

        local end_time=$(date +%s)
        local duration=$((end_time - START_TIME))
        info "Build time: ${duration}s"
        info "Log: $LOG_FILE"

        echo "" >> "$LOG_FILE"
        echo "Build completed at $(date)" >> "$LOG_FILE"
        echo "Duration: ${duration}s" >> "$LOG_FILE"
        exit 0
    fi

    # Phase 3: Extract PRODUCT_NAME
    phase_3_extract_product_name

    # Phase 4: Remove Old Versions
    phase_4_remove_old_versions

    # Phase 5: Install New Versions
    phase_5_install_new_versions

    # Phase 6: Clear DAW Caches
    phase_6_clear_daw_caches

    # Phase 7: Verification
    phase_7_verification

    # Final success message
    echo ""
    echo "============================================================================"
    success "Build and installation complete!"
    echo "============================================================================"

    local end_time=$(date +%s)
    local duration=$((end_time - START_TIME))
    info "Total time: ${duration}s"
    info "Log: $LOG_FILE"

    echo "" >> "$LOG_FILE"
    echo "Build completed at $(date)" >> "$LOG_FILE"
    echo "Duration: ${duration}s" >> "$LOG_FILE"
}

# Run main
main "$@"
