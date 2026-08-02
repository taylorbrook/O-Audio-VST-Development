---
quick_id: 260801-u3o
plugin: O-simpleSampler
version: 1.1.0
date: 2026-08-01
task: "Task 4 — build, install, and run the full verification battery"
verdict: PASS
---

# O-simpleSampler v1.1.0 — verification battery

Run after the three commercial-library built-ins were withdrawn and the `sourceSample`
choice parameter was removed (APVTS contract 21 → 20 parameters).

All figures below are the values actually observed on 2026-08-01, not summaries.

---

## 1. Build and install

`./scripts/build-and-install.sh O-simpleSampler` — total time **49 s**, exit 0.

```
→ Phase 3: Product name (from artefact): O-simpleSampler-dev
→ Phase 4: Removing old VST3: ~/Library/Audio/Plug-Ins/VST3/O-simpleSampler-dev.vst3
→ Phase 4: Removing old AU:   ~/Library/Audio/Plug-Ins/Components/O-simpleSampler-dev.component
✓ Old versions removed
✓ New versions installed
→ Phase 6: Clearing Audio Unit cache... / Killing AudioComponentRegistrar...
✓ Verification complete
  VST3: ~/Library/Audio/Plug-Ins/VST3/O-simpleSampler-dev.vst3          Size: 6.2M, Age: 0s
  AU:   ~/Library/Audio/Plug-Ins/Components/O-simpleSampler-dev.component Size: 6.2M, Age: 0s
```

**No `⚠ Sweeping ALTERNATE-variant` warning was emitted** — the machine carried only the
`-dev` variant, so no unsuffixed orphan existed to sweep. Asserted independently below.

### Installed-bundle assertions

| Assertion | Result |
|---|---|
| `~/Library/Audio/Plug-Ins/VST3/O-simpleSampler.vst3` (unsuffixed orphan) | **absent** — `No such file or directory` |
| `~/Library/Audio/Plug-Ins/Components/O-simpleSampler.component` (unsuffixed orphan) | **absent** — `No such file or directory` |
| `-dev` VST3 present | yes, 6.2 M |
| `-dev` AU present | yes, 6.2 M |
| Withdrawn filenames anywhere inside the installed VST3 bundle | **0 matches** |
| Withdrawn filenames anywhere inside the installed AU bundle | **0 matches** |
| `strings` of the VST3 executable matching the withdrawn tokens | **0** |
| `strings` of the AU executable matching the withdrawn tokens | **0** |

The last two rows are the stronger check: they scan the linked Mach-O, not just filenames,
so a withdrawn asset re-embedded through `juce_add_binary_data` would still surface.

---

## 2. auval

`./scripts/verify-au-link.sh O-simpleSampler` — exit **0**.

```
Manufacturer String: Ouaricon Audio Development
Component Version: 1.1.0 (0x10100)
# # # 20 Global Scope Parameters:
...
* * PASS
--------------------------------------------------
AU VALIDATION SUCCEEDED.
--------------------------------------------------
[verify-au-link] PASS: auval accepted O-simpleSampler (aumu OsSm OuDv)
```

| Assertion | Required | Observed |
|---|---|---|
| Terminates in AU validation success | `AU VALIDATION SUCCEEDED` | **`AU VALIDATION SUCCEEDED`** |
| Global Scope Parameters | **20** (was 21) | **20** |
| Component Version | **1.1.0** | **1.1.0 (0x10100)** |

The version assertion is what catches a silently-ignored `PLUGIN_VERSION` keyword: JUCE
reads `VERSION`, and a bundle built with the wrong keyword would have reported
`1.0.0 (0x10000)` here. It reports `0x10100`, so the `VERSION "1.1.0"` edit took effect.

The 20 parameter IDs auval enumerated (hashed from the string IDs, so index-independent):

```
100571, 3143098, 3571704, 51114284, 109757538, 109806183, 349902743, 351093686,
408009663, 462452390, 544002307, 627419222, 794615928, 1099846370, 1203012003,
1262195742, 1305086723, 1380082205, 1676421509, 2109644716
```

---

## 3. pluginval — strictness level 10, six runs

```
/Applications/pluginval.app/Contents/MacOS/pluginval \
  --strictness-level 10 --validate-in-process --skip-gui-tests \
  --timeout-ms 60000 --validate <bundle>
```

Every run's log was scanned for a case-insensitive `nan`, a standalone `inf`, and
`FAILED`. Exit code alone is not treated as sufficient.

