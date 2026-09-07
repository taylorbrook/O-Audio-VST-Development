---
task: 260906-tyt
type: quick
status: complete
completed: 2026-09-06
subsystem: plugins/O-Lyrica
tags: [auval, au-validation, meta-parameter, juce, o-lyrica, parameter-stability]

dependency_graph:
  requires:
    - "O-Lyrica v2.5.0 installed as O-Lyrica-dev.component (aumu OLyr OuDv)"
    - "build/ CMake-configured"
  provides:
    - "O-Lyrica 2.5.1 with a green AU gate — auval usable as a regression detector again"
  affects:
    - "Any future O-Lyrica release: the AU gate is now a real signal, not a hand-waved red"

tech_stack:
  added: []
  patterns:
    - "juce::AudioParameterBoolAttributes().withMeta(true) for parameters that programmatically write other parameters"

key_files:
  created: []
  modified:
    - plugins/O-Lyrica/Source/PluginProcessor.cpp
    - plugins/O-Lyrica/CMakeLists.txt
    - plugins/O-Lyrica/CHANGELOG.md
    - plugins/O-Lyrica/NOTES.md
    - PLUGINS.md

decisions:
  - "Flag BOTH toggles, not just the reported victim: auval names freeToggle, but scaleToggle is the writer. Each writes the other, so each is a meta parameter."
  - "Corrected the CHANGELOG/NOTES severity claim rather than inheriting it — the assertion was a hard FAIL, never a warning."
  - "Corrected NOTES.md's build-script claim only after the script actually succeeded (plan's conditional), not on inspection alone."

metrics:
  duration: ~12min
  commits: 1
  tasks: 3

actuals:
  tokens: 1890
  tasks: 3
  commits: 1
---

# Quick Task 260906-tyt: O-Lyrica auval Meta Param Flag — Summary

O-Lyrica's `auval` had been failing since v1.30.0 on the parameter-stability sweep, carried
in two places as a "pre-existing and benign" warning. It was neither pre-existing-and-benign
nor a warning: a hard `* * FAIL` and a failed verdict. Declaring both glissando toggles as
meta parameters clears it. Ships as 2.5.1.

## The exact validation command, as run

```
auval -v aumu OLyr OuDv
```

Three unquoted words after `-v`, per the plan. Both runs redirected combined output to
`$SCRATCH/auval-before.txt` and `$SCRATCH/auval-after.txt`, run in the background and polled.

## Task 1 — Pre-fix auval (negative control)

Run against the **installed v2.5.0 bundle, before any source was touched**. Transcript:
880 lines, `$SCRATCH/auval-before.txt`.

The `Meta Param Flag` assertion block, verbatim (lines 823-827):

```
ParameterID=1275870432, Scope=0, Element=0: Saved Value = 0.337891, Current Value 0.000000
ERROR: Parameter values are different since last set - probable cause: a Meta Param Flag is NOT set on a parameter that will change values of other parameters.
Cannot perform Parameter Value check across initialization and reset

* * FAIL
```

Closing verdict, verbatim (line 878):

```
AU VALIDATION FAILED: CORRECT THE ERRORS ABOVE.
```

Failure-marker count, unanchored `grep -cE 'FAIL'`: **2** (the `* * FAIL` at 827 and the
verdict at 878). The plan's warning about anchoring was correct — auval writes the marker
as `* * FAIL`, so a `^FAIL` anchor would have matched nothing and read green by construction.

**Severity resolved from the transcript, per the plan's third branch test:** block PRESENT
and the run ends with a *failed* verdict. That is the "as briefed" case — proceed.

### ParameterID 1275870432 identified — two independent ways

The plan asked for a hash re-derivation if the ID differed. It did not differ, and the
identification did not actually need the hash: **auval's own parameter dump names it**
(before-transcript lines 488-493):

```
Parameter ID:1275870432
Name: Free Glissando
Parameter Type: Boolean
Values: Minimum = Off, Default = Off, Maximum = On
Flags: Values Have Strings, High Resolution, Readable, Writable 
  -parameter PASS
```

The hash derivation was run anyway as a cross-check over all **70** declared param IDs
(`result = 31*result + c` over uint32, masked to 31 bits). Exactly one collides:

| param ID | masked hash |
|----------|-------------|
| `freeToggle` | **1275870432** ← matches |
| `scaleToggle` | 1351160126 |

Both methods agree: the reported parameter is `freeToggle` ("Free Glissando"), the *victim*.
auval moved `scaleToggle`; the v1.30.0 mutual exclusion in `parameterChanged()` wrote
`freeToggle` back to 0 behind auval's back, so the saved value (0.337891, auval's random
probe) and the current value (0.000000) disagreed after reset.

