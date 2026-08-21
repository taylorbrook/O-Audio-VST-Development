#!/usr/bin/env python3
# This file is part of O-Octagon, an Ouaricon Audio plugin.
# Copyright (C) 2026  Ouaricon Audio
# SPDX-License-Identifier: AGPL-3.0-or-later
"""
The DSP-01 oracle — an INDEPENDENT implementation of the DBAP gain equations.

DSP-01 acceptance criterion 3: *"the gain vector matches an independent reference implementation of
equations 9-10 to 1e-6 over a fixture set."*  This script is that reference, and the fixture it emits
is COMMITTED (PLAN-2.2 P22).

  Layer 2's JUCE golden          this DBAP reference
  ---------------------------    ------------------------------------------
  tracks a MOVING target (JUCE)  tracks a FIXED published equation set
  regenerate on a JUCE bump      never regenerate
  generated at BUILD time        generated ONCE, reviewed, committed

Regenerating every build would make the oracle a build product: a change to the C++ solver plus a
matching change to this script would agree silently, with nothing in the diff. Committed, it shows
up in review. Run with --check to prove the committed file still matches this generator.

═══ EQUATIONS: THE 2011-04-14 REVISION ONLY ═══════════════════════════════════════════════════════
The original DBAP paper's equations 3-6 and 9-10 are WRONG
(memory pattern: pattern_dbap_not_vbap_for_irregular_arrays). Everything below follows the revised
edition:

    a      = rolloff / (20 * log10(2))
    r_s    = min(blur * 1.5 * rigScale, 24.0)                 [metres]  (v1.3.0 scale)
    d_i    = max(sqrt(dx^2 + dy^2 + dz^2 + r_s^2), 0.05)      [metres, 3D — the z term is real]
    u_i    = w_i * d_i^(-a)
    S      = sum(u_i^2)
    S < 1e-20  ->  v = 0        (SILENCE, explicitly — not NaN, not full scale)
    otherwise  ->  v_i = u_i / sqrt(S)                        so that sum(v_i^2) = 1

═══ INDEPENDENCE DISCIPLINE — READ BEFORE EDITING ═════════════════════════════════════════════════
An oracle that re-runs the implementation's own expression reproduces its errors and passes forever.
This file therefore deliberately does NOT transcribe the C++:

  * d^(-a) is computed as exp(-a * log(d)), NOT via pow() and NOT via the C++'s `t`/`t*t` reuse;
  * normalisation forms sum(v^2) EXPLICITLY and divides by its square root, rather than taking the
    C++'s k = 1/sqrt(denom) shortcut;
  * everything runs in double precision, against inputs pre-rounded to float32 (see below).

This is the same argument SUMMARY-2.1 F2 made for the hull oracle, which was a ternary search rather
than the closed-form projection the implementation uses.

═══ WHY INPUTS ARE PRE-ROUNDED THROUGH float32 ════════════════════════════════════════════════════
The C++ solver is single-precision, so it sees float(x) for every input. If this script computed
against the full-precision double, the comparison would fold input rounding into the measured
deviation and DSP-01's 1e-6 would be testing the wrong thing. Rounding each input through float32
before computing means both sides start from bit-identical values and the number probe Y prints is
purely the arithmetic's own error.

Exits non-zero on any failure and NEVER emits a fixture with zero cases: a vacuous oracle is worse
than no oracle, because it reports green.
"""

import argparse
import math
import struct
import sys
from pathlib import Path

FIXTURE_VERSION = 1
NUM_SPEAKERS = 8

# §3.3.1 constants, restated here rather than parsed out of the C++ — a reference that reads its
# constants from the thing it checks is not a reference.
MIN_DISTANCE = 0.05
MAX_BLUR_METRES = 24.0   # v1.3.0 audibility rescale (was 8.0)
BLUR_SCALE = 1.5         # v1.3.0 audibility rescale (was 0.5)
DENOM_EPSILON = 1e-20

# ARCHITECTURE §OQ4's default venue. The fixture is SELF-CONTAINED (CONTEXT-2.2 D3): it carries
# these coordinates and the C++ probe feeds them straight to the solver, so no mirrored coordinate
# table exists anywhere and none can drift. The heights are GRADED (4.50 -> 5.40 m) on purpose — a
# uniform z would make every (z_i - z_s) difference identical and hide a dropped z term.
SPEAKERS_M = [
    (0.50, 4.50, 4.50),    # 1 front-left
    (12.50, 4.50, 4.50),   # 2 front-right
    (12.50, 9.85, 4.70),   # 3 right-2nd
    (12.50, 16.00, 5.10),  # 4 right-3rd
    (9.80, 19.50, 5.40),   # 5 back-right
    (3.20, 19.50, 5.40),   # 6 back-left
    (0.50, 16.00, 5.10),   # 7 left-3rd
    (0.50, 9.85, 4.70),    # 8 left-2nd
]

