#!/usr/bin/env python3
"""Add AGPL-3.0 notice headers to Ouaricon-authored source files.

The repository is licensed AGPL-3.0 (see LICENSE, and PUBLIC-RELEASE-READINESS.md
section 5.2 for why that horn of JUCE's dual licence was elected). The FSF advises
attaching the notice to the start of each source file, since that is what most
effectively states the exclusion of warranty.

Idempotent and re-runnable: files that already carry the notice are skipped, so
this can be run again after adding a plugin.

CRITICAL — this must never touch third-party code. Redistributing JUCE source is
permitted under AGPLv3, but stamping OUR copyright onto JUCE's files would be a
false attribution. The exclusion list below is derived from an audit of every
candidate file; see --audit to re-verify it before trusting a run.

Usage:
    python3 scripts/add-agpl-headers.py --dry-run     # report, change nothing
    python3 scripts/add-agpl-headers.py --audit       # re-verify exclusions hold
    python3 scripts/add-agpl-headers.py --apply       # write headers
"""

from __future__ import annotations

import argparse
import re
import subprocess
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent

COPYRIGHT_HOLDER = "Ouaricon Audio"
COPYRIGHT_YEAR = "2026"

# Roots we sweep. Everything outside these is out of scope by construction.
ROOTS = ("plugins", "modules")

EXTENSIONS = {".cpp", ".h", ".hpp", ".mm", ".c", ".cc", ".js", ".css", ".html"}

# --- Exclusions -------------------------------------------------------------
# Each entry is a substring test against the repo-relative POSIX path.
EXCLUDE_SUBSTRINGS = (
    "/js/juce/",        # JUCE-shipped WebView JS — Raw Material Software's code
    "/.planning/",      # planning artifacts and throwaway mockups, not shipped source
    "app.bundle.js",    # generated webpack bundle (single 220k-char line)
)

# Any file containing one of these markers is third-party and must be skipped even
# if the path-based rules above miss it. This is the backstop that makes the
# exclusion list self-verifying rather than a static guess.
FOREIGN_NOTICE = re.compile(
    r"Copyright|SPDX-License-Identifier|@license|Licensed under|All rights reserved",
    re.IGNORECASE,
)

# Our own notice, used for the idempotency check.
OURS_MARKER = "GNU Affero General Public License"

BLOCK_COMMENT_EXT = {".cpp", ".h", ".hpp", ".mm", ".c", ".cc", ".js", ".css"}
HTML_EXT = {".html"}

NOTICE_BODY = """\
This file is part of {product}.
Copyright (C) {year}  {holder}

SPDX-License-Identifier: AGPL-3.0-or-later

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.\
"""


def tracked_files() -> list[Path]:
    out = subprocess.run(
        ["git", "ls-files", "-z", *ROOTS],
        cwd=REPO, capture_output=True, text=True, check=True,
    ).stdout
    return [REPO / p for p in out.split("\0") if p]


def product_name(rel: str) -> str:
    """Human-readable 'this file is part of X' subject, from the path.

    Shared modules nest as modules/<category>/<module>/…, so the module name is
    the third component — the second is a category ('core', 'ui', 'tuning') and
    naming it would be wrong.
    """
    parts = rel.split("/")
    if parts[0] == "plugins" and len(parts) > 1:
        return f"{parts[1]}, an {COPYRIGHT_HOLDER} plugin"
    # len > 3 guarantees parts[2] is a directory, not a file sitting directly in
    # a category dir (modules/cmake/foo.h would otherwise name itself a module).
    if parts[0] == "modules" and len(parts) > 3:
        return f"the {COPYRIGHT_HOLDER} {parts[2]} module"
    return f"the {COPYRIGHT_HOLDER} plugin suite"


def read_preserving_newlines(path: Path) -> str:
    """Read without universal-newline translation.

    Path.read_text() silently rewrites CRLF to LF, which turns a header insertion
    into a whole-file rewrite. One file in this repo (an O-Tremolo index.html) is
    CRLF, and it is exactly the kind of thing that hides in a 707-file diff.
    """
    with open(path, "r", encoding="utf-8", newline="") as fh:
        return fh.read()


def write_preserving_newlines(path: Path, text: str) -> None:
    with open(path, "w", encoding="utf-8", newline="") as fh:
        fh.write(text)


def dominant_eol(text: str) -> str:
    return "\r\n" if "\r\n" in text else "\n"


