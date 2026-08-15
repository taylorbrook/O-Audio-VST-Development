#!/usr/bin/env python3
# This file is part of O-Octagon, an Ouaricon Audio plugin.
# Copyright (C) 2026  Ouaricon Audio
# SPDX-License-Identifier: AGPL-3.0-or-later
"""
Layer 2 of the O-Octagon channel-map test strategy — the source-parsed golden.

COMPAT-03's acceptance criterion is unusually specific: *"Test fails loudly if JUCE's enum-bit
order changes (asserted against PARSED SOURCE, not a mirrored constant)."*  A fixture that mirrors
the value it checks drifts with it and keeps passing forever, so the expectation is re-derived from
JUCE's own source on every build and a committed SHA-256 is compared against it.

What this script does:

  1. parses the `enum ChannelType` block of juce_AudioChannelSet.h into name -> int;
  2. parses the create7point1() / create7point1SDDS() / create5point1point2() initializer lists
     out of juce_AudioChannelSet.cpp;
  3. asserts every name those lists reference actually resolved;
  4. sorts each list by parsed enum value — which IS the buffer order, because AudioChannelSet is
     a bitset and getChannelIndexForType() counts set bits in ascending order;
  5. emits JuceChannelOrderGolden.h containing the three derived orders and a SHA-256.

THREE TRAPS THIS PARSER EXISTS TO AVOID (RESEARCH-2.1 G3):

  * Declaration order is NOT value order.  `topSideLeft = 28` and `topSideRight = 29` are declared
    BEFORE `ambisonicACN0..3 = 24..27`, and topSideLeft/Right are precisely the two types
    create5point1point2() uses.  Assigning values by line position would silently produce a wrong
    golden that still looks plausible.

  * The enum contains ALIASES with identifier right-hand sides — `surround = centreSurround`,
    `ambisonicW = ambisonicACN0`, and three more.  Parsing `= <int>` only and skipping the rest is
    fine; mis-parsing them as zero is not.

  * A silently-missing name drops a channel from the golden, and the golden's entire job is to be
    independent of the implementation it checks.  Step 3 is therefore fatal, not a warning.

THE SHA COVERS THE PARSED DATA, NOT THE EMITTED FILE TEXT.  Hashing the output would make a comment
reflow churn the checksum, which trains everyone to re-bless it without reading — the exact
failure mode the gate exists to prevent.

Exits non-zero on any parse failure and never writes a header with zero entries: a vacuous golden
is worse than no golden, because it reports green.
"""

import argparse
import hashlib
import re
import sys
from pathlib import Path

# The three 8-channel containers isBusesLayoutSupported() accepts. Order here is the order they are
# emitted and hashed in — it is part of the canonical serialisation and must not be shuffled.
TARGET_SETS = ["create7point1", "create7point1SDDS", "create5point1point2"]

EXPECTED_CHANNELS = 8


def die(message):
    sys.stderr.write("gen_juce_channel_order.py: ERROR: %s\n" % message)
    sys.exit(1)


def strip_comments(text):
    """Removes /* ... */ and // ... so a commented-out entry can never be parsed as a live one."""
    text = re.sub(r"/\*.*?\*/", " ", text, flags=re.DOTALL)
    text = re.sub(r"//[^\n]*", " ", text)
    return text


def parse_channel_type_enum(header_path):
    """Returns {name: int} for every ChannelType with a resolvable integer value."""
    source = header_path.read_text(encoding="utf-8", errors="replace")

    match = re.search(r"\benum\s+ChannelType\s*\{", source)
    if not match:
        die("could not find 'enum ChannelType {' in %s" % header_path)

    # Walk braces from the opening one so a nested brace (none today, but do not depend on that)
    # cannot terminate the block early.
    start = match.end() - 1
    depth = 0
    end = None

    for i in range(start, len(source)):
        if source[i] == "{":
            depth += 1
        elif source[i] == "}":
            depth -= 1
            if depth == 0:
                end = i
                break

    if end is None:
        die("unterminated 'enum ChannelType' block in %s" % header_path)

    body = strip_comments(source[start + 1:end])

    literals = {}   # name -> int
    aliases = {}    # name -> name

    # ONE ENTRY PER `name = value`, matched wherever it appears. Never by line position.
    for name, value in re.findall(r"(\w+)\s*=\s*([A-Za-z_]\w*|-?\d+)", body):
        if re.fullmatch(r"-?\d+", value):
            literals[name] = int(value)
        else:
            aliases[name] = value

    if not literals:
        die("parsed zero integer-valued ChannelType entries from %s — the enum layout has changed "
            "and this parser needs updating. Refusing to emit a vacuous golden." % header_path)

    # Resolve identifier-valued entries to a fixpoint. Chains are one deep today
    # (surround = centreSurround); the loop costs nothing and removes the assumption.
    for _ in range(len(aliases) + 1):
        progressed = False
        for name, target in list(aliases.items()):
            if target in literals:
                literals[name] = literals[target]
                del aliases[name]
                progressed = True
        if not progressed:
            break

    # Unresolved aliases are NOT fatal here — an alias we do not reference is none of our business.
    # Step 3 in parse_set_membership() is where an unresolved name we DO reference becomes fatal.
    return literals


