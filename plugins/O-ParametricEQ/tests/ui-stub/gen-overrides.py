#!/usr/bin/env python3
# This file is part of O-ParametricEQ, an Ouaricon Audio plugin.
# Copyright (C) 2026  Ouaricon Audio
# SPDX-License-Identifier: AGPL-3.0-or-later
"""
gen-overrides.py -- writes tests/ui-stub/generic-overrides.json (PLAN-3.1 P8).

The generic UI stub (scripts/ui-stub/generic-juce-stub.js) seeds slider ranges
from .planning/params.tsv, which carries no skew (the stub then uses skew 1),
and gives every combo four one-character choices. This file supplies what the
TSV cannot: the 48 frequency / Q sliders' real start / end / skew / interval
and the 72 combos' real choice lists and default indices.

Nothing here is typed by hand. The sources, all generated from the processor:

  .planning/params.tsv       O-ParametricEQ-param-dump (getParameters() walk):
                             the ID set and order, numSteps, defaultNorm,
                             getText at 0 / 1 and at the default.
  tests/golden/params.tsv    O-ParametricEQ-render-harness --dump-params: the
                             live NormalisableRange (start, end, interval,
                             skew), defaultRaw and the choice strings. The
                             harness's layout rows compare it with the live
                             processor on every run. With --harness <binary>
                             the dump is taken live instead and must equal the
                             committed file byte for byte.
  .planning/parameter-spec.md  the contract: cross-check only (choice lists,
                             the six-digit skews).

The golden prints floats with 9 significant digits, which identifies the
float32 exactly; each number is rounded to float32 here and written as that
float's double value -- what WebSliderRelay sends the page (a float range
converted to a JSON double, e.g. skew 0.20321448147296906).

Every cross-check failure exits 1 and writes nothing. Two runs on the same
inputs produce the same bytes.

Usage: python3 tests/ui-stub/gen-overrides.py [--harness <render-harness binary>]
"""

import hashlib
import json
import re
import struct
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
PLUGIN = HERE.parent.parent
DUMP_TSV = PLUGIN / ".planning" / "params.tsv"
GOLDEN_TSV = PLUGIN / "tests" / "golden" / "params.tsv"
SPEC_MD = PLUGIN / ".planning" / "parameter-spec.md"
OUT = HERE / "generic-overrides.json"

EXPECTED = 196
errors = []


def fail(msg):
    errors.append(msg)


def f32(text):
    """The float32 a 9-significant-digit text names, as its double value."""
    return struct.unpack("<f", struct.pack("<f", float(text)))[0]


def rel_path(p):
    return str(p.relative_to(PLUGIN))


def sha256(data):
    return hashlib.sha256(data).hexdigest()


def read_dump(text):
    cols, rows = None, []
    for line in text.split("\n"):
        line = line.rstrip("\r")
        if not line.strip():
            continue
        if line.startswith("#"):
            maybe = [c.strip() for c in line.lstrip("#").strip().split("\t")]
            if "id" in maybe and "defaultNorm" in maybe:
                cols = maybe
            continue
        if cols is None:
            continue
        cells = line.split("\t")
        rows.append({c: (cells[i] if i < len(cells) else "") for i, c in enumerate(cols)})
    return rows


def read_golden(text):
    cols, rows = None, []
    for line in text.split("\n"):
        line = line.rstrip("\r")
        if not line.strip() or line.startswith("#"):
            continue
        cells = line.split("\t")
        if cols is None:
            cols = cells
            continue
        rows.append(dict(zip(cols, cells)))
    return rows


def spec_choices(spec):
    """{'type': [...], 'slope': [...], 'placement': [...]} from the bNN_* sections."""
    out = {}
    for field in ("type", "slope", "placement"):
        m = re.search(r"### bNN_" + field + r"\n(?:.*\n)*?- \*\*Range:\*\* (.*)\n", spec)
        if not m:
            fail(f"parameter-spec.md: no Range line for bNN_{field}")
            continue
        items = re.findall(r'(\d+) (?:"([^"]+)"|([^,]+?))(?:,|$)', m.group(1).strip())
        out[field] = [(q or u).strip() for _, q, u in items]
    return out


def spec_skews(spec):
    out = {}
    for field in ("freq", "q"):
        m = re.search(r"### bNN_" + field + r"\n(?:.*\n)*?- \*\*Skew Factor:\*\* .*-> skew ([0-9.]+)", spec)
        if not m:
            fail(f"parameter-spec.md: no skew for bNN_{field}")
            continue
        out[field] = float(m.group(1))
    return out


def num(text):
    m = re.search(r"-?\d+(?:\.\d+)?", text or "")
    return float(m.group(0)) if m else None


