#!/usr/bin/env python3
#
#  This file is part of the Ouaricon Audio plugin suite.
#  Copyright (C) 2026  Ouaricon Audio
#
#  SPDX-License-Identifier: AGPL-3.0-or-later
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU Affero General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU Affero General Public License for more details.
#
#  You should have received a copy of the GNU Affero General Public License
#  along with this program.  If not, see <https://www.gnu.org/licenses/>.
#
"""Tie the committed parameter golden to parameter-spec.md (Stage 1, P5).

The golden (params.tsv) is a dump of the built plugin, so on its own it only
proves the layout has not moved. This script checks it against the contract,
independently of the C++:

  * the "Full Registration Table (196)" of parameter-spec.md gives index, ID,
    host-visible name, type and group for every row;
  * the KINDS table below is transcribed by hand from the spec's "Parameter
    Definitions" section and gives ranges, skews, defaults and choice strings.

A failure here is a layout bug in the plugin. Never edit the expectations or
the spec to make it pass.

Usage:
    check_spec.py [--spec <parameter-spec.md>] [--golden <params.tsv>]

Exit 0 and one "OK:" line on success; exit 1 with one "FAIL" line per failure.
Python 3 standard library only.
"""

import argparse
import math
import re
import struct
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
DEFAULT_SPEC = HERE.parent.parent / ".planning" / "parameter-spec.md"
DEFAULT_GOLDEN = HERE / "params.tsv"

COLUMNS = ["index", "id", "name", "group", "type", "versionHint", "start",
           "end", "interval", "skew", "defaultNorm", "defaultRaw", "numSteps",
           "choices", "automatable"]

EXPECTED_TOTAL = 196
EXPECTED_COUNTS = {"Bool": 50, "Choice": 72, "Float": 74}
EXPECTED_GROUPS = ["global"] + ["band%02d" % n for n in range(1, 25)]

SKEW_TOL = 1e-5        # freq / q skew against the setSkewForCentre formula
RAW_REL_TOL = 2e-4     # freq / q raw default (float pow round trip is inexact)
CHOICE_NORM_TOL = 1e-6  # choice default normalised, index / (count - 1)

TYPE_CHOICES = ["Bell", "Low Shelf", "High Shelf", "Low Cut", "High Cut",
                "Notch", "Band Pass", "Tilt Shelf"]
SLOPE_CHOICES = ["6 dB/oct", "12 dB/oct", "18 dB/oct", "24 dB/oct",
                 "36 dB/oct", "48 dB/oct", "72 dB/oct", "96 dB/oct"]
PLACEMENT_CHOICES = ["Stereo", "Left", "Right", "Mid", "Side"]

# Transcribed from parameter-spec.md "Parameter Definitions".
#   name    host-visible name; "{n}" is the unpadded band number
#   centre  setSkewForCentre argument; None = linear (skew exactly 1)
#   default raw default (Float), index (Choice), 0 / 1 (Bool)
GLOBAL_KINDS = {
    "bypass":      {"type": "Bool", "name": "Bypass", "default": 0},
    "output_gain": {"type": "Float", "name": "Output Gain",
                    "start": -36.0, "end": 36.0, "default": 0.0,
                    "norm": 0.5, "centre": None},
    "gain_scale":  {"type": "Float", "name": "Gain Scale",
                    "start": 0.0, "end": 200.0, "default": 100.0,
                    "norm": 0.5, "centre": None},
    "auto_gain":   {"type": "Bool", "name": "Auto Gain", "default": 0},
}

BAND_KINDS = {
    "enabled":   {"type": "Bool", "name": "B{n} On", "default": 0},
    "bypass":    {"type": "Bool", "name": "B{n} Bypass", "default": 0},
    "type":      {"type": "Choice", "name": "B{n} Type",
                  "choices": TYPE_CHOICES, "default": 0},
    "freq":      {"type": "Float", "name": "B{n} Freq",
                  "start": 10.0, "end": 30000.0, "default": 1000.0,
                  "norm": 0.5, "centre": 1000.0},
    "gain":      {"type": "Float", "name": "B{n} Gain",
                  "start": -30.0, "end": 30.0, "default": 0.0,
                  "norm": 0.5, "centre": None},
    "q":         {"type": "Float", "name": "B{n} Q",
                  "start": 0.025, "end": 40.0, "default": 1.0,
                  "norm": 0.5, "centre": 1.0},
    "slope":     {"type": "Choice", "name": "B{n} Slope",
                  "choices": SLOPE_CHOICES, "default": 1},
    "placement": {"type": "Choice", "name": "B{n} Placement",
                  "choices": PLACEMENT_CHOICES, "default": 0},
}