def parse_set_membership(cpp_path):
    """Returns {set_name: [type names, in initializer-list order]}."""
    source = strip_comments(cpp_path.read_text(encoding="utf-8", errors="replace"))
    membership = {}

    for set_name in TARGET_SETS:
        # `\s*\(\s*\)` after the name is what stops create7point1 from also matching
        # create7point1SDDS.
        pattern = (r"AudioChannelSet::" + re.escape(set_name) +
                   r"\s*\(\s*\)[^{]*\{[^{]*AudioChannelSet\s*\(\s*\{([^}]*)\}")
        match = re.search(pattern, source)

        if not match:
            die("could not find the initializer list for AudioChannelSet::%s() in %s"
                % (set_name, cpp_path))

        names = [n.strip() for n in match.group(1).split(",") if n.strip()]

        if len(names) != EXPECTED_CHANNELS:
            die("AudioChannelSet::%s() lists %d channel(s), expected %d. JUCE's set membership has "
                "CHANGED — this is exactly what the gate is for, but it must be re-blessed by a "
                "human, not absorbed here." % (set_name, len(names), EXPECTED_CHANNELS))

        membership[set_name] = names

    return membership


def canonical_serialisation(values, membership, orders):
    """The bytes the SHA-256 is taken over.

    PARSED DATA ONLY — every referenced name with its value, then the three derived orders. No
    file text, no timestamps, no paths, no comments. A reflow of JUCE's source that changes nothing
    semantic must not change this hash, and a renumbering that changes everything must.
    """
    lines = []

    referenced = sorted({name for names in membership.values() for name in names})

    for name in referenced:
        lines.append("enum:%s=%d" % (name, values[name]))

    for set_name in TARGET_SETS:
        lines.append("set:%s:%s" % (set_name, ",".join(str(v) for v in orders[set_name])))

    return ("\n".join(lines) + "\n").encode("utf-8")


def emit_header(out_path, orders, sha):
    body = []
    body.append("// GENERATED FILE — DO NOT EDIT, DO NOT COMMIT.")
    body.append("//")
    body.append("// Written by plugins/O-Octagon/tests/tools/gen_juce_channel_order.py at build")
    body.append("// time, from the JUCE source tree the build is actually compiling against.")
    body.append("// Layer 2 of the channel-map test strategy (COMPAT-03).")
    body.append("//")
    body.append("// Deterministic by construction: no timestamps, no absolute paths, no build")
    body.append("// directory strings. Two runs against the same JUCE produce byte-identical output.")
    body.append("#pragma once")
    body.append("")
    body.append("namespace juce_golden")
    body.append("{")
    body.append("")
    body.append("/** SHA-256 of the canonical serialisation of the PARSED DATA — the referenced")
    body.append("    name->value pairs and the three derived orders. NOT a hash of this file's text.")
    body.append("*/")
    body.append('inline constexpr char kGeneratedChannelOrderSha256[] = "%s";' % sha)
    body.append("")
    body.append("inline constexpr int kNumChannels = %d;" % EXPECTED_CHANNELS)
    body.append("")

    for set_name in TARGET_SETS:
        pairs = orders[set_name + ":pairs"]
        body.append("// AudioChannelSet::%s() — enum values in ASCENDING order, which IS the"
                    % set_name)
        body.append("// buffer order (juce_AudioChannelSet.cpp getChannelIndexForType).")
        body.append("inline constexpr int k%s[%d] = { %s };"
                    % (set_name[0].upper() + set_name[1:], EXPECTED_CHANNELS,
                       ", ".join(str(v) for _, v in pairs)))
        body.append("inline constexpr const char* k%sNames[%d] = { %s };"
                    % (set_name[0].upper() + set_name[1:], EXPECTED_CHANNELS,
                       ", ".join('"%s"' % n for n, _ in pairs)))
        body.append("")

    body.append("} // namespace juce_golden")
    body.append("")

    text = "\n".join(body)

    # Write only if the content differs, so an unchanged golden does not churn the build graph.
    if out_path.exists() and out_path.read_text(encoding="utf-8") == text:
        return

    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(text, encoding="utf-8")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--juce-modules", required=True,
                        help="path to JUCE's modules directory (JUCE_MODULES_DIR)")
    parser.add_argument("--output", required=True, help="path of the header to emit")
    parser.add_argument("--print-sha", action="store_true",
                        help="print the SHA-256 to stdout and emit nothing else")
    args = parser.parse_args()

    modules = Path(args.juce_modules)

    if not modules.is_dir():
        die("--juce-modules '%s' is not a directory. JUCE_MODULES_DIR was empty or wrong, and a "
            "golden generated from nothing would make the COMPAT-03 gate pass vacuously." % modules)

    header = modules / "juce_audio_basics" / "buffers" / "juce_AudioChannelSet.h"
    cpp = modules / "juce_audio_basics" / "buffers" / "juce_AudioChannelSet.cpp"

    for path in (header, cpp):
        if not path.is_file():
            die("expected JUCE source file not found: %s" % path)

    values = parse_channel_type_enum(header)
    membership = parse_set_membership(cpp)

    # ── Step 3: every referenced name MUST have resolved ────────────────────────────────────────
    # A name that silently fails to resolve drops a channel from the golden. Fatal, always.
    unresolved = sorted({n for names in membership.values() for n in names if n not in values})

    if unresolved:
        die("these channel type names appear in a create*() initializer list but did not resolve "
            "to an integer while parsing the enum: %s. The golden would be missing a channel."
            % ", ".join(unresolved))

    orders = {}

    for set_name in TARGET_SETS:
        pairs = sorted(((n, values[n]) for n in membership[set_name]), key=lambda p: p[1])
        orders[set_name] = [v for _, v in pairs]
        orders[set_name + ":pairs"] = pairs

        if len(set(orders[set_name])) != EXPECTED_CHANNELS:
            die("AudioChannelSet::%s() resolved to duplicate enum values %s — an alias is being "
                "counted twice and the derived buffer order is meaningless."
                % (set_name, orders[set_name]))

    payload = canonical_serialisation(values, membership, orders)
    sha = hashlib.sha256(payload).hexdigest()

    if args.print_sha:
        sys.stdout.write(sha + "\n")
        return

    emit_header(Path(args.output), orders, sha)


if __name__ == "__main__":
    main()
