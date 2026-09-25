#!/usr/bin/env bash
# This file is part of the Ouaricon Audio eb-garamond module.
# Copyright (C) 2026  Ouaricon Audio
# SPDX-License-Identifier: AGPL-3.0-or-later
#
# Rebuilds fonts/EBGaramond-{Regular,Italic,Bold}.woff2 and OFL.txt from a
# PINNED google/fonts commit. Every input is sha256-checked; the run aborts on
# any mismatch. Output is byte-reproducible (SOURCE_DATE_EPOCH fixes the
# fontTools timestamps) — two runs must hash identically.
#
# Steps: download variable TTFs -> static instances (400, 700, 400 italic) ->
# subset (Latin + Latin-Ext + the punctuation, arrows and fleurons the UIs use)
# -> bake Times New Roman's vertical metrics (891 / -216 / 42, USE_TYPO_METRICS)
# so a line box under `line-height: normal` is the same height it was on Times.
#
# Requires: curl, shasum, python3 with fontTools (fonttools, pyftsubset) and
# brotli. Nothing is installed by this script.
#
# usage: bash modules/ui/eb-garamond/tools/build-fonts.sh

set -euo pipefail

MODULE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

SHA="f8c1d3d6cc75e30d77130bdcbfbff27e3b6233fe"
BASE="https://raw.githubusercontent.com/google/fonts/${SHA}/ofl/ebgaramond"

SHA_ROMAN="ef9512f92f6d579e5dc75af59a5a4b1b8b47d2eda89e00b954d44520e5369027"
SHA_ITALIC="bba2c4499c93c9612b90b9825d32b07da52fce2fe57562a1eb6b833553f93c4e"
SHA_OFL="0985066662eb755ed3683ae5482a81a9195b49ce3f7e165cc2388b3dbece7dd7"

# google/fonts@f8c1d3d6 commit date, 2026-06-17 00:00:00 UTC.
export SOURCE_DATE_EPOCH=1781654400

UNI="U+0000-00FF,U+0100-017F,U+0192,U+0218-021B,U+02BB-02BC,U+02C6-02DD,U+0300-0308,U+0327,U+0329,U+1E9E,U+2000-206F,U+20AC,U+2116,U+2122,U+2190-2199,U+2212,U+2215,U+2260,U+2264-2265,U+25B2-25C0,U+2619,U+2766-2767,U+FB00-FB06,U+FEFF,U+FFFD"
FEAT="kern,liga,mark,locl,lnum,tnum,onum,pnum,smcp,c2sc,case"

WORK="$(mktemp -d)"
trap 'rm -rf "${WORK}"' EXIT

echo "== download google/fonts@${SHA} ofl/ebgaramond"
curl -fsSL --retry 3 -o "${WORK}/VF.ttf"  "${BASE}/EBGaramond%5Bwght%5D.ttf"
curl -fsSL --retry 3 -o "${WORK}/VFi.ttf" "${BASE}/EBGaramond-Italic%5Bwght%5D.ttf"
curl -fsSL --retry 3 -o "${WORK}/OFL.txt" "${BASE}/OFL.txt"

echo "== verify sha256"
(
    cd "${WORK}"
    printf '%s  %s\n%s  %s\n%s  %s\n' \
        "${SHA_ROMAN}"  "VF.ttf" \
        "${SHA_ITALIC}" "VFi.ttf" \
        "${SHA_OFL}"    "OFL.txt" | shasum -a 256 -c -
)

echo "== instance"
fonttools varLib.instancer "${WORK}/VF.ttf"  wght=400 --update-name-table -q -o "${WORK}/Regular.ttf"
fonttools varLib.instancer "${WORK}/VF.ttf"  wght=700 --update-name-table -q -o "${WORK}/Bold.ttf"
fonttools varLib.instancer "${WORK}/VFi.ttf" wght=400 --update-name-table -q -o "${WORK}/Italic.ttf"

echo "== subset"
for FACE in Regular Italic Bold; do
    pyftsubset "${WORK}/${FACE}.ttf" \
        --unicodes="${UNI}" \
        --layout-features="${FEAT}" \
        --name-IDs='0,1,2,3,4,5,6,13,14' \
        --flavor=woff2 \
        --output-file="${WORK}/${FACE}.sub.woff2"
done

echo "== bake Times-matched vertical metrics"
mkdir -p "${MODULE_DIR}/fonts"
for FACE in Regular Italic Bold; do
    python3 - "${WORK}/${FACE}.sub.woff2" "${MODULE_DIR}/fonts/EBGaramond-${FACE}.woff2" <<'PY'
import sys
from fontTools.ttLib import TTFont

ASC, DESC, GAP = 891, 216, 42          # Times New Roman (macOS) per 1000 UPM
src, dst = sys.argv[1], sys.argv[2]

f = TTFont(src)
assert f['head'].unitsPerEm == 1000, 'metric bake assumes UPM 1000'
hhea, os2, head = f['hhea'], f['OS/2'], f['head']

hhea.ascent, hhea.descent, hhea.lineGap = ASC, -DESC, GAP
os2.sTypoAscender, os2.sTypoDescender, os2.sTypoLineGap = ASC, -DESC, GAP
os2.fsSelection |= (1 << 7)            # USE_TYPO_METRICS
# win metrics are the clip box on Windows: never cut ink.
os2.usWinAscent = max(ASC + GAP, head.yMax)
os2.usWinDescent = max(DESC, -head.yMin)

f.flavor = 'woff2'
f.save(dst)
PY
done

cp "${WORK}/OFL.txt" "${MODULE_DIR}/OFL.txt"

echo "== outputs"
for FACE in Regular Italic Bold; do
    OUT="${MODULE_DIR}/fonts/EBGaramond-${FACE}.woff2"
    printf '%s  %8d B  %s\n' "$(shasum -a 256 "${OUT}" | cut -d' ' -f1)" "$(wc -c < "${OUT}" | tr -d ' ')" "fonts/EBGaramond-${FACE}.woff2"
done
printf '%s  %8d B  %s\n' "$(shasum -a 256 "${MODULE_DIR}/OFL.txt" | cut -d' ' -f1)" "$(wc -c < "${MODULE_DIR}/OFL.txt" | tr -d ' ')" "OFL.txt"