BAND_ID = re.compile(r"^b(\d{2})_([a-z_]+)$")
SPEC_ROW = re.compile(
    r"^\|\s*(\d+)\s*\|\s*`([^`]+)`\s*\|\s*(.*?)\s*\|\s*(.*?)\s*\|\s*(.*?)\s*\|\s*$")
SPEC_TYPE = re.compile(r"^(Bool|Float|Choice)(?:\s*\((\d+)\))?$")


def f32(x):
    """Round a Python float to the nearest IEEE single, as the plugin holds it."""
    return struct.unpack("<f", struct.pack("<f", x))[0]


class Failures:
    def __init__(self):
        self.items = []

    def add(self, who, message):
        self.items.append("FAIL [%s] %s" % (who, message))


def resolve_kind(param_id):
    """Return (kind dict, expected name, expected group) or None for an unknown ID."""
    if param_id in GLOBAL_KINDS:
        kind = GLOBAL_KINDS[param_id]
        return kind, kind["name"], "global"
    m = BAND_ID.match(param_id)
    if m and m.group(2) in BAND_KINDS and 1 <= int(m.group(1)) <= 24:
        kind = BAND_KINDS[m.group(2)]
        band = int(m.group(1))
        return kind, kind["name"].format(n=band), "band%02d" % band
    return None


def parse_spec(path, fails):
    """Parse the Full Registration Table. Returns a list of row dicts."""
    try:
        text = path.read_text(encoding="utf-8")
    except OSError as e:
        fails.add("spec", "cannot read %s: %s" % (path, e))
        return []

    rows = []
    in_table = False
    found = False
    for line in text.splitlines():
        if line.startswith("## "):
            in_table = line.startswith("## Full Registration Table")
            found = found or in_table
            continue
        if not in_table or not line.startswith("|"):
            continue
        if re.match(r"^\|\s*#\s*\|", line) or re.match(r"^\|[\s:|-]+\|\s*$", line):
            continue  # header and separator
        m = SPEC_ROW.match(line)
        if not m:
            fails.add("spec", "unparseable registration row: %r" % line)
            continue
        index, param_id, name, type_cell, group = m.groups()
        tm = SPEC_TYPE.match(type_cell)
        if not tm:
            fails.add(param_id, "spec: unknown type %r" % type_cell)
            continue
        rows.append({
            "index": int(index), "id": param_id, "name": name,
            "type": tm.group(1),
            "count": int(tm.group(2)) if tm.group(2) else None,
            "group": group,
        })

    if not found:
        fails.add("spec", "no '## Full Registration Table' section in %s" % path)
        return rows

    if len(rows) != EXPECTED_TOTAL:
        fails.add("spec", "registration table has %d rows, expected %d"
                  % (len(rows), EXPECTED_TOTAL))
    seen = {}
    for pos, row in enumerate(rows):
        if row["index"] != pos:
            fails.add(row["id"], "spec: index %d at table position %d "
                      "(indices must be contiguous 0-%d)"
                      % (row["index"], pos, EXPECTED_TOTAL - 1))
        if row["id"] in seen:
            fails.add(row["id"], "spec: duplicate ID (rows %d and %d)"
                      % (seen[row["id"]], pos))
        seen[row["id"]] = pos

        # The table must agree with the definitions transcribed above.
        resolved = resolve_kind(row["id"])
        if resolved is None:
            fails.add(row["id"], "spec: ID is not in the checker's kind table")
            continue
        kind, name, group = resolved
        if row["type"] != kind["type"]:
            fails.add(row["id"], "spec: table type %s, definitions say %s"
                      % (row["type"], kind["type"]))
        if row["name"] != name:
            fails.add(row["id"], "spec: table name %r, definitions say %r"
                      % (row["name"], name))
        if row["group"] != group:
            fails.add(row["id"], "spec: table group %r, expected %r"
                      % (row["group"], group))
        if kind["type"] == "Choice" and row["count"] != len(kind["choices"]):
            fails.add(row["id"], "spec: table says Choice (%s), definitions list %d"
                      % (row["count"], len(kind["choices"])))
    return rows