## Task 2 — The code change

`plugins/O-Lyrica/Source/PluginProcessor.cpp`, in `createParameterLayout()`. The complete
source diff (`git show HEAD -- .../PluginProcessor.cpp`), nothing elided:

```diff
+    // v2.5.1: Both toggles are META parameters. They are mutually exclusive — turning one ON
+    // programmatically writes the other OFF in parameterChanged() below — so each one changes
+    // the value of another parameter. The meta attribute tells the host/AU wrapper that, and
+    // without it auval's parameter-stability sweep fails on Free Glissando:
+    // "a Meta Param Flag is NOT set on a parameter that will change values of other parameters."
     layout.add(std::make_unique<juce::AudioParameterBool>(
         juce::ParameterID { "freeToggle", 1 },
         "Free Glissando",
-        false
+        false,
+        juce::AudioParameterBoolAttributes().withMeta (true)
     ));
 
     layout.add(std::make_unique<juce::AudioParameterBool>(
         juce::ParameterID { "scaleToggle", 1 },
         "Scale-Locked Glissando",
-        false
+        false,
+        juce::AudioParameterBoolAttributes().withMeta (true)
     ));
```

Two call sites, one per toggle. `parameterChanged` (1031-1052), the `isUpdatingToggles`
reentrancy guard, the listener registrations and both `false` defaults are byte-identical
to 2.5.0 — the diff above is the whole of the source change. This tells the host about
existing behaviour; it does not alter behaviour.

The comment deliberately avoids the setter identifier so the acceptance gate
(`grep -v '^\s*//' | grep -c 'withMeta'` = 2) counts real call sites only. Measured: 2.

Version bump `2.5.0` → `2.5.1` in CMakeLists.txt (the single source of truth; a repo grep
across txt/cmake/json/md confirmed no other pin inside `plugins/O-Lyrica`), a 2.5.1
CHANGELOG entry at the top of the file, the NOTES.md severity claim rewritten, and the
PLUGINS.md row moved to 2.5.1.

## Task 3 — Build, install, validate

**Build path used: the SCRIPT, not the manual fallback.** `./scripts/build-and-install.sh
O-Lyrica` succeeded in 59s, exit 0. It resolved the target correctly, logging:

```
→   - CMake target: OLyrica (targets OLyrica_VST3, OLyrica_AU)
```

It cleared the AU caches, killed `AudioComponentRegistrar`, swept the old bundles and
installed both formats. No `⚠ Sweeping ALTERNATE-variant` warning appeared, consistent with
the plan's finding that no unsuffixed variants were installed.

