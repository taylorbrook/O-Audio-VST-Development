#!/bin/bash
# ============================================================================
# resolve-target.sh - Shared plugin target / product-name resolver
# ============================================================================
# Sourceable library AND directly executable CLI. Used by both the local
# build-and-install.sh and the CI workflow (.github/workflows/build-and-release.yml)
# so that target/product-name resolution logic lives in exactly one place.
#
# Contract: assumes CWD = repo root (same as build-and-install.sh). References
# plugins/<folder>/CMakeLists.txt and build/plugins/<folder> with relative paths.
#
# Both functions echo their single result to STDOUT and emit ONLY warnings to
# STDERR, so `$(...)` capture stays clean.
#
# CLI:
#   scripts/resolve-target.sh cmake-target  <folder>
#   scripts/resolve-target.sh product-name  <folder>
# ============================================================================

# ----------------------------------------------------------------------------
# resolve_cmake_target_for <plugin-folder>
# ----------------------------------------------------------------------------
# Resolves the first argument to juce_add_plugin(<TARGET> ...) — the JUCE build
# target, which may differ from the folder name or PRODUCT_NAME. Handles the
# target on the same line as "juce_add_plugin(" OR on the next line, a
# ${VAR}-backed target resolved via set(<VAR> ...) / project(<name>), an already
# built *_artefacts dir fallback, and finally the folder name as last resort.
resolve_cmake_target_for() {
    local folder="$1"
    local cml="plugins/$folder/CMakeLists.txt"
    local raw="" target=""

    # Grab the first argument to juce_add_plugin() — the raw token, which may be a
    # literal ("OuariconChorus", "O-Gain") or a CMake variable ("${PROJECT_NAME}").
    # Handles the argument on the same line as the "(" or on the next line.
    if [ -f "$cml" ]; then
        raw=$(awk '
            found {
                sub(/#.*/, "")
                if (match($0, /[^[:space:])]+/)) { print substr($0, RSTART, RLENGTH); exit }
            }
            /juce_add_plugin[[:space:]]*\(/ {
                sub(/.*juce_add_plugin[[:space:]]*\(/, "")
                found=1
                sub(/#.*/, "")
                if (match($0, /[^[:space:])]+/)) { print substr($0, RSTART, RLENGTH); exit }
            }
        ' "$cml")
    fi

    if [[ "$raw" == *'${'* ]]; then
        # Resolve a ${VAR} reference, e.g. juce_add_plugin(${PROJECT_NAME}) backed by
        # set(PROJECT_NAME OuariconTexture) — or the project(<name>) call for PROJECT_NAME.
        local var="${raw#*\$\{}"; var="${var%%\}*}"
        target=$(grep -E "set[[:space:]]*\([[:space:]]*${var}[[:space:]]" "$cml" | head -1 | \
                 sed -E "s/.*set[[:space:]]*\([[:space:]]*${var}[[:space:]]+([^[:space:])]+).*/\1/")
        if [ -z "$target" ] && [ "$var" = "PROJECT_NAME" ]; then
            target=$(grep -E "^[[:space:]]*project[[:space:]]*\(" "$cml" | head -1 | \
                     sed -E "s/.*project[[:space:]]*\([[:space:]]*([^[:space:])]+).*/\1/")
        fi
    else
        target="$raw"
    fi

    # Fallback: an already-built "<target>_artefacts" dir names the real target
    # authoritatively (covers unresolved variables when a prior build exists).
    if [ -z "$target" ]; then
        local built
        built=$(find "build/plugins/$folder" -maxdepth 1 -name '*_artefacts' -type d 2>/dev/null | head -1)
        [ -n "$built" ] && { local b; b=$(basename "$built"); target="${b%_artefacts}"; }
    fi

    # Last resort: folder name (correct for plugins whose target == folder name).
    # Warn to STDERR so `$(...)` capture of STDOUT stays clean.
    if [ -z "$target" ]; then
        echo "[resolve-target] WARNING: could not resolve juce_add_plugin() target for '$folder'; falling back to folder name" >&2
        target="$folder"
    fi

    printf '%s\n' "$target"
}

# ----------------------------------------------------------------------------
# resolve_product_name_for <plugin-folder>
# ----------------------------------------------------------------------------
# Replicates the CI source-parse of PRODUCT_NAME: grep the first PRODUCT_NAME
# "..." line from the plugin CMakeLists, extract the quoted value, strip any
# ${...} CMake-variable substrings, and default to the folder name when empty.
# (Local build-and-install.sh Phase 3 reads PRODUCT_NAME from built artefacts
# and is strictly more robust — this mirrors the CI parse only.)
resolve_product_name_for() {
    local folder="$1"
    local cml="plugins/$folder/CMakeLists.txt"
    local name=""

    if [ -f "$cml" ]; then
        name=$(grep -E "PRODUCT_NAME" "$cml" | head -1 | \
               sed -E 's/.*PRODUCT_NAME[[:space:]]+"([^"]*)".*/\1/')
        # Strip CMake variable references like ${OUARICON_DEV_SUFFIX}
        name=$(echo "$name" | sed 's/\${[^}]*}//g')
    fi

    printf '%s\n' "${name:-$folder}"
}

# ----------------------------------------------------------------------------
# CLI dispatch (only when executed directly, not when sourced)
# ----------------------------------------------------------------------------
if [ "${BASH_SOURCE[0]}" = "${0}" ]; then
    case "${1:-}" in
        cmake-target)
            resolve_cmake_target_for "${2:-}"
            ;;
        product-name)
            resolve_product_name_for "${2:-}"
            ;;
        *)
            echo "Usage: $0 {cmake-target|product-name} <plugin-folder>" >&2
            exit 1
            ;;
    esac
fi