def parse_golden(path, fails):
    """Read the golden in binary so a CRLF cannot hide. Returns a list of cell dicts."""
    try:
        raw = path.read_bytes()
    except OSError as e:
        fails.add("golden", "cannot read %s: %s" % (path, e))
        return []

    cr = raw.count(b"\r")
    if cr:
        fails.add("golden", "%d carriage return(s) in the file; line endings must be LF" % cr)
    try:
        text = raw.decode("utf-8")
    except UnicodeDecodeError as e:
        fails.add("golden", "not valid UTF-8: %s" % e)
        return []

    if not text.endswith("\n"):
        fails.add("golden", "file does not end with a newline")
    lines = text.replace("\r\n", "\n").replace("\r", "\n").split("\n")
    if lines and lines[-1] == "":
        lines.pop()

    if len(lines) < 2:
        fails.add("golden", "expected a '#' line, a header line and %d rows; got %d line(s)"
                  % (EXPECTED_TOTAL, len(lines)))
        return []
    if not lines[0].startswith("#"):
        fails.add("golden", "first line must start with '#': %r" % lines[0])
    header = lines[1].split("\t")
    if header != COLUMNS:
        fails.add("golden", "header is %r, expected %r" % (header, COLUMNS))
        return []

    rows = []
    for lineno, line in enumerate(lines[2:], start=3):
        cells = line.split("\t")
        if len(cells) != len(COLUMNS):
            who = cells[1] if len(cells) > 1 else "golden"
            fails.add(who, "golden line %d has %d cells, expected %d"
                      % (lineno, len(cells), len(COLUMNS)))
            continue
        row = dict(zip(COLUMNS, cells))
        row["_line"] = lineno
        rows.append(row)
    return rows


def number(cell):
    try:
        value = float(cell)
    except ValueError:
        return None
    return value if math.isfinite(value) else None


def check_exact(fails, pid, column, cell, expected, optional=False):
    """Equal as IEEE singles (the golden prints floats with %.9g)."""
    if cell == "-":
        if not optional:
            fails.add(pid, "%s: expected %.9g, golden is empty ('-')" % (column, expected))
        return
    value = number(cell)
    if value is None:
        fails.add(pid, "%s: %r is not a number (expected %.9g)" % (column, cell, expected))
    elif f32(value) != f32(expected):
        fails.add(pid, "%s: expected %.9g, golden %s" % (column, f32(expected), cell))


def check_close(fails, pid, column, cell, expected, tol, relative=False, optional=False):
    if cell == "-":
        if not optional:
            fails.add(pid, "%s: expected %.9g, golden is empty ('-')" % (column, expected))
        return
    value = number(cell)
    if value is None:
        fails.add(pid, "%s: %r is not a number (expected %.9g)" % (column, cell, expected))
        return
    error = abs(value - expected)
    if relative:
        error /= abs(expected)
    if error > tol:
        fails.add(pid, "%s: expected %.9g within %g%s, golden %s"
                  % (column, expected, tol, " relative" if relative else "", cell))


def check_literal(fails, pid, column, cell, expected):
    if cell != expected:
        fails.add(pid, "%s: expected %r, golden %r" % (column, expected, cell))


def check_values(fails, pid, kind, g):
    """Ranges, skew, defaults, steps and choices for one golden row."""
    check_literal(fails, pid, "versionHint", g["versionHint"], "1")
    check_literal(fails, pid, "automatable", g["automatable"], "1")

    if kind["type"] == "Float":
        start, end, default = kind["start"], kind["end"], kind["default"]
        check_exact(fails, pid, "start", g["start"], start)
        check_exact(fails, pid, "end", g["end"], end)
        check_exact(fails, pid, "interval", g["interval"], 0.0)
        if kind["centre"] is None:
            check_exact(fails, pid, "skew", g["skew"], 1.0)
            check_exact(fails, pid, "defaultRaw", g["defaultRaw"], default)
        else:
            skew = math.log(0.5) / math.log((kind["centre"] - start) / (end - start))
            check_close(fails, pid, "skew", g["skew"], skew, SKEW_TOL)
            check_close(fails, pid, "defaultRaw", g["defaultRaw"], default,
                        RAW_REL_TOL, relative=True)
        check_exact(fails, pid, "defaultNorm", g["defaultNorm"], kind["norm"])
        check_literal(fails, pid, "choices", g["choices"], "-")
        return

    # Bool and Choice: the range columns are checked only when the harness
    # fills them; JUCE gives both a 0 .. count-1 range, interval 1, skew 1.
    if kind["type"] == "Bool":
        count, default = 2, kind["default"]
        if g["choices"] not in ("-", "Off|On"):
            fails.add(pid, "choices: expected '-' (or 'Off|On'), golden %r" % g["choices"])
        check_exact(fails, pid, "defaultNorm", g["defaultNorm"], float(default))
    else:
        choices = kind["choices"]
        count, default = len(choices), kind["default"]
        got = g["choices"].split("|") if g["choices"] != "-" else []
        if len(got) != count:
            fails.add(pid, "choices: expected %d strings, golden has %d"
                      % (count, len(got)))
        if got != choices:
            fails.add(pid, "choices: expected %r, golden %r"
                      % ("|".join(choices), g["choices"]))
        check_close(fails, pid, "defaultNorm", g["defaultNorm"],
                    default / (count - 1), CHOICE_NORM_TOL)

    check_exact(fails, pid, "defaultRaw", g["defaultRaw"], float(default))
    check_exact(fails, pid, "start", g["start"], 0.0, optional=True)
    check_exact(fails, pid, "end", g["end"], float(count - 1), optional=True)
    check_exact(fails, pid, "interval", g["interval"], 1.0, optional=True)
    check_exact(fails, pid, "skew", g["skew"], 1.0, optional=True)
    if g["numSteps"] != "-":
        check_literal(fails, pid, "numSteps", g["numSteps"], str(count))