def main(argv):
    harness = None
    if len(argv) == 3 and argv[1] == "--harness":
        harness = Path(argv[2])
    elif len(argv) != 1:
        print(__doc__.split("Usage:")[1].strip(), file=sys.stderr)
        return 2

    dump_bytes = DUMP_TSV.read_bytes()
    golden_bytes = GOLDEN_TSV.read_bytes()

    if harness is not None:
        live = subprocess.run([str(harness), "--dump-params"], check=True, capture_output=True).stdout
        if live != golden_bytes:
            fail("the live --dump-params output differs from tests/golden/params.tsv")

    dump = read_dump(dump_bytes.decode("utf-8"))
    golden = read_golden(golden_bytes.decode("utf-8"))
    spec = SPEC_MD.read_text(encoding="utf-8")

    if len(dump) != EXPECTED:
        fail(f"params.tsv has {len(dump)} rows, expected {EXPECTED}")
    if len(golden) != EXPECTED:
        fail(f"golden params.tsv has {len(golden)} rows, expected {EXPECTED}")

    choices_spec = spec_choices(spec)
    skews_spec = spec_skews(spec)

    sliders, combos = {}, {}
    counts = {"Float": 0, "Bool": 0, "Choice": 0}

    for d, g in zip(dump, golden):
        pid = d["id"]
        if pid != g["id"]:
            fail(f"order differs: dump {pid} vs golden {g['id']}")
            continue

        kind = g["type"]
        counts[kind] = counts.get(kind, 0) + 1

        if abs(float(d["defaultNorm"]) - float(g["defaultNorm"])) > 1e-6:
            fail(f"{pid}: defaultNorm dump {d['defaultNorm']} vs golden {g['defaultNorm']}")

        if kind == "Float":
            start, end = f32(g["start"]), f32(g["end"])
            lo, hi = num(d["textAtMin"]), num(d["textAtMax"])
            if lo is None or hi is None or abs(lo - start) > 1e-6 * max(1.0, abs(start)) \
                    or abs(hi - end) > 1e-6 * max(1.0, abs(end)):
                fail(f"{pid}: getText at 0 / 1 ({d['textAtMin']}, {d['textAtMax']}) vs range {start} .. {end}")

            field = pid.split("_", 1)[1] if pid.startswith("b") else None
            if field not in ("freq", "q"):
                continue   # linear: the TSV path's skew 1 is already right

            skew = f32(g["skew"])
            if field in skews_spec and abs(skew - skews_spec[field]) > 1e-6:
                fail(f"{pid}: skew {skew} vs parameter-spec.md {skews_spec[field]}")
            if abs(skew - 1.0) < 1e-9:
                fail(f"{pid}: skew 1 -- a frequency / Q slider must be skewed")

            entry = {
                "start": start,
                "end": end,
                "skew": skew,
                "interval": f32(g["interval"]),
                "def": f32(g["defaultRaw"]),
                "label": d.get("label", ""),
            }
            dt = num(d["defaultText"])
            if dt is None or abs(dt - entry["def"]) > 1e-4 * max(1.0, abs(entry["def"])):
                fail(f"{pid}: defaultText {d['defaultText']} vs defaultRaw {entry['def']}")
            sliders[pid] = entry

        elif kind == "Choice":
            choices = g["choices"].split("|")
            index = int(round(float(g["defaultRaw"])))
            if int(d["numSteps"]) != len(choices):
                fail(f"{pid}: numSteps {d['numSteps']} vs {len(choices)} choices")
            if d["textAtMin"] != choices[0] or d["textAtMax"] != choices[-1] or d["defaultText"] != choices[index]:
                fail(f"{pid}: getText ({d['textAtMin']}, {d['textAtMax']}, {d['defaultText']}) vs choices {choices} / default {index}")
            field = pid.split("_", 1)[1]
            if field in choices_spec and choices_spec[field] != choices:
                fail(f"{pid}: choices {choices} vs parameter-spec.md {choices_spec[field]}")
            combos[pid] = {"choices": choices, "def": index}

    if counts.get("Float") != 74 or counts.get("Bool") != 50 or counts.get("Choice") != 72:
        fail(f"type counts {counts}, expected Float 74 / Bool 50 / Choice 72")
    if len(sliders) != 48:
        fail(f"{len(sliders)} frequency / Q sliders, expected 48")
    if len(combos) != 72:
        fail(f"{len(combos)} combos, expected 72")

    if errors:
        for e in errors:
            print("gen-overrides: " + e, file=sys.stderr)
        print(f"gen-overrides: {len(errors)} cross-check failure(s); nothing written", file=sys.stderr)
        return 1

    doc = {
        "_why": (
            "GENERATED by tests/ui-stub/gen-overrides.py -- do not edit; re-run it. "
            "The generic stub seeds sliders from .planning/params.tsv, which has no skew (the stub "
            "uses 1), and gives every combo four one-character choices. With skew 1 the page's "
            "typed-entry write ((hz - start) / (end - start)) ^ skew lands on the wrong normalised "
            "value and the frequency / Q labels and readouts are measured at values the plugin never "
            "shows, so the label and typed-entry checks would be vacuous. These are the 48 frequency / "
            "Q sliders' real ranges and the 72 combos' real choice lists and default indices, from "
            "the processor's own dumps (float32 values written as the doubles the relays send)."
        ),
        "_source": {
            rel_path(DUMP_TSV): "sha256:" + sha256(dump_bytes),
            rel_path(GOLDEN_TSV): "sha256:" + sha256(golden_bytes),
        },
        "sliders": sliders,
        "combos": combos,
    }

    text = json.dumps(doc, indent=2, ensure_ascii=False) + "\n"
    OUT.write_text(text, encoding="utf-8", newline="\n")
    print(f"gen-overrides: wrote {rel_path(OUT)} ({len(sliders)} sliders, {len(combos)} combos)")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
