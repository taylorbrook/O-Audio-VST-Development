---
name: improve-verify
description: Verify a plugin's just-resolved code-review fixes actually hold and introduced no regressions, then gate ship-readiness
argument-hint: "[PluginName] [version? e.g. 1.3.3]"
---

# /improve-verify

The verification gate that runs **after** `/improve-review` (or any `/improve`) has resolved
findings and bumped a version — and **before** `/package` or `/publish`. It answers three
questions the resolution step cannot answer about itself:

1. **Did the fixes survive to disk?** Concurrent sessions, linters, and failed/partial commits
   can silently clobber a resolution (a staged edit gets reset, a version row reverts, a whole
   file is overwritten). This confirms the shipped tree actually contains every fix.
2. **Is each finding really closed?** Not "an edit was made near the cited line" — the specific
   failure mode named in `CODE_REVIEW.md` must be demonstrably gone.
3. **Did the fixes break anything else?** A resolution edits real code paths (a new
   `processBlock` branch, a SafePointer capture, a preset-apply reset) — those new lines get an
   adversarial regression pass of their own.

`CODE_REVIEW.md` and `CHANGELOG.md` live at `plugins/[PluginName]/`. This command is
**read-mostly**: it builds and inspects, but only writes verification status (see Reporting) —
it does not resolve new findings. If it finds a regression or an unclosed finding, it hands
back to `/improve-review` / `/improve` rather than fixing in place.

## Preconditions

<preconditions enforcement="blocking">
  <status_verification target="PLUGINS.md" required="true">
    <check condition="plugin_exists">
      Plugin entry MUST exist in PLUGINS.md.
    </check>
    <check condition="status_in(✅ Working, 📦 Installed)">
      Status MUST be ✅ Working OR 📦 Installed.
      <on_violation status="🚧 In Development">
        REJECT: "[PluginName] is still in development (Stage [N]). There is no resolution to
        verify — use /continue [PluginName]."
      </on_violation>
      <on_violation status="[any other status]">
        REJECT: "[PluginName] has status '[Status]'. Only ✅ Working or 📦 Installed plugins
        can be improve-verified."
      </on_violation>
    </check>
  </status_verification>
  <resolution_present required="true">
    A resolution to verify MUST exist: a top CHANGELOG entry for [version] (or the latest
    version if none given) whose notes reference finding IDs, and/or a CODE_REVIEW.md.
    <on_missing>
      REJECT: "No resolution found to verify for [PluginName] [version]. Run /improve-review
      [PluginName] (or /improve) first, then re-run /improve-verify."
    </on_missing>
  </resolution_present>
</preconditions>

## Routing

**If no plugin name provided:**
1. `ls plugins/*/CHANGELOG.md` → for each, read the top entry. Build the candidate list of
   plugins whose latest CHANGELOG version is NEWER than their last "verified:" stamp (see
   Reporting) — i.e. resolved-but-not-yet-verified.
2. Show a numbered menu: `1. O-Bass v1.3.3 (CR-01, WR-01..03) — resolved 2026-07-08  2. …`.
3. Wait for selection, then proceed as "plugin name only".

**If plugin name only (no version):**
1. Take the top `CHANGELOG.md` version as the resolution under test.
2. Proceed to Verification.

**If plugin name and version provided:**
1. Locate that version's CHANGELOG entry + the matching `O-<Plugin>-v<version>` tag if present.
2. Proceed to Verification.

## Verification

