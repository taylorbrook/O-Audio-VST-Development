#!/usr/bin/env bash
# ==============================================================================
#   O-Granulator — N1 neutral-2.4 out-of-tree null (PLAN-2.4 T1 / P31,
#   CONTEXT-2.4 gate 1). The tests/n0 machinery with OLD_SHA = 6aed8017 (the
#   verified 2.3 tree).
#
#   usage: plugins/O-Granulator/tests/n1/run.sh [--strict] [WORK_DIR]
#          (WORK_DIR default: ${TMPDIR:-/tmp}/ogran-n1; nothing is written
#           inside the repo)
#
#   Exit 0 = N1 free and capped max|Δ| == 0 (non-vacuous: free 0 steals and
#   0 kills on both sides, capped kills > 0 on both sides, planCalls > 0 for
#   every stream, every parameter landed with identical raw values on both
#   sides, max|wet| below the D9 knee T − 1 dB on both sides) AND N1c (new
#   side, seed + 1) differs.
#
#   Controls N1n-MK / KN / FB / FZ / RV render the SAME variant on both sides
#   (each turns ONE 2.4 element minimally on): each is IDENTICAL until its
#   stage lands (MK T4, KN T5, FB T7, FZ T8, RV T10) and must DIFFER from then
#   on. With --strict (T10 onwards) all five must differ; without it they are
#   printed for the record.
#
#   How it works: see tests/r12/run.sh. The old side takes plugins/O-Granulator
#   AND modules/ from the old tree's archive (reverb-unit moves to 1.2.0 at
#   T10); grain-engine, pitch-collections and scripts/param-dump must be
#   byte-identical, or the script refuses.
# ==============================================================================
set -euo pipefail

STRICT=0
if [ "${1:-}" = "--strict" ]; then STRICT=1; shift; fi

OLD_SHA=6aed8017
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO="$(cd "$HERE/../../../.." && pwd)"
WORK="${1:-${TMPDIR:-/tmp}/ogran-n1}"
JUCE_SRC="${JUCE_DIR:-/Users/taylorbrook/JUCE}"
mkdir -p "$WORK"

cd "$REPO"
if ! git diff --quiet "${OLD_SHA}" -- modules/synthesis/grain-engine modules/tuning/pitch-collections scripts/param-dump; then
    echo "N1: grain-engine / pitch-collections / scripts/param-dump differ from ${OLD_SHA} — archive them too before trusting a null" >&2
    exit 2
fi

# 1. Old tree + its modules (re-extracted every run: never trust a stale copy).
rm -rf "$WORK/old"
mkdir -p "$WORK/old"
git archive "${OLD_SHA}" plugins/O-Granulator modules | tar -x -C "$WORK/old"
[ -f "$WORK/old/modules/effects/reverb-unit/cpp/ReverbUnit.h" ] || { echo "N1: old archive lacks reverb-unit" >&2; exit 2; }

make_proj () {   # $1 = side, $2 = plugin tree, $3 = modules tree
    local P="$WORK/proj-$1"
    mkdir -p "$P"
    ln -sfn "$3" "$P/modules"
    ln -sfn "$REPO/scripts" "$P/scripts"
    cat > "$P/CMakeLists.txt" <<EOF
cmake_minimum_required(VERSION 3.22)
project(ogran_n1_$1 VERSION 1.0.0)
set(OUARICON_COMPANY_NAME "Ouaricon Audio Development")
set(OUARICON_MANUFACTURER_CODE OuDv)
set(OUARICON_DEV_SUFFIX "-dev")
add_subdirectory("${JUCE_SRC}" JUCE)
add_subdirectory("$2" gran)
include(\${CMAKE_SOURCE_DIR}/scripts/param-dump/ParamDump.cmake)
ouaricon_add_processor_console(OuariconGranulator "$2/Source" "${HERE}/n1_render.cpp" n1)
target_compile_features(proj-$1-n1 PRIVATE cxx_std_20)
target_compile_definitions(proj-$1-n1 PRIVATE OGRAN_TEST_HOOKS=1)
EOF
    if [ ! -f "$P/build/build.ninja" ]; then
        cmake -S "$P" -B "$P/build" -G Ninja -DCMAKE_BUILD_TYPE=Release > "$P/configure.log"
    fi
    ninja -C "$P/build" "proj-$1-n1" > "$P/build.log" 2>&1 || { tail -40 "$P/build.log"; exit 3; }
}

make_proj old "$WORK/old/plugins/O-Granulator" "$WORK/old/modules"
make_proj new "$REPO/plugins/O-Granulator" "$REPO/modules"
# The old side's reverb-unit must be the archived copy (not the working tree's).
[ "$(readlink "$WORK/proj-old/modules")" = "$WORK/old/modules" ] || { echo "N1: old side modules link wrong" >&2; exit 2; }

bin () { echo "$WORK/proj-$1/build/proj-$1-n1_artefacts/Release/proj-$1-n1"; }

status=0
cd "$WORK"
field () { echo "$1" | sed -n "s/.* $2 \([0-9-]*\).*/\1/p"; }
ffield () { echo "$1" | sed -n "s/.* $2 \([0-9.eE+-]*\).*/\1/p"; }

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

# Below-knee non-vacuity: max|wet| < T − 1 dB (T = 0.501187234 → 0.44668).
belowknee () { awk -v w="$1" 'BEGIN { exit !(w + 0 < 0.501187234 * 10^(-1/20)) }'; }

for c in free capped; do
    echo "== N1 case $c"
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
        belowknee "$(ffield "$d" maxwet)" || { echo "  max|wet| $(ffield "$d" maxwet) not below T - 1 dB"; live=0; }
    done
    if "$(bin new)" compare "old-$c.f32" "new-$c.f32" && [ $live = 1 ]; then
        echo "  [PASS] N1 $c: max|d| == 0"
    else
        [ $live = 1 ] || echo "  VACUOUS (landed / raw / planCalls / steal-kill / below-knee precondition not met)"
        echo "  [FAIL] N1 $c"; status=1
    fi
done

# N1c — new side seed + 1 vs the old side's free render.
echo "== N1c (vs old free)"
n="$("$(bin new)" "new-free-seed1.f32" free seed1 2>/dev/null)"
if "$(bin new)" compare "old-free.f32" "new-free-seed1.f32" > "cmp-seed1.txt"; then res=IDENTICAL; else res=DIFFERS; fi
echo "  N1c: $res  ($(cat cmp-seed1.txt))"
if [ "$res" = DIFFERS ]; then echo "  [PASS] N1c differs"; else echo "  [FAIL] N1c identical"; status=1; fi

# Per-element controls — the SAME variant on both sides (free case).
echo "== N1n controls (old variant vs new variant)"
for v in MK KN FB FZ RV; do
    o="$("$(bin old)" "old-free-$v.f32" free "$v" 2>/dev/null)"
    n="$("$(bin new)" "new-free-$v.f32" free "$v" 2>/dev/null)"
    if "$(bin new)" compare "old-free-$v.f32" "new-free-$v.f32" > "cmp-$v.txt"; then res=IDENTICAL; else res=DIFFERS; fi
    echo "  N1n-$v: $res  ($(cat "cmp-$v.txt"))"
    echo "     old $o"
    echo "     new $n"
    if [ $STRICT = 1 ]; then
        if [ "$res" = DIFFERS ]; then echo "  [PASS] N1n-$v differs (strict)"; else echo "  [FAIL] N1n-$v identical (strict)"; status=1; fi
    fi
done
exit $status