PRNG_SEED = 0x0C7A2026


def die(message):
    sys.stderr.write("gen_dbap_reference.py: ERROR: %s\n" % message)
    sys.exit(1)


def f32(x):
    """Rounds a double to the nearest float32 and returns it back as a double."""
    return struct.unpack("f", struct.pack("f", float(x)))[0]


class Lcg:
    """A pinned 64-bit LCG.

    Deliberately not random.Random: the fixture must be byte-reproducible from this file alone,
    independent of the Python version's Mersenne Twister implementation details.
    """

    def __init__(self, seed):
        self.state = seed & 0xFFFFFFFFFFFFFFFF

    def next_u32(self):
        self.state = (self.state * 6364136223846793005 + 1442695040888963407) & 0xFFFFFFFFFFFFFFFF
        return (self.state >> 32) & 0xFFFFFFFF

    def uniform(self, lo, hi):
        return lo + (hi - lo) * (self.next_u32() / 4294967296.0)


# ══════════════════════════════════════════════════════════════════════════════════════════════════
# The reference implementation.
# ══════════════════════════════════════════════════════════════════════════════════════════════════

def rig_scale(speakers):
    """RMS 3-D speaker radius about the centroid.

    Recomputed here rather than taken from ARCHITECTURE's prose — that prose carried a hand-calc slip
    (7.95 vs 7.93165) that survived two phases and was only caught by independent recomputation.
    """
    n = float(len(speakers))
    cx = sum(p[0] for p in speakers) / n
    cy = sum(p[1] for p in speakers) / n
    cz = sum(p[2] for p in speakers) / n

    sum_sq = sum((p[0] - cx) ** 2 + (p[1] - cy) ** 2 + (p[2] - cz) ** 2 for p in speakers)
    return math.sqrt(sum_sq / n)


def solve_reference(speakers, weights, src, rolloff, blur, scale):
    """Equations 9-10, 2011-04-14 revision. Returns the 8 gains."""
    a = rolloff / (20.0 * math.log10(2.0))
    r_s = min(blur * BLUR_SCALE * scale, MAX_BLUR_METRES)
    rs_sq = r_s * r_s

    # d_i^(-a) via exp(-a*log d) — NOT pow(), and NOT the implementation's t/t*t reuse.
    u = []
    for (sx, sy, sz) in speakers:
        dx = sx - src[0]
        dy = sy - src[1]
        dz = sz - src[2]
        d = math.sqrt(dx * dx + dy * dy + dz * dz + rs_sq)
        d = max(d, MIN_DISTANCE)
        u.append(math.exp(-a * math.log(d)))

    numerators = [w * ui for (w, ui) in zip(weights, u)]

    # Normalise by EXPLICITLY forming sum(v^2), not via k = 1/sqrt(denom).
    sum_sq = sum(n * n for n in numerators)

    if sum_sq < DENOM_EPSILON:
        return [0.0] * NUM_SPEAKERS

    norm = math.sqrt(sum_sq)
    return [n / norm for n in numerators]


# ══════════════════════════════════════════════════════════════════════════════════════════════════
# Case construction.
# ══════════════════════════════════════════════════════════════════════════════════════════════════