<verification>
  Establish the resolution's scope, then verify closure, regressions, and runtime behavior.

  <mandatory_steps>
    1. RESOLVE THE DIFF UNDER TEST. Determine the exact change set being verified:
       - If tagged: `git diff <prev-tag>..O-<Plugin>-v<version>` (prev-tag = the plugin's
         previous `O-<Plugin>-v*` tag).
       - If committed but untagged: the commit(s) whose message names the version.
       - If uncommitted: the working-tree diff for `plugins/[PluginName]/`.
       Enumerate the finding IDs this resolution claims to close (from the CHANGELOG entry).

    2. FIX-SURVIVED-TO-DISK CHECK (concurrency guard). This is the step that catches a
       clobbered resolution. For each claimed fix, confirm its marker is present in the CURRENT
       on-disk source (grep the code comment / new symbol the fix introduced), AND that the
       version is coherent end-to-end: CMakeLists `VERSION`, the built bundle's
       `CFBundleShortVersionString`, the CHANGELOG top entry, and the PLUGINS.md row all agree.
       Any mismatch (e.g. PLUGINS.md reverted to the old version, or a source marker missing) =
       the resolution was partially lost → REPORT and hand back to /improve-review; do not
       proceed to PASS.

    3. PER-FINDING CLOSURE. For each finding ID, read the cited file:line in the current source
       and assert the specific failure mode is gone — not merely that an edit exists nearby.
       State the closure evidence per finding (e.g. "CR-01: both launchAsync completions capture
       a SafePointer and bare-return on null; no raw `this` remains in either completion").

    4. REGRESSION HUNT ON THE NEW CODE. Adversarially review ONLY the lines the resolution
       added/changed, looking for defects the fix itself may have introduced. Weight by what the
       diff touched — audio-thread code → RT-safety (allocation, locking, non-atomic shared
       state); WebView/async → lifetime (UAF, SafePointer, `complete()` on a dead path); preset/
       state → round-trip + defaults; params → ID/range/skew/state-format stability. For a
       nontrivial or high-risk diff, spawn parallel reviewers and adversarially verify each
       candidate before reporting (default to refuted); for a small diff an inline pass is fine.

    5. BUILD + VALIDATE CLEAN. Rebuild via build-automation / build-and-install.sh. On macOS
       confirm `auval -v <type> <subtype> <mfr>` PASSES; on Windows run pluginval. A resolution
       that no longer builds or validates fails verification regardless of finding closure.

    6. EXERCISE THE BEHAVIOR. Drive the affected flow end-to-end, don't just trust the build:
       - Offline DSP change → run/extend the render-harness and assert the acceptance criteria.
       - Runtime-only or host-dependent change (UI, preset dialogs, automation, mode switches)
         → emit a precise, numbered DAW test script for the human (exact steps + expected
         result per finding), since these can't be driven headlessly. Never silently skip —
         say which findings are covered by automation vs. handed to manual test.

    7. NO SILENT PASS. If any finding is unclosed, any regression is confirmed, the build/auval
       fails, or the survived-to-disk check mismatches → overall verdict is FAIL. Report what
       failed and route back (/improve-review for CODE_REVIEW findings, /improve for freeform).
  </mandatory_steps>

  <output_guarantee>
    - Per-finding verdict (CLOSED / NOT-CLOSED) with concrete evidence, most-severe first.
    - Regression pass result over the resolution diff (findings, or "none survived verification").
    - Build + auval/pluginval result; behavior-exercise result (automated) or a numbered manual
      DAW test script (host-dependent), with coverage stated explicitly.
    - Overall PASS/FAIL and the copy-paste next command (see Handoff).
  </output_guarantee>
</verification>

## Reporting

- On PASS, stamp the resolution as verified so the no-arg menu stops listing it: add a
  `verified: <ISO-8601>` line to the top `CODE_REVIEW.md` front-matter (or a
  `### Verified [version] — <date>` note in NOTES.md if there is no CODE_REVIEW.md). This is
  the only write this command makes to plugin sources.
- On FAIL, write nothing to the sources; surface the failing items and route back.

## Handoff

Per project handoff protocol, end with **Step 1:** `/clear` and **Step 2:** the specific next
command, then STOP:
- **PASS** → `/package [PluginName]` (branded installer) or `/publish [PluginName]`
  (cross-platform release), whichever the user is heading toward.
- **FAIL (CODE_REVIEW findings)** → `/improve-review [PluginName] [failing IDs]`.
- **FAIL (freeform/regression)** → `/improve [PluginName] "<regression description>"`.

## Notes

- Complements, does not duplicate, `/plugin-verify` (stage-based, goal-backward over a
  workflow stage) and the `verify` skill (generic change-exercise). `/improve-verify` is
  scoped to a *resolution diff* and is finding-ID-aware: its unit of truth is the CHANGELOG /
  CODE_REVIEW finding set, and its headline job is proving those specific fixes shipped intact.
- The survived-to-disk check (step 2) exists because resolutions are frequently authored while
  other sessions touch the same monorepo — a passing `/improve-review` does NOT guarantee the
  fixes reached the committed/shipped tree.
