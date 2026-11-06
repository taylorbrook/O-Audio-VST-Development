#!/usr/bin/env bash
# ==============================================================================
#   O-Granulator — N3 neutral-2.5b out-of-tree null (PLAN-2.5b T1 / P60,
#   CONTEXT-2.5b gate). The tests/n2 machinery with OLD_SHA = 2cf30120 (the
#   verified 2.5a tree).
#
#   usage: plugins/O-Granulator/tests/n3/run.sh [--strict] [WORK_DIR]
#          (WORK_DIR default: ${TMPDIR:-/tmp}/ogran-n3; nothing is written
#           inside the repo)
#
#   Exit 0 = N3 free and capped max|Δ| == 0 (non-vacuous: free 0 steals and
#   0 kills on both sides, capped kills > 0 on both sides, planCalls > 0 for
#   every stream, every parameter — incl. the non-default but neutral 2.5a and
#   2.5b dials — landed with identical raw values on both sides, max|wet| below
#   the D9 knee T − 1 dB on both sides) AND N3c (new side, seed + 1) differs
#   AND both positive controls are IDENTICAL:
#     N3p-THR  onset_threshold swept −60 → 0 dB per host block (no consumer)
#     N3p-Z    Pitch → s1_level, Onset → mix at depth 0 (R46)
#   A MockPlayHead (constant 120 bpm, playing) is attached on BOTH sides.
#
#   Controls N3n-TRG / ONS / PIT / FOL render the SAME variant on both sides
#   (each turns ONE 2.5b element minimally on): each is IDENTICAL until its
#   stage lands (TRG T3, ONS T4, PIT T6, FOL T8) and must DIFFER from then on.
#   With --strict (T8 onwards) all four must differ AND the analysis liveness
#   line of the new base render (onsets / hops / confident / f0writes) must be
#   > 0 on every counter; without it they are printed for the record.
#
#   How it works: see tests/r12/run.sh. The old side takes plugins/O-Granulator
#   AND modules/ from the old tree's archive; grain-engine, pitch-collections,
#   reverb-unit and scripts/param-dump must be byte-identical (2.5b changes no
#   module), or the script refuses.
# ==============================================================================
set -euo pipefail

STRICT=0
if [ "${1:-}" = "--strict" ]; then STRICT=1; shift; fi

OLD_SHA=2cf30120
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO="$(cd "$HERE/../../../.." && pwd)"
WORK="${1:-${TMPDIR:-/tmp}/ogran-n3}"
JUCE_SRC="${JUCE_DIR:-/Users/taylorbrook/JUCE}"
mkdir -p "$WORK"

cd "$REPO"
if ! git diff --quiet "${OLD_SHA}" -- modules/synthesis/grain-engine modules/tuning/pitch-collections modules/effects/reverb-unit scripts/param-dump; then
    echo "N3: grain-engine / pitch-collections / reverb-unit / scripts/param-dump differ from ${OLD_SHA} — archive them too before trusting a null" >&2
    exit 2
fi

# 1. Old tree + its modules (re-extracted every run: never trust a stale copy).
rm -rf "$WORK/old"
mkdir -p "$WORK/old"
git archive "${OLD_SHA}" plugins/O-Granulator modules | tar -x -C "$WORK/old"
[ -f "$WORK/old/modules/effects/reverb-unit/cpp/ReverbUnit.h" ] || { echo "N3: old archive lacks reverb-unit" >&2; exit 2; }

make_proj () {   # $1 = side, $2 = plugin tree, $3 = modules tree
    local P="$WORK/proj-$1"
    mkdir -p "$P"
    ln -sfn "$3" "$P/modules"
    ln -sfn "$REPO/scripts" "$P/scripts"
    cat > "$P/CMakeLists.txt" <<EOF
cmake_minimum_required(VERSION 3.22)
project(ogran_n3_$1 VERSION 1.0.0)
set(OUARICON_COMPANY_NAME "Ouaricon Audio Development")
set(OUARICON_MANUFACTURER_CODE OuDv)
set(OUARICON_DEV_SUFFIX "-dev")
add_subdirectory("${JUCE_SRC}" JUCE)
add_subdirectory("$2" gran)
include(\${CMAKE_SOURCE_DIR}/scripts/param-dump/ParamDump.cmake)
ouaricon_add_processor_console(OuariconGranulator "$2/Source" "${HERE}/n3_render.cpp" n3)
target_compile_features(proj-$1-n3 PRIVATE cxx_std_20)
target_compile_definitions(proj-$1-n3 PRIVATE OGRAN_TEST_HOOKS=1)
EOF
    if [ ! -f "$P/build/build.ninja" ]; then
        cmake -S "$P" -B "$P/build" -G Ninja -DCMAKE_BUILD_TYPE=Release > "$P/configure.log"
    fi
    ninja -C "$P/build" "proj-$1-n3" > "$P/build.log" 2>&1 || { tail -40 "$P/build.log"; exit 3; }
}

