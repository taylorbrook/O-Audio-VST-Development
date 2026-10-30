#!/usr/bin/env python3
"""Apply the backups/ retention policy: per plugin, keep the newest 5 snapshots
(by mtime) plus every snapshot whose version has a release tag.

Both tag forms count as "released": `<Plugin>-vX.Y.Z` (current, the form CI
fires on), the legacy version-first `vX.Y.Z-<Plugin>`, and the few legacy
lowercase `o-name-vX.Y.Z` tags. Entries that don't
start with a version (stash patches, notes) are always kept.

Dry run by default; pass --apply to delete. Deletion is permanent — backups/
is gitignored, so nothing in git can bring a pruned snapshot back. Release
tags and `git restore --source=<tag> -- plugins/<Name>` cover tagged rollback.

Usage:
    python3 scripts/prune-backups.py            # list what would go
    python3 scripts/prune-backups.py --keep 10  # more conservative
    python3 scripts/prune-backups.py --apply
"""
import argparse
import os
import re
import shutil
import subprocess
import sys

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
BACKUPS = os.path.join(ROOT, "backups")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--keep", type=int, default=5)
    ap.add_argument("--apply", action="store_true")
    args = ap.parse_args()

    tags = set(subprocess.run(["git", "-C", ROOT, "tag"], capture_output=True,
                              text=True, check=True).stdout.split())
    doomed, kept = [], 0
    for plugin in sorted(os.listdir(BACKUPS)):
        pdir = os.path.join(BACKUPS, plugin)
        if not os.path.isdir(pdir):
            continue
        entries = [e for e in os.listdir(pdir) if not e.startswith(".")]
        entries.sort(key=lambda e: os.path.getmtime(os.path.join(pdir, e)), reverse=True)
        for i, e in enumerate(entries):
            m = re.match(r"v?(\d+\.\d+\.\d+)", e)
            released = m and any(t in tags for t in (f"{plugin}-v{m.group(1)}",
                                                     f"v{m.group(1)}-{plugin}",
                                                     f"{plugin.lower()}-v{m.group(1)}"))
            if not m or i < args.keep or released:
                kept += 1
            else:
                doomed.append(os.path.join(pdir, e))

    for path in doomed:
        print(("deleting " if args.apply else "would delete ") + os.path.relpath(path, ROOT))
        if args.apply:
            shutil.rmtree(path) if os.path.isdir(path) else os.remove(path)
    print(f"\nkept {kept}, {'deleted' if args.apply else 'would delete'} {len(doomed)}",
          file=sys.stderr)


if __name__ == "__main__":
    main()