| Format | Run | Exit code | `nan` matches | standalone `inf` matches | `FAILED` matches | Log lines | Verdict |
|---|---|---|---|---|---|---|---|
| VST3 | 1 | **0** | 0 | 0 | 0 | 137 | PASS |
| VST3 | 2 | **0** | 0 | 0 | 0 | 137 | PASS |
| VST3 | 3 | **0** | 0 | 0 | 0 | 137 | PASS |
| AU   | 1 | **0** | 0 | 0 | 0 | 140 | PASS |
| AU   | 2 | **0** | 0 | 0 | 0 | 140 | PASS |
| AU   | 3 | **0** | 0 | 0 | 0 | 140 | PASS |

Every run reported `Strictness level: 10`, reached
`Starting tests in: pluginval / Fuzz parameters...` → `Completed tests in pluginval / Fuzz
parameters`, and terminated with `SUCCESS`.

The parameter-fuzz stage is the one that matters here. Had the source selector been kept
as a one-entry `AudioParameterChoice`, its `NormalisableRange {0.0f, 0.0f}` would make
`convertTo0to1` compute `0/0`, and `jlimit` does not clamp NaN — the fuzz stage drives
arbitrary normalised values into every parameter and is exactly where that would surface.
Three runs per format rather than one, because a single strictness-10 run can pass by luck.

Logs: `/tmp/u3o-pv-{VST3,AU}-{1,2,3}.log`.

---

## 4. Offline render harness

Rebuilt and re-run against the current tree (not trusting the Task 1 run — the harness is
the artefact that quietly stops reflecting the plugin). Exit **0**.

```
O-simpleSampler render-harness — fs=44100, pianoRoot=48 (f0~131 Hz)
  [PASS] makes-sound                rms=0.2154
  [PASS] repitch-tuning             f48=131.2 f60=262.5 f36=65.8 (f60/f48=2.000 f36/f48=0.501)
  [PASS] stretch-pitch-tracks       f48=131.2 f60=264.1 ratio=2.012
  [PASS] stretch-time-independence  dSrc=0.88s win=3.3s | Repitch 1.61/0.85=1.89 | Stretch 0.82/0.88=0.93
  [PASS] loop-seam-continuity       fwd contMin=0.953 seamMaxDelta=0.004 | pingpong cont=0.944 seamMaxDelta=0.004
  [PASS] region-end-declick         L=39823 endMaxDelta=0.0008 contentLevel=0.0348
  [PASS] vintage-clean-at-zero      relDiff(v100 vs v0)=0.190 flatClean=0.0026 flatCrush=0.3088
  [PASS] aa-uptranspose-stable      Repitch peak=0.509 rms=0.0999 | Stretch peak=0.256 rms=0.1131
  [PASS] stress-bounded             peak=4.823 heldRms=0.5273 tailRms=0.0000

ALL PASS — 0 failure(s)
```

**9/9 cases pass, 0 failures.** The figures are bit-identical to the Task 1 run, which is
the expected result: the removals were a parameter and three unused assets, not DSP.
`repitch-tuning` still reports f48 = 131.2 Hz, confirming the root-48 seed survives with
the selector gone — the deferred `pendingRootSeed` path is now the only thing that seeds it.

---

## 5. Headless UI render (Task 2 carry-in)

The Source group was re-verified by **rendering** it, not by reading the HTML — two
project patterns (`pattern_module_toplevel_init_tdz`,
`pattern_js_state_updater_overwrites_html_labels`) describe static reads passing while the
UI is dead. `index.html` was loaded in headless Chromium against a JUCE-bridge stub and
driven over CDP:

| Probe | Observed |
|---|---|
| Uncaught exceptions | **0** |
| `console.error` / `console.warn` | **0** |
| `<select>` elements | **2** (`combo-loopMode`, `combo-pitchMode`) |
| `combo-loopMode` options | `["Off", "Forward", "Ping-Pong"]` |
| `combo-pitchMode` options | `["Repitch", "Stretch"]` |
| `<select>` remaining in `.group-source` | **false** |
| Source status line text | **`piano — built-in source`** (rendered, non-empty) |
| Load… button visible | true |
| Drop zone visible | true |
| Knob readouts populated | **17 / 17** |
| Pitch-mode readout | `Repitch — pitch & time linked` |
| Combo round-trip (set loopMode → index 2) | resolved to `Ping-Pong` — machinery live |
| Rack groups rendered | 7 |
| `document.body.scrollHeight` | 950 (fits the 950 px viewport; the layout got shorter) |

Still requires the developer's eyes in Task 6: real WKWebView/WebView2 rendering, in-DAW
behaviour, and the by-ear checks.

---

## Verdict

**PASS.** v1.1.0 is installed as `-dev` VST3 + AU with no unsuffixed orphan; no withdrawn
asset filename or string survives in either installed bundle; auval succeeds reporting 20
Global Scope Parameters at Component Version 1.1.0; six strictness-10 pluginval runs all
exit 0 with clean logs; the render harness reports 9/9.