make_proj old "$WORK/old/plugins/O-Granulator" "$WORK/old/modules"
make_proj new "$REPO/plugins/O-Granulator" "$REPO/modules"
# The old side's reverb-unit must be the archived copy (not the working tree's).
[ "$(readlink "$WORK/proj-old/modules")" = "$WORK/old/modules" ] || { echo "N3: old side modules link wrong" >&2; exit 2; }

bin () { echo "$WORK/proj-$1/build/proj-$1-n3_artefacts/Release/proj-$1-n3"; }

status=0
cd "$WORK"
field () { echo "$1" | sed -n "s/.* $2 \([0-9-]*\).*/\1/p"; }
sfield () { echo "$1" | sed -n "s/.* $2 \([0-9a-z/-]*\).*/\1/p"; }
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
# Stimulus bound: peak input <= 0.5 (the N2 bound).
peakok () { awk -v w="$1" 'BEGIN { exit !(w + 0 <= 0.5) }'; }

newbase=""
for c in free capped; do
    echo "== N3 case $c"
    o="$("$(bin old)" "old-$c.f32" "$c" base 2>/dev/null)"; echo "  old $o"
    n="$("$(bin new)" "new-$c.f32" "$c" base 2>/dev/null)"; echo "  new $n"
    [ "$c" = free ] && newbase="$n"
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
        peakok "$(ffield "$d" peakin)" || { echo "  peak input $(ffield "$d" peakin) above 0.5"; live=0; }
    done
    if "$(bin new)" compare "old-$c.f32" "new-$c.f32" && [ $live = 1 ]; then
        echo "  [PASS] N3 $c: max|d| == 0"
    else
        [ $live = 1 ] || echo "  VACUOUS (landed / raw / planCalls / steal-kill / below-knee / peak precondition not met)"
        echo "  [FAIL] N3 $c"; status=1
    fi
done

# Analysis liveness (new side, free base): onsets / hops / confident / f0writes.
lv_on="$(sfield "$newbase" onsets)"; lv_hop="$(sfield "$newbase" hops)"
lv_cf="$(sfield "$newbase" confident)"; lv_f0="$(sfield "$newbase" f0writes)"
echo "== N3 analysis liveness (new free base): onsets $lv_on hops $lv_hop confident $lv_cf f0writes $lv_f0"
if [ $STRICT = 1 ]; then
    alive=1
    for v in "$lv_on" "$lv_hop" "$lv_cf" "$lv_f0"; do
        case "$v" in ''|n/a|0) alive=0 ;; esac
    done
    if [ $alive = 1 ]; then echo "  [PASS] N3 liveness: every counter > 0 (strict)"; else echo "  [FAIL] N3 liveness (strict)"; status=1; fi
fi

# N3c — new side seed + 1 vs the old side's free render.
echo "== N3c (vs old free)"
n="$("$(bin new)" "new-free-seed1.f32" free seed1 2>/dev/null)"
if "$(bin new)" compare "old-free.f32" "new-free-seed1.f32" > "cmp-seed1.txt"; then res=IDENTICAL; else res=DIFFERS; fi
echo "  N3c: $res  ($(cat cmp-seed1.txt))"
if [ "$res" = DIFFERS ]; then echo "  [PASS] N3c differs"; else echo "  [FAIL] N3c identical"; status=1; fi

# Positive controls — IDENTICAL at every task.
echo "== N3p positive controls (old variant vs new variant)"
for v in THR Z; do
    o="$("$(bin old)" "old-free-$v.f32" free "$v" 2>/dev/null)"
    n="$("$(bin new)" "new-free-$v.f32" free "$v" 2>/dev/null)"
    if "$(bin new)" compare "old-free-$v.f32" "new-free-$v.f32" > "cmp-$v.txt"; then res=IDENTICAL; else res=DIFFERS; fi
    echo "  N3p-$v: $res  ($(cat "cmp-$v.txt"))"
    echo "     old $o"
    echo "     new $n"
    if [ "$res" = IDENTICAL ]; then echo "  [PASS] N3p-$v identical"; else echo "  [FAIL] N3p-$v differs"; status=1; fi
done

# Per-element controls — the SAME variant on both sides (free case).
echo "== N3n controls (old variant vs new variant)"
for v in TRG ONS PIT FOL; do
    o="$("$(bin old)" "old-free-$v.f32" free "$v" 2>/dev/null)"
    n="$("$(bin new)" "new-free-$v.f32" free "$v" 2>/dev/null)"
    if "$(bin new)" compare "old-free-$v.f32" "new-free-$v.f32" > "cmp-$v.txt"; then res=IDENTICAL; else res=DIFFERS; fi
    echo "  N3n-$v: $res  ($(cat "cmp-$v.txt"))"
    echo "     old $o"
    echo "     new $n"
    if [ $STRICT = 1 ]; then
        if [ "$res" = DIFFERS ]; then echo "  [PASS] N3n-$v differs (strict)"; else echo "  [FAIL] N3n-$v identical (strict)"; status=1; fi
    fi
done
exit $status