def build_cases(speakers, scale):
    """Every case P22 names, plus a pinned grid and a pinned-seed pseudorandom set."""
    ones = [1.0] * NUM_SPEAKERS
    cases = []

    def add(name, weights, rolloff, blur, src):
        cases.append({
            "name": name,
            "w": [f32(x) for x in weights],
            "rolloff": f32(rolloff),
            "blur": f32(blur),
            "src": [f32(c) for c in src],
        })

    # ── Inside the hull ───────────────────────────────────────────────────────────────────────────
    add("inside-centre", ones, 4.0, 0.10, (6.5, 12.4625, 1.10))
    add("inside-offcentre", ones, 4.0, 0.10, (3.75, 8.25, 1.60))
    add("inside-high-srcZ", ones, 4.0, 0.10, (6.5, 12.4625, 6.40))
    add("inside-low-srcZ", ones, 4.0, 0.10, (6.5, 12.4625, -1.90))

    # ── Outside the hull (the solver never refuses a position; the caller projects) ───────────────
    add("outside-rear-corner", ones, 4.0, 0.10, (13.0, 22.0, 2.0))
    add("outside-downstage", ones, 4.0, 0.10, (6.5, -3.0, 1.10))
    add("outside-far-left", ones, 4.0, 0.10, (-8.0, 12.0, 1.10))

    # ── At hull vertices (speakers 1,2,4,5,6,7 are the §OQ4 hull; 3 and 8 are on-edge) ────────────
    for idx in (0, 1, 3, 4, 5, 6):
        p = speakers[idx]
        add("hull-vertex-spk%d" % (idx + 1), ones, 4.0, 0.10, (p[0], p[1], 1.10))

    # ── EXACT speaker coordinates with blur = 0 — the kMinDistance floor is the only thing keeping
    #    this finite (QUAL-02 criterion 1). All eight, including the on-edge pair.
    for idx, p in enumerate(speakers):
        add("on-speaker-%d-blur0" % (idx + 1), ones, 4.0, 0.0, p)

    # ── Both rolloff ends × both blur ends ────────────────────────────────────────────────────────
    for rolloff in (3.0, 6.0):
        for blur in (0.0, 1.0):
            add("rolloff%.0f-blur%.0f" % (rolloff, blur), ones, rolloff, blur, (4.25, 14.75, 1.35))

    # ── Weight subsets ────────────────────────────────────────────────────────────────────────────
    for idx in range(NUM_SPEAKERS):
        w = [0.0] * NUM_SPEAKERS
        w[idx] = 1.0
        # With w = delta_ij the normalisation is exact: v_j = 1 analytically. This is the fixture
        # half of render-harness probe Q'.
        add("single-weight-spk%d" % (idx + 1), w, 4.0, 0.10, (5.5, 11.0, 1.20))

    two = [0.0] * NUM_SPEAKERS
    two[0] = 1.0
    two[4] = 1.0
    add("two-weights-1-and-5", two, 4.0, 0.10, (5.5, 11.0, 1.20))

    graded = [0.0, 0.25, 0.5, 0.75, 1.0, 0.75, 0.5, 0.25]
    add("graded-weights-with-a-zero", graded, 5.0, 0.35, (7.75, 15.5, 0.85))

    # ── All-zero weights: SILENCE, not NaN and not full scale (DSP-05/3) ──────────────────────────
    add("all-zero-weights", [0.0] * NUM_SPEAKERS, 4.0, 0.10, (6.5, 12.0, 1.10))
    add("all-zero-weights-on-speaker", [0.0] * NUM_SPEAKERS, 6.0, 0.0, speakers[2])

    # ── A pinned grid across the room, at the default parameters ──────────────────────────────────
    for gx in range(5):
        for gy in range(5):
            x = -2.0 + 17.0 * (gx / 4.0)
            y = -2.0 + 24.0 * (gy / 4.0)
            add("grid-%d-%d" % (gx, gy), ones, 4.0, 0.10, (x, y, 1.10))

    # ── A pinned-seed pseudorandom set over the whole parameter product ───────────────────────────
    rng = Lcg(PRNG_SEED)

    for i in range(40):
        w = [rng.uniform(0.0, 1.0) for _ in range(NUM_SPEAKERS)]
        rolloff = rng.uniform(3.0, 6.0)
        blur = rng.uniform(0.0, 1.0)
        src = (rng.uniform(-6.0, 19.0), rng.uniform(-6.0, 26.0), rng.uniform(-2.0, 8.0))
        add("random-%02d" % i, w, rolloff, blur, src)

    if not cases:
        die("case list is empty — a fixture with zero cases reports green and tests nothing")

    # Solve every case AFTER the inputs have been rounded to float32, so both sides start from
    # bit-identical values.
    for case in cases:
        case["v"] = solve_reference(speakers, case["w"], case["src"],
                                    case["rolloff"], case["blur"], scale)

        for value in case["v"]:
            if not math.isfinite(value):
                die("case '%s' produced a non-finite reference gain — the ORACLE is wrong, and a "
                    "broken oracle is worse than none" % case["name"])

    return cases


# ══════════════════════════════════════════════════════════════════════════════════════════════════
# Emission.
# ══════════════════════════════════════════════════════════════════════════════════════════════════

def d(x):
    """A C++ double literal carrying every bit of the value."""
    return "%.17g" % x


