#!/usr/bin/env bash
# ==============================================================================
#   O-GranulatorInstrument — DSP-01 parity reference (PLAN-2.2 T3 / PD5)
#
#   Builds O-Granulator AT THE FORK PIN 82406fce out of tree (git archive, the
#   O-Granulator tests/n1/run.sh shape) as a processor console driven by
#   tests/parity/parent_render.cpp, then renders every row of ParityRows.h.
#
#   usage: plugins/O-GranulatorInstrument/tests/parity/run.sh [WORK_DIR] [rowPrefix]
#          WORK_DIR default ${TMPDIR:-/tmp}/ogi-parity — NEVER inside the repo.
#          Outputs land in WORK_DIR/out; then run our side:
#            O-GranulatorInstrument-render-harness --parity WORK_DIR/out
#
#   Exit: 0 = rendered and every parent self-check passed; 1 = self-check
#   failure; 2 = refused (module drift since the pin / bad WORK); 3 = build.
#   Nothing from the parent ever enters our CMake graph (P21): this project is
#   separate, and our build's compdb is checked for it (X1).
# ==============================================================================
set -euo pipefail

PIN=82406fce
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO="$(cd "${HERE}/../../../.." && pwd)"
WORK="${1:-${TMPDIR:-/tmp}/ogi-parity}"
PREFIX="${2:-}"
JUCE_SRC="${JUCE_DIR:-/Users/taylorbrook/JUCE}"

case "${WORK}" in
    "${REPO}"|"${REPO}"/*) echo "parity: WORK_DIR must be outside the repo (${WORK})" >&2; exit 2 ;;
esac
mkdir -p "${WORK}"
WORK="$(cd "${WORK}" && pwd)"

cd "${REPO}"
git cat-file -e "${PIN}" || { echo "parity: pin ${PIN} missing" >&2; exit 2; }
if ! git diff --quiet "${PIN}" HEAD -- modules/synthesis/grain-engine modules/tuning/pitch-collections modules/effects/reverb-unit scripts/param-dump; then
    echo "parity: grain-engine / pitch-collections / reverb-unit / scripts/param-dump differ from ${PIN} — refusing" >&2
    exit 2
fi

# 1. Parent tree + its modules, re-extracted every run (never trust a stale copy).
rm -rf "${WORK}/pin"
mkdir -p "${WORK}/pin"
git archive "${PIN}" plugins/O-Granulator modules scripts/param-dump | tar -x -C "${WORK}/pin"
[ -f "${WORK}/pin/plugins/O-Granulator/Source/dsp/Engine.cpp" ] || { echo "parity: archive lacks the parent engine" >&2; exit 2; }

# 2. Out-of-tree project (configured once, ninja incrementally).
P="${WORK}/proj"
mkdir -p "${P}"
ln -sfn "${WORK}/pin/modules" "${P}/modules"
ln -sfn "${WORK}/pin/scripts" "${P}/scripts"
cat > "${P}/CMakeLists.txt.new" <<EOF
cmake_minimum_required(VERSION 3.22)
project(ogi_parity VERSION 1.0.0)
set(OUARICON_COMPANY_NAME "Ouaricon Audio Development")
set(OUARICON_MANUFACTURER_CODE OuDv)
set(OUARICON_DEV_SUFFIX "-dev")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_subdirectory("${JUCE_SRC}" JUCE)
add_subdirectory("${WORK}/pin/plugins/O-Granulator" gran)
include(\${CMAKE_SOURCE_DIR}/scripts/param-dump/ParamDump.cmake)
ouaricon_add_processor_console(OuariconGranulator "${WORK}/pin/plugins/O-Granulator/Source" "${HERE}/parent_render.cpp" parity)
target_compile_features(proj-parity PRIVATE cxx_std_20)
target_compile_definitions(proj-parity PRIVATE OGRAN_TEST_HOOKS=1)
target_include_directories(proj-parity PRIVATE "${HERE}")
EOF
if ! cmp -s "${P}/CMakeLists.txt.new" "${P}/CMakeLists.txt" 2>/dev/null; then
    mv "${P}/CMakeLists.txt.new" "${P}/CMakeLists.txt"
    rm -rf "${P}/build"
else
    rm -f "${P}/CMakeLists.txt.new"
fi
if [ ! -f "${P}/build/build.ninja" ]; then
    cmake -S "${P}" -B "${P}/build" -G Ninja -DCMAKE_BUILD_TYPE=Release > "${P}/configure.log" 2>&1 \
        || { tail -40 "${P}/configure.log"; exit 3; }
fi
ninja -C "${P}/build" proj-parity > "${P}/build.log" 2>&1 || { tail -60 "${P}/build.log"; exit 3; }

# X1 positive control: the parity project's compdb DOES contain the parent source.
n_parent="$(grep -c 'O-Granulator/Source' "${P}/build/compile_commands.json" || true)"
echo "X1 positive control: parity compdb entries naming O-Granulator/Source = ${n_parent}"
[ "${n_parent}" -gt 0 ] || { echo "parity: X1 positive control vacuous" >&2; exit 2; }

# 3. Render.
BIN="${P}/build/proj-parity_artefacts/Release/proj-parity"
mkdir -p "${WORK}/out"
if [ -n "${PREFIX}" ]; then
    "${BIN}" "${WORK}/out" "${PREFIX}"
else
    "${BIN}" "${WORK}/out"
fi