def compare(spec_rows, golden_rows, fails):
    """Returns (type counts, ordered distinct groups) seen in the golden."""
    if len(golden_rows) != EXPECTED_TOTAL:
        fails.add("golden", "%d parameter rows, expected %d"
                  % (len(golden_rows), EXPECTED_TOTAL))

    counts = {"Bool": 0, "Choice": 0, "Float": 0}
    groups = []
    seen = {}
    for pos, g in enumerate(golden_rows):
        pid = g["id"]
        if "_dyn_" in pid:
            fails.add(pid, "reserved dynamic-EQ ID is registered (golden line %d)" % g["_line"])
        if pid in seen:
            fails.add(pid, "duplicate ID in the golden (lines %d and %d)"
                      % (seen[pid], g["_line"]))
        seen[pid] = g["_line"]
        if g["index"] != str(pos):
            fails.add(pid, "index: golden %r at row position %d" % (g["index"], pos))
        if not groups or groups[-1] != g["group"]:
            groups.append(g["group"])
        golden_type = g["type"].strip().lower()
        for name in counts:
            if golden_type == name.lower():
                counts[name] += 1

        if pos >= len(spec_rows):
            fails.add(pid, "golden row %d has no row in the spec table" % pos)
            continue
        s = spec_rows[pos]
        sid = s["id"]
        if pid != sid:
            fails.add(sid, "id: spec row %d is %r, golden has %r" % (pos, sid, pid))
            continue  # the remaining cells belong to some other parameter
        if g["name"] != s["name"]:
            fails.add(pid, "name: expected %r, golden %r" % (s["name"], g["name"]))
        if golden_type != s["type"].lower():
            fails.add(pid, "type: expected %s, golden %r" % (s["type"], g["type"]))
        if g["group"] != s["group"]:
            fails.add(pid, "group: expected %r, golden %r" % (s["group"], g["group"]))

        resolved = resolve_kind(pid)
        if resolved is not None:  # an unknown ID was already reported by parse_spec
            check_values(fails, pid, resolved[0], g)

    for s in spec_rows[len(golden_rows):]:
        fails.add(s["id"], "spec row %d is missing from the golden" % s["index"])

    if golden_rows:
        if counts != EXPECTED_COUNTS:
            fails.add("golden", "type counts %d Bool / %d Choice / %d Float, expected 50 / 72 / 74"
                      % (counts["Bool"], counts["Choice"], counts["Float"]))
        if groups != EXPECTED_GROUPS:
            fails.add("golden", "groups in order are %r, expected global, band01 ... band24 "
                      "(%d distinct runs, expected 25)" % (groups, len(groups)))
    return counts, groups


def main(argv=None):
    parser = argparse.ArgumentParser(
        description="Check tests/golden/params.tsv against parameter-spec.md.")
    parser.add_argument("--spec", type=Path, default=DEFAULT_SPEC,
                        help="parameter-spec.md (default: %(default)s)")
    parser.add_argument("--golden", type=Path, default=DEFAULT_GOLDEN,
                        help="params.tsv (default: %(default)s)")
    args = parser.parse_args(argv)

    fails = Failures()
    spec_rows = parse_spec(args.spec, fails)
    golden_rows = parse_golden(args.golden, fails)
    counts, groups = compare(spec_rows, golden_rows, fails)

    if fails.items:
        for item in fails.items:
            print(item)
        print("%d FAIL (spec %s, golden %s)" % (len(fails.items), args.spec, args.golden))
        return 1

    print("OK: %d params (%d Bool / %d Choice / %d Float), %d groups"
          % (len(golden_rows), counts["Bool"], counts["Choice"], counts["Float"],
             len(groups)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