**Because the script path worked, NOTES.md:71-73 WAS corrected** (the plan's conditional).

Installed-artefact version confirmed **before** validating, so a green run could not be a
stale artefact:

```
CFBundleShortVersionString = 2.5.1
CFBundleVersion            = 2.5.1
```

and the after-transcript's own header agrees: `Component Version: 2.5.1 (0x20501)`.

### Post-fix verdict, verbatim (auval-after.txt line 877)

```
AU VALIDATION SUCCEEDED.
```

All three conditions met:

| Condition | Result |
|-----------|--------|
| `AU VALIDATION SUCCEEDED` present | yes, line 877 |
| unanchored `grep -cE 'FAIL'` over whole transcript | **0** |
| `Meta Param Flag` block present | **0 occurrences — gone** |

The sweep that previously failed now passes (after-transcript, replacing the old block):

```
Testing that parameters retain value across reset and initialization
  PASS

* * PASS
```

### The pass is attributable to this change, not a registry reshuffle

The `Global Meta` flag count across the transcript goes **0 → 2**, on precisely the two
toggles and no other parameter:

```
  BEFORE  Parameter ID:1275870432 / Name: Free Glissando
          Flags: Values Have Strings, High Resolution, Readable, Writable

  AFTER   Parameter ID:1275870432 / Name: Free Glissando
          Flags: Values Have Strings, High Resolution, Global Meta, Readable, Writable
```

Same for `Parameter ID:1351160126 / Name: Scale-Locked Glissando`. The two transcripts are
880 and 879 lines and otherwise structurally identical; the only source edit between them is
the meta attribute.

One `WARNING:` line (`Source AU supports multi-channel output but does not provide a channel
layout`, line 837) is present in **both** transcripts — pre-existing, non-failing, unchanged
by this task, and left alone.

## Commit

**`62f2611c`** — `fix(O-Lyrica): declare both glissando toggles as meta params — auval green at 2.5.1`

Path-scoped (`git commit -- plugins/O-Lyrica PLUGINS.md`), 5 files, +58/−9:

```
 PLUGINS.md                                  |  2 +-
 plugins/O-Lyrica/CHANGELOG.md               | 33 +++++++++++++++++++++++++++++
 plugins/O-Lyrica/CMakeLists.txt             |  2 +-
 plugins/O-Lyrica/NOTES.md                   | 19 ++++++++++++-----
 plugins/O-Lyrica/Source/PluginProcessor.cpp | 11 ++++++++--
```

`git branch --show-current` (`main`) and `git status --short` were re-run immediately before
committing; the index was empty at that moment. `.gsd/dispatch-isolation-sentinel.json` is
**not** in the commit (0 `.gsd` paths in `git show --stat`) and is still modified in the
working tree afterwards, as it was found. No tag created (`git tag --points-at HEAD` = 0).
Session trailer present.

## Plan predictions that turned out FALSE

1. **The severity claim in the repo was wrong — the biggest one.** CHANGELOG.md:540 and
   NOTES.md:61 both called it a static *"warning"* that was *"pre-existing and benign"*, and
   claimed "all render/MIDI/parameter tests pass". The transcript shows `* * FAIL` on the
   parameter test and `AU VALIDATION FAILED: CORRECT THE ERRORS ABOVE.` The task brief was
   right and both in-repo documents were wrong. Both have been corrected. This is precisely
   the blind spot the task existed to close: a red gate reclassified in prose as green.

2. **NOTES.md:71-73 was stale, as the plan suspected but would not assume.** It claimed
   `build-and-install.sh` fails for this plugin with `unknown target 'O-Lyrica_VST3'`. The
   script resolves the target from CMakeLists and succeeded first try. Corrected in the same
   commit, with the verification date, so the next reader does not re-derive it.

3. **The hash derivation was unnecessary work.** The plan invested a whole table and a
   fallback Python snippet in proving `1275870432 == freeToggle` by
   `String::hashCode()`. auval's own parameter dump prints `Parameter ID:1275870432` followed
   by `Name: Free Glissando` in the same block, 335 lines above the assertion. The hash was
   run as a cross-check and agreed, but reading the transcript would have sufficed. Worth
   knowing for the next AU parameter investigation: **auval names the parameter, look up-file
   before computing anything.**

4. **No fifteen-minute registry rescan.** The plan budgeted ~15 min for a cold `auval` after
   install and instructed not to kill it. Both runs — including the one immediately after a
   fresh install with the AU caches cleared and `AudioComponentRegistrar` killed — completed
   in well under a minute. The tolerance cost nothing, but the estimate was pessimistic here.

5. **Token estimate was ~48× high.** The plan estimated 90000 tokens (45000 raw). The realized
   diff is 7560 chars ≈ **1890 tokens** on the same chars/4 scale. Recorded unrounded; the
   task was a two-line source change plus documentation, and the estimate priced it as a
   research task.

Predictions that held: the `^FAIL` anchor would have been vacuous (confirmed — the marker is
`* * FAIL`); both toggles needed the flag, not just the reported one; `AudioParameterBool`'s
trailing attributes argument is available and O-simplePhysicalModelSynth's call shape
transplanted unchanged; no unsuffixed variant existed to sweep; CMakeLists was the only
in-plugin version pin.

## Success criteria

- [x] `auval -v aumu OLyr OuDv` prints `AU VALIDATION SUCCEEDED.` with zero failure lines, quoted above with the exact command
- [x] Pre-fix assertion block quoted as the negative control
- [x] Both toggles declared as meta parameters; mutual-exclusion logic byte-identical
- [x] 2.5.1 across CMakeLists.txt, CHANGELOG.md, PLUGINS.md and the installed bundle's Info.plist
- [x] NOTES.md no longer claims the assertion is a benign permanent fixture
- [x] One path-scoped commit with the session trailer; no tag

## Self-Check: PASSED

- `plugins/O-Lyrica/Source/PluginProcessor.cpp` — FOUND, 2 non-comment `withMeta` call sites
- `plugins/O-Lyrica/CMakeLists.txt` — FOUND, `VERSION "2.5.1"`, no `2.5.0`
- `plugins/O-Lyrica/CHANGELOG.md` — FOUND, `## [2.5.1] - 2026-09-06` at top
- `plugins/O-Lyrica/NOTES.md` — FOUND, both stale claims rewritten
- `PLUGINS.md` — FOUND, O-Lyrica row reads 2.5.1
- Commit `62f2611c` — FOUND in `git log`
- Task 2 gate — PASS; Task 3 gate — PASS
- `git status --porcelain -- plugins/O-Lyrica PLUGINS.md` — empty (all committed)