def emit(speakers, scale, cases):
    out = []
    w = out.append

    w("/*")
    w("   This file is part of O-Octagon, an Ouaricon Audio plugin.")
    w("   Copyright (C) 2026  Ouaricon Audio")
    w("")
    w("   SPDX-License-Identifier: AGPL-3.0-or-later")
    w("*/")
    w("//" + "=" * 98)
    w("//  GENERATED FILE — DO NOT EDIT BY HAND.")
    w("//")
    w("//  Produced by tests/tools/gen_dbap_reference.py, which is an INDEPENDENT implementation of")
    w("//  the DBAP gain equations (2011-04-14 revision) written so that it does not transcribe the")
    w("//  C++ solver: d^(-a) via exp(-a*log d), and normalisation by explicitly forming sum(v^2).")
    w("//")
    w("//  This file is COMMITTED and reviewed rather than regenerated at build time (PLAN-2.2 P22).")
    w("//  A change to the solver plus a matching change to the generator would otherwise agree")
    w("//  silently, with nothing in the diff. Run the O-Octagon-dbap-fixture-check target (or")
    w("//  `gen_dbap_reference.py --output <this file> --check`) to prove the two still agree.")
    w("//")
    w("//  SELF-CONTAINED (CONTEXT-2.2 D3): the speaker array, rigScale and every solver input used")
    w("//  are carried here, so the C++ probe feeds the solver exactly what the oracle saw. There is")
    w("//  no mirrored coordinate table anywhere, so there is nothing that can drift.")
    w("//")
    w("//  Every input below is exactly representable as a float32, so the deviation probe Y measures")
    w("//  is the arithmetic's own error and not input rounding.")
    w("//" + "=" * 98)
    w("#pragma once")
    w("")
    w("namespace dbap_fixture")
    w("{")
    w("")
    w("inline constexpr int kFixtureVersion = %d;" % FIXTURE_VERSION)
    w("inline constexpr int kNumSpeakers    = %d;" % NUM_SPEAKERS)
    w("")
    w("/// RMS 3-D speaker radius about the centroid, computed by the generator from the array below.")
    w("inline constexpr double kRigScale = %s;" % d(scale))
    w("")
    w("/// Speaker positions in metres. Heights are GRADED on purpose — a uniform z would hide a")
    w("/// dropped (z_i - z_s) term in DSP-01's acceptance test.")
    w("inline constexpr double kSpeakers[kNumSpeakers][3] =")
    w("{")

    for (sx, sy, sz) in speakers:
        w("    { %s, %s, %s }," % (d(sx), d(sy), d(sz)))

    w("};")
    w("")
    w("struct Case")
    w("{")
    w("    const char* name;")
    w("    double      w[kNumSpeakers];   ///< per-speaker weights")
    w("    double      rolloff;           ///< dB per distance doubling")
    w("    double      blur;              ///< 0..1")
    w("    double      src[3];            ///< solve position, metres")
    w("    double      v[kNumSpeakers];   ///< EXPECTED gains")
    w("};")
    w("")
    w("inline constexpr Case kCases[] =")
    w("{")

    for case in cases:
        w("    {")
        w('        "%s",' % case["name"])
        w("        { %s }," % ", ".join(d(x) for x in case["w"]))
        w("        %s, %s," % (d(case["rolloff"]), d(case["blur"])))
        w("        { %s }," % ", ".join(d(x) for x in case["src"]))
        w("        { %s }," % ", ".join(d(x) for x in case["v"]))
        w("    },")

    w("};")
    w("")
    w("inline constexpr int kNumCases = static_cast<int> (sizeof (kCases) / sizeof (kCases[0]));")
    w("")
    w("static_assert (kNumCases > 0,")
    w('               "A DBAP fixture with zero cases would make probe Y pass VACUOUSLY.");')
    w("")
    w("} // namespace dbap_fixture")
    w("")

    return "\n".join(out)


# ══════════════════════════════════════════════════════════════════════════════════════════════════

def main():
    parser = argparse.ArgumentParser(description="Generate the committed DBAP reference fixture.")
    parser.add_argument("--output", required=True, help="path to DbapReferenceFixture.h")
    parser.add_argument("--check", action="store_true",
                        help="do not write; compare the committed file against a fresh generation "
                             "and exit non-zero if they differ")
    args = parser.parse_args()

    speakers = [tuple(f32(c) for c in p) for p in SPEAKERS_M]

    if len(speakers) != NUM_SPEAKERS:
        die("expected %d speakers, got %d" % (NUM_SPEAKERS, len(speakers)))

    scale = f32(rig_scale(speakers))

    if not math.isfinite(scale) or scale <= 0.0:
        die("rigScale computed as %r — refusing to emit a fixture whose blur mapping is degenerate"
            % scale)

    cases = build_cases(speakers, scale)
    text = emit(speakers, scale, cases)

    path = Path(args.output)

    if args.check:
        if not path.exists():
            die("--check: %s does not exist. The committed fixture is missing." % path)

        committed = path.read_text(encoding="utf-8")

        if committed != text:
            sys.stderr.write(
                "gen_dbap_reference.py: FAILED --check\n"
                "  %s does not match what this generator produces.\n"
                "  Either the generator was edited without regenerating the fixture, or the fixture\n"
                "  was hand-edited. Regenerate, READ THE DIFF, and commit it.\n" % path)
            sys.exit(1)

        sys.stdout.write("gen_dbap_reference.py: --check OK — %d cases, %s\n"
                         % (len(cases), path))
        return

    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text, encoding="utf-8")

    sys.stdout.write("gen_dbap_reference.py: wrote %d cases to %s (rigScale %.6f m)\n"
                     % (len(cases), path, scale))


if __name__ == "__main__":
    main()