def render_header(rel: str, ext: str, eol: str = "\n") -> str:
    body = NOTICE_BODY.format(
        product=product_name(rel), year=COPYRIGHT_YEAR, holder=COPYRIGHT_HOLDER
    )
    if ext in HTML_EXT:
        lines = eol.join(f"  {ln}".rstrip() for ln in body.splitlines())
        return f"<!--{eol}{lines}{eol}-->{eol}"
    lines = eol.join(f"   {ln}".rstrip() for ln in body.splitlines())
    return f"/*{eol}{lines}{eol}*/{eol}"


def classify(path: Path) -> tuple[str, str]:
    """Return (verdict, reason). verdict in {skip, ready}."""
    rel = path.relative_to(REPO).as_posix()
    ext = path.suffix.lower()

    if ext not in EXTENSIONS:
        return "skip", "not a source extension"
    for frag in EXCLUDE_SUBSTRINGS:
        if frag in rel:
            return "skip", f"excluded path ({frag})"
    if not path.is_file():
        return "skip", "not a regular file"

    try:
        text = read_preserving_newlines(path)
    except (UnicodeDecodeError, OSError) as exc:
        return "skip", f"unreadable ({type(exc).__name__})"

    head = "\n".join(text.splitlines()[:40])
    if OURS_MARKER in head:
        return "skip", "already has the notice"
    if FOREIGN_NOTICE.search(head):
        return "skip", "THIRD-PARTY NOTICE PRESENT"
    return "ready", ""


def insert(text: str, header: str, ext: str) -> str:
    """Place the header, respecting HTML's DOCTYPE-must-lead rule.

    A comment before <!DOCTYPE> is legal but historically triggered quirks mode,
    so the header goes on the line after it. Everything else takes it at the top.
    """
    if ext in HTML_EXT:
        lines = text.splitlines(keepends=True)
        if lines and lines[0].lstrip().lower().startswith("<!doctype"):
            return lines[0] + header + "".join(lines[1:])
    return header + text


def main() -> int:
    ap = argparse.ArgumentParser()
    g = ap.add_mutually_exclusive_group(required=True)
    g.add_argument("--dry-run", action="store_true")
    g.add_argument("--apply", action="store_true")
    g.add_argument("--audit", action="store_true")
    args = ap.parse_args()

    ready: list[Path] = []
    skipped: dict[str, int] = {}
    foreign: list[str] = []

    for path in tracked_files():
        verdict, reason = classify(path)
        if verdict == "ready":
            ready.append(path)
        else:
            skipped[reason] = skipped.get(reason, 0) + 1
            if reason == "THIRD-PARTY NOTICE PRESENT":
                foreign.append(path.relative_to(REPO).as_posix())

    if args.audit:
        print("Third-party notices found OUTSIDE the excluded paths:")
        if foreign:
            for f in foreign:
                print(f"  !! {f}")
            print("\nThese would have been stamped with our copyright. "
                  "Add them to EXCLUDE_SUBSTRINGS before applying.")
            return 1
        print("  (none) — every third-party file is caught by EXCLUDE_SUBSTRINGS.")
        return 0

    by_ext: dict[str, int] = {}
    for p in ready:
        by_ext[p.suffix.lower()] = by_ext.get(p.suffix.lower(), 0) + 1

    print(f"{'WOULD WRITE' if args.dry_run else 'WRITING'}: {len(ready)} files")
    for ext, n in sorted(by_ext.items(), key=lambda kv: -kv[1]):
        print(f"    {ext:<6} {n}")
    print("\nSkipped:")
    for reason, n in sorted(skipped.items(), key=lambda kv: -kv[1]):
        flag = "  !!" if "THIRD-PARTY" in reason else "    "
        print(f"{flag} {reason:<34} {n}")

    if foreign:
        print("\nABORT: third-party files reached the content backstop. Run --audit.")
        return 1

    if args.dry_run:
        if ready:
            sample = ready[0]
            rel = sample.relative_to(REPO).as_posix()
            print(f"\n--- sample header for {rel} ---")
            print(render_header(rel, sample.suffix.lower()), end="")
        return 0

    for path in ready:
        rel = path.relative_to(REPO).as_posix()
        ext = path.suffix.lower()
        text = read_preserving_newlines(path)
        eol = dominant_eol(text)
        write_preserving_newlines(
            path, insert(text, render_header(rel, ext, eol), ext)
        )

    print(f"\nDone. {len(ready)} files updated.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
