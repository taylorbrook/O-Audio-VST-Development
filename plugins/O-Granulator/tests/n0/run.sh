#!/usr/bin/env bash
# ==============================================================================
#   O-Granulator — N0 neutral-FX out-of-tree null (PLAN-2.3 T1 / P17,
#   RESEARCH-2.3 §7). The tests/r12 machinery with OLD_SHA = c10d0f2c (the
#   verified 2.2 tree).
#
#   usage: plugins/O-Granulator/tests/n0/run.sh [--strict] [WORK_DIR]
#          (WORK_DIR default: ${TMPDIR:-/tmp}/ogran-n0; nothing is written
#           inside the repo)
#
#   Exit 0 = N0 free and capped max|Δ| == 0 (non-vacuous: free 0 steals and
#   0 kills on both sides, capped kills > 0 on both sides, planCalls > 0 for
#   every stream, every parameter exact on both sides) AND N0c (new side,
#   seed + 1) differs. With --strict, every per-stage control N0n-F / D / C / R
#   must also DIFFER from the old side's free render (T8 onwards); without it
#   they are printed DIFFERS / IDENTICAL for the record.
#
#   How it works: see tests/r12/run.sh — git archive of the old tree, one CMake
#   wrapper per side (proj-{old,new}), the SAME n0_render.cpp compiled twice as
#   a processor console with OGRAN_TEST_HOOKS=1 (diag counters only).
# ==============================================================================
set -euo pipefail

STRICT=0
if [ "${1:-}" = "--strict" ]; then STRICT=1; shift; fi

OLD_SHA=c10d0f2c
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO="$(cd "$HERE/../../../.." && pwd)"
WORK="${1:-${TMPDIR:-/tmp}/ogran-n0}"
JUCE_SRC="${JUCE_DIR:-/Users/taylorbrook/JUCE}"
mkdir -p "$WORK"

cd "$REPO"
if ! git diff --quiet "${OLD_SHA}" -- modules scripts/param-dump; then
    echo "N0: modules/ or scripts/param-dump differ from ${OLD_SHA} — archive them too before trusting a null" >&2
    exit 2
fi

# 1. Old tree (re-extracted every run: never trust a stale copy).
rm -rf "$WORK/old"
mkdir -p "$WORK/old"
git archive "${OLD_SHA}" plugins/O-Granulator | tar -x -C "$WORK/old"

make_proj () {   # $1 = side, $2 = plugin tree
    local P="$WORK/proj-$1"
    mkdir -p "$P"
    ln -sfn "$REPO/modules" "$P/modules"
    ln -sfn "$REPO/scripts" "$P/scripts"
    cat > "$P/CMakeLists.txt" <<EOF
cmake_minimum_required(VERSION 3.22)
project(ogran_n0_$1 VERSION 1.0.0)
set(OUARICON_COMPANY_NAME "Ouaricon Audio Development")
set(OUARICON_MANUFACTURER_CODE OuDv)
set(OUARICON_DEV_SUFFIX "-dev")
add_subdirectory("${JUCE_SRC}" JUCE)
add_subdirectory("$2" gran)
include(\${CMAKE_SOURCE_DIR}/scripts/param-dump/ParamDump.cmake)
ouaricon_add_processor_console(OuariconGranulator "$2/Source" "${HERE}/n0_render.cpp" n0)
target_compile_features(proj-$1-n0 PRIVATE cxx_std_20)
target_compile_definitions(proj-$1-n0 PRIVATE OGRAN_TEST_HOOKS=1)
EOF
    if [ ! -f "$P/build/build.ninja" ]; then
        cmake -S "$P" -B "$P/build" -G Ninja -DCMAKE_BUILD_TYPE=Release > "$P/configure.log"
    fi
    ninja -C "$P/build" "proj-$1-n0" > "$P/build.log" 2>&1 || { tail -40 "$P/build.log"; exit 3; }
}

make_proj old "$WORK/old/plugins/O-Granulator"
make_proj new "$REPO/plugins/O-Granulator"

bin () { echo "$WORK/proj-$1/build/proj-$1-n0_artefacts/Release/proj-$1-n0"; }

status=0
cd "$WORK"
field () { echo "$1" | sed -n "s/.* $2 \([0-9-]*\).*/\1/p"; }

# Non-vacuity on one diag line: every param LANDED (exact, or within 1e-5 of
# its span where a skewed range cannot reach the value exactly), planCalls > 0
# per stream. Raw-value identity across the two sides is checked separately.
nonvac () {   # $1 = diag line
    local la tot
    la="$(field "$1" landed)"
    tot="$(echo "$1" | sed -n 's/.* exact \([0-9]*\)\/\([0-9]*\).*/\2/p')"
    [ -n "$la" ] && [ "$la" = "$tot" ] || return 1
    for s in 1 2 3 4; do
        [ "$(field "$1" "plan$s")" -gt 0 ] || return 1
    done
    return 0
}

for c in free capped; do
    echo "== N0 case $c"
    o="$("$(bin old)" "old-$c.f32" "$c" base 2>/dev/null)"; echo "  old $o"
    n="$("$(bin new)" "new-$c.f32" "$c" base 2>/dev/null)"; echo "  new $n"
    live=1
    raw_o="$(echo "$o" | sed -n 's/.* raw \([0-9a-f]*\).*/\1/p')"
    raw_n="$(echo "$n" | sed -n 's/.* raw \([0-9a-f]*\).*/\1/p')"
    [ -n "$raw_o" ] && [ "$raw_o" = "$raw_n" ] || { echo "  raw parameter values differ between sides"; live=0; }
    for d in "$o" "$n"; do
        nonvac "$d" || live=0
        if [ "$c" = free ]; then
            [ "$(field "$d" steals)" = 0 ] && [ "$(field "$d" kills)" = 0 ] || live=0
        else
            [ "$(field "$d" kills)" -gt 0 ] || live=0
        fi
    done
    if "$(bin new)" compare "old-$c.f32" "new-$c.f32" && [ $live = 1 ]; then
        echo "  [PASS] N0 $c: max|d| == 0"
    else
        [ $live = 1 ] || echo "  VACUOUS (params landed + identical raw / planCalls / steal-kill precondition not met)"
        echo "  [FAIL] N0 $c"; status=1
    fi
done

# Controls — new side only, each compared with the old side's FREE render.
echo "== N0 controls (vs old free)"
for v in seed1 F D C R; do
    n="$("$(bin new)" "new-free-$v.f32" free "$v" 2>/dev/null)"
    if "$(bin new)" compare "old-free.f32" "new-free-$v.f32" > "cmp-$v.txt"; then res=IDENTICAL; else res=DIFFERS; fi
    case "$v" in
        seed1) name=N0c ;;
        *)     name="N0n-$v" ;;
    esac
    echo "  $name: $res  ($(cat "cmp-$v.txt"))"
    echo "     $n"
    if [ "$v" = seed1 ]; then
        if [ "$res" = DIFFERS ]; then echo "  [PASS] N0c differs"; else echo "  [FAIL] N0c identical"; status=1; fi
    elif [ $STRICT = 1 ]; then
        if [ "$res" = DIFFERS ]; then echo "  [PASS] $name differs (strict)"; else echo "  [FAIL] $name identical (strict)"; status=1; fi
    fi
done
exit $status
