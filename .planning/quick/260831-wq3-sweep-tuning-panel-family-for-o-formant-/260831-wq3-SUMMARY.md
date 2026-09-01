---
phase: 260831-wq3
plan: 01
subsystem: tuning-panel
tags: [tuning, webview, native-fn, item-22, item-71, sweep]
status: complete
requires: []
provides:
  - "O-MicrotonalSampler v1.25.2 — getMasterTune native fn + panel load-time A4 read + currentHz drag continuity"
  - "O-Reed tuning-tab disconnection proven and recorded as found-not-fixed"
affects:
  - plugins/O-MicrotonalSampler
tech-stack:
  added: []
  patterns:
    - "An unregistered JUCE native fn does not reject — it never settles (jassertfalse; return). The panel hangs silently."
key-files:
  created: []
  modified:
    - plugins/O-MicrotonalSampler/Source/PluginEditor.cpp
    - plugins/O-MicrotonalSampler/Resources/ui/js/tuning-panel.js
    - plugins/O-MicrotonalSampler/CHANGELOG.md
    - plugins/O-MicrotonalSampler/CMakeLists.txt
    - PLUGINS.md
decisions:
  - "O-Reed reported, not fixed: all 19 panel-called native fns are unregistered, so the item-22 pair alone would be decorative."
  - "boot-all-uis reported as producing no evidence for O-Reed rather than forced into a verdict — the mount is lazy and the stub invents values."
metrics:
  duration: ~35 min
  completed: 2026-08-31
actuals:
  tokens: 21000
  tasks: 3
  commits: 2
---

# Quick Task 260831-wq3: Sweep the tuning-panel family for the O-Formant item 22 gap — Summary

Swept all seven tuning-panel-family plugins for the O-Formant item 22 A4 gap
(SUMMARY item 71). Exactly one carried the panel half — O-MicrotonalSampler, shipped
as v1.25.2 with a probe that failed before and passed after plus a control arm.
O-Reed carries a different and much larger defect, proven by an exact native-fn set
difference and a runtime arm, and recorded as found-not-fixed. The other five were
re-verified clean, unedited and unbumped.

## Commits

| SHA | Scope | Files |
|---|---|---|
| `56fc8bbc` | `plugins/O-MicrotonalSampler` | 4 — PluginEditor.cpp, tuning-panel.js, CHANGELOG.md, CMakeLists.txt |
| `5ecd6df2` | `PLUGINS.md` | 1 |

Two commits, both path-scoped, both on `main`. No push, no tag, no `--amend`, no
`git add -A`. Branch and `git status --short` were re-checked immediately before each.

## Per-plugin verdict table (executor-observed citations)

Every line number below was re-grepped in this session. Every survey citation
resolved — none had drifted, with two wording corrections noted in the last column.

| Plugin | ver | Verdict | Observed evidence (file:line) |
|---|---|---|---|
| **O-MicrotonalSampler** | 1.25.1 → **1.25.2** | **PANEL-HALF GAP → FIXED** | State half already correct: `PluginProcessor.cpp:2844` `setProperty ("masterTune", engine.getMasterTune())`, `:2876` `hasProperty ("masterTune")` → `engine.setMasterTune(...)`. Gap: `PluginEditor.cpp` had `getOctaveStretch` at **:824** but no `getMasterTune`; `Resources/ui/js/tuning-panel.js` `loadInitialState()` **:275-302** never read A4; `setupRefPitchKnob()` **:1010** had `startValue = 440` literal and no `currentHz`. Fixed: editor `getMasterTune` at **:834-847**, panel read at **:293-306**, `currentHz` at **:1034 / :1037 / :1056**, `this.updateRefPitchKnob = updateKnob` at **:1051**. |
| **O-Bells** | 4.3.2 | **CLEAN** (both halves present) — no edit, no bump | Editor `getMasterTune` **PluginEditor.cpp:497-498**; `setMasterTune` **:501-511** routing through the `tuning_masterTune` param (`getParameter("tuning_masterTune")` at **:506**). Processor listener **PluginProcessor.cpp:1626** `if (parameterID == "tuning_masterTune")` forwards to the engine (registered **:579**, cached **:717**). Panel `Resources/ui/js/tuning-panel.js`: load-time read **:1012** (`.then` form), `let currentHz = 440` **:993**, `currentHz = hz` **:996**, `startValue = currentHz` **:1022**. |
| **O-IntonationPad** | 2.9.1 | **CLEAN** (SliderState shape — no native fn needed) — no edit, no bump | `Source/ui/public/js/tuning-panel.js:85` `this.masterTuneState = this.juce.getSliderState('tuning_masterTune')`; stretch at **:84**. Reads via `getScaledValue()` at **:99** (and **:90** for stretch). Drag continuity is inherent rather than bolted on: `startValue` is re-read from the live param at **:1071** and **:1088**, so the class of bug fixed in O-MicrotonalSampler cannot occur here. |
| **O-Lyrica** | 2.4.3 | **CLEAN** (own param-bound knob, no shared panel) — no edit, no bump | `Resources/ui/js/app.js:141` `bindSlider('masterTune', …)`. Param→engine push each block: `PluginProcessor.cpp:773` `tuningEngine.setMasterTune(coreCache.masterTune->load())`, stretch **:781**. Param declared **:319**, cached **:574**. JSON save/restore of stretch at **:616 / :676**. |
| **O-Prism** | 1.22.1 | **CLEAN** (own param-bound knob) — no edit, no bump | `Source/ui/public/index.html:2922` `bindKnob('masterTune', masterTuneFmt, 0.5)`; **:2923** `bindKnob('octaveStretch', octStretchFmt, 0.1667)`. |
| **O-Wind** | 1.18.2 | **CLEAN** (shared module panel, both halves) — no edit, no bump | Editor `getMasterTune` **PluginEditor.cpp:337-338**; `setMasterTune` **:341-352** routed through the `referencePitch` PARAM (`setValueNotifyingHost` + `convertTo0to1`) with a comment naming WR-11. Engine `octaveStretch` save **PluginProcessor.cpp:487**, restore **:537-538**. Registers **all 19** names the shared panel calls — `comm -23 called registered` is empty. This is the working reference for the O-Reed recommendation below. |
| **O-Reed** | 1.3.1 | **DIFFERENT, LARGER DEFECT — FOUND, NOT FIXED.** No edit, no bump, no commit. | See the dedicated section below. |

Two corrections to survey wording (neither is a drifted line number):

- O-Reed's mount: the survey cited `index.html:1643`. The observed dynamic import is
  `Resources/ui/index.html:1642` and the **construction call** is **:1645**. Also note
  the UI root is `Resources/ui`, not `Source/ui/public`.
- O-Reed's failure mechanism: the survey said the panel load "throws". It does not —
  see below. This is a material correction, not a wording nit.

## Probe results — O-MicrotonalSampler (Task 1)

`<scratch>/O-MicrotonalSampler/probe-a4.js` — a page-level probe that mounts the
**vendored** panel module in real Chromium against a hand-written `juce` stub, served
from a `mkdtemp` copy of the UI tree assembled by `scripts/serve-ui.js`. No tracked
file was touched to run it — deliberately not `tests/ui-stub/generic-overrides.json`,
which pins `getMasterTune: 440.0` and whose edit would move the check-ui-labels
baseline.

The probe was **written and run before any source edit**, then re-run after. No
`git stash`, no `git checkout --`, no `git restore` was used to produce the "before"
arm.

| Arm | What it asserts | BEFORE | AFTER | Verdict |
|---|---|---|---|---|
| **A** — the fix | stub `getMasterTune` → 442; `#ref-pitch-value` must read `"442.0 Hz"` after `init()` | `"440.0 Hz"` | `"442.0 Hz"` | **FAIL → PASS** |
| **B** — control | stub `getMasterTune` → 440; same path, expects `"440.0 Hz"` | `"440.0 Hz"` | `"440.0 Hz"` | **PASS → PASS** (discriminating, not always-red) |
| **C** — drag continuity | two identical drags (`mousedown` @300, `mousemove` @280, `mouseup`); `end2 > end1` strictly | `450.0` then `450.0` (**equal**) | `452.0` then `462.0` | **FAIL → PASS** |

Arm C's `setMasterTune` call log corroborates it: `[450, 450]` before, `[452, 462]`
after — the second drag was genuinely discarding the first, and now continues from it.
Arm C's seed value also moves with the fix: `"440.0 Hz"` before, `"442.0 Hz"` after.

Raw output: `<scratch>/O-MicrotonalSampler/probe-BEFORE.txt`,
`probe-AFTER.txt`, `probe-TRACER-GATE.txt` (re-run against the committed tree, all
three arms PASS).

## Gate results — O-MicrotonalSampler

| Gate | Result |
|---|---|
| `node scripts/check-i18n.js --plugin O-MicrotonalSampler` | **ALL CHECKS PASS** (assertions 10-15 all PASS; 0/270 French entries unreviewed; canon v2) |
| `node scripts/check-ui-labels.js --plugin O-MicrotonalSampler` | **ALL CHECKS PASSED** — every `[7][GEOMETRY DIFF]` check PASS, i.e. **0 moved**. (27 labels reported as never-visible/never-measured — pre-existing coverage note, reported not asserted, unchanged by this work.) |
| `node scripts/boot-all-uis.js` | **43/43 clean**, 0 warn, 0 failed, **0 DEAD**; **19 late across 2 plugins** (O-Bells 2, O-IntonationPad 17); **O-MicrotonalSampler 0 late** — identical to the pre-edit baseline, including `rendered text-bearing elements: 3788`. Baseline `<scratch>/boot-all-uis-BEFORE.txt` vs `<scratch>/boot-all-uis-AFTER.txt`. |
| Build | `./scripts/build-and-install.sh O-MicrotonalSampler` under the mutex, after `touch CMakeLists.txt`. Complete in 52s; VST3 + AU installed (dev branding, `O-MicrotonalSampler-dev`). |
| `auval -v aumu OMtS OuDv` | **AU VALIDATION SUCCEEDED**, rc 0 |
| Installed `Info.plist` | **1.25.2** (read from `~/Library/Audio/Plug-Ins/Components/`, not the build tree) |
| Build mutex | Acquired at 0s wait, released after `auval`. Confirmed released at end of run. |

## O-Reed — found, not fixed

**No file under `plugins/O-Reed` was modified.** `git status --short -- plugins/O-Reed`
is empty. No version bump, no CHANGELOG entry, no commit.

### The native-fn set difference

Full report: `<scratch>/O-Reed/native-fn-inventory.txt`
(with `registered.txt`, `called.txt`, `missing.txt` alongside).

The editor inventory was taken over the **whole** `Source/` tree, and a second
registration site was actively checked for and ruled out: `PluginEditor.cpp` is the
only file mentioning `NativeFunction` at all, and there is no table-style
`{ "name", lambda }` site (the shape O-MicrotonalSampler uses).

- **Registered by the editor — 9 names**, all preset/language:
  `getCurrentPreset`, `getPresetList`, `getUiLanguage`, `loadPreset`, `savePreset`,
  `savePresetWithDialog`, `selectNextPreset`, `selectPreviousPreset`, `setUiLanguage`
- **Called by the shared panel — 19 names**:
  `applyGeneratedScale`, `exportTuningHTML`, `generateEDO`, `generateHarmonicSeries`,
  `getEmbeddedTuningList`, `getMasterTune`, `getOctaveStretch`, `getTonicNote`,
  `getTuningIntervals`, `getTuningName`, `loadEmbeddedTuning`, `loadKBMFile`,
  `loadScalaFile`, `saveKBMFile`, `saveScalaFile`, `setMasterTune`, `setOctaveStretch`,
  `setSingleInterval`, `setTonicNote`
- **`comm -23 called registered` — 19 of 19 missing. The intersection is empty.**
  Not one native fn the tuning panel calls exists on this plugin.

Required assertions all pass: `missing.txt` non-empty, and contains
`getTuningIntervals`, `getMasterTune` and `setMasterTune` as exact lines.

### The mount (observed)

- `plugins/O-Reed/Resources/ui/index.html:1642` — `await import('/js/tuning-panel.js')`
- `plugins/O-Reed/Resources/ui/index.html:1645` — `module.initTuningPanel(document.getElementById('tuning-container'), Juce)`
- `modules/tuning/scala-tuning-engine/js/tuning-panel.js:1037` — `initTuningPanel` constructs `new TuningPanel(container, juceApi)` and awaits `panel.init()`
- `plugins/O-Reed/CMakeLists.txt:75` — embeds the shared module file
- `plugins/O-Reed/Source/PluginEditor.cpp:410` — serves it from `BinaryData::tuningpanel_js`
- The mount is **lazy**: `index.html:1631-1633` fires it only on a click of the Tuning tab.

### Runtime arm — and what boot-all-uis actually showed

`node scripts/boot-all-uis.js --plugin O-Reed` returned **1/1 clean, 0 warn, 0 failed,
0 DEAD, 0 late, "no i18n runtime diagnostics on any page", exit 0**
(`<scratch>/O-Reed/boot-all-uis.txt`).

**Stated plainly: that run produced no evidence in either direction.** It is not
evidence the panel loads — it is evidence the panel is never reached. The mount sits
behind a tab click a boot render never performs, and the generic ui-stub *invents* a
value for any native fn it does not know, so even if the tab were clicked the stub
would mask the defect. The plan anticipated "aborts" or "does not abort"; the honest
third answer is "not exercised", so a second, purpose-built arm was written rather
than reading a verdict into a null result.

`<scratch>/O-Reed/probe-tuning-disconnect.js` mounts the **shared** module against a
stub restricted to the 9 measured names, reproducing the shipping bridge exactly.
Result (`<scratch>/O-Reed/runtime-arm.txt`):

```
panel.init() completed              : false
panel.init() threw                  : no (nothing to catch)
native fns that HUNG (never settled): ["getTuningIntervals"]
native fns that were SERVED         : []
#ref-pitch-value                    : "440.0 Hz"   (markup default — never read)
#octave-stretch-value               : "1.00"
#interval-list rendered rows        : 0
console errors during the mount     : 0   []
console warnings during the mount   : 0   []

FINDING PROVEN
```

### Correction to the survey's mechanism (material)

The survey stated the panel's `loadInitialState()` **"throws at its FIRST call
(`getTuningIntervals`)"**. It does not throw.

`~/JUCE/modules/juce_gui_extra/misc/juce_WebBrowserComponent.cpp:307-313`
(`handleNativeFunctionCall`): for a name the backend did not register, it hits
`jassertfalse; return;` and **never calls `completeNativeFunctionCall`**. `jassertfalse`
compiles out in Release, so the JS promise is **never settled** — it does not reject,
it hangs. `await this.juce.getNativeFunction('getTuningIntervals')()` therefore never
returns and never enters the `catch`. The `[TuningPanel] Failed to load initial state`
line never fires; **zero console errors, zero warnings**.

The user-visible outcome matches the survey (empty tuning tab, A4 frozen at the
markup's `440.0 Hz`), but the diagnostic consequence is worse: there is no log line to
find. In a real build the only signal is a JS-side `console.warn` at *binding* time —
`~/JUCE/modules/juce_gui_extra/native/javascript/index.js:73-77`, *"Creating native
function binding for '<name>', which is unknown to the backend"*. This is the
`pattern_webview_native_fn_bridge_gap` failure mode in its silent form.

### The false CHANGELOG claim

`plugins/O-Reed/CHANGELOG.md:37` asserts:

> "`referencePitch` is driven by `#ref-pitch-knob` inside the **shared**
> `scala-tuning-engine` tuning panel"

**That does not hold today.** `#ref-pitch-knob` writes via `setMasterTune`, which this
plugin does not register, so the write is swallowed by the same bridge gap and never
reaches the `referencePitch` parameter. The knob drives nothing. (The engine's A4 *is*
driven from the `referencePitch` param every block at
`plugins/O-Reed/Source/PluginProcessor.cpp:411-412`, so the parameter itself is saved
and works from the host — only the panel control is dead.)

### Recommended fix shape (not applied — feature-scale, outside item 22)

Register the full 19-name set on the editor, modelled on
`plugins/O-Wind/Source/PluginEditor.cpp:300-460`. O-Wind mounts the same shared module
and registers all 19 — verified, not assumed: `comm -23 called <O-Wind registered>` is
empty. It is a working reference rather than a sketch.

For the A4 pair specifically use the O-Wind WR-11 shape at
`plugins/O-Wind/Source/PluginEditor.cpp:337-352`: `getMasterTune` reads the engine, and
`setMasterTune` routes through the `referencePitch` **parameter**
(`setValueNotifyingHost` + `convertTo0to1`) rather than writing the engine directly —
O-Reed's engine A4 is already parameter-driven per block, so a direct engine write
would diverge from the APVTS and snap back on reload.

**The minimal item-22 pair alone would be decorative.** `loadInitialState()` hangs at
`getTuningIntervals`, the very first call, and never reaches the A4 read at all.

## PLUGINS.md

Exactly one row changed, located by `grep -n` (observed at line 59, matching the survey):

```
-| O-MicrotonalSampler | 📦 Installed | 1.25.1 | Synth (Microtonal Sampler) | 2026-08-31 |
+| O-MicrotonalSampler | 📦 Installed | 1.25.2 | Synth (Microtonal Sampler) | 2026-08-31 |
```

Union-merge duplicate check prints nothing:
`grep '^| O-' PLUGINS.md | awk -F'|' '{print $2}' | sort | uniq -d` → empty.
`git show --stat 5ecd6df2` → 1 file.

## Deviations from Plan

**1. [Rule 1 — Bug in the plan's gate threshold] `grep -c "currentHz"` expected `>= 4`; the correct value is 3**

- **Found during:** Task 1, step 7 (symbol-presence gate).
- **Issue:** The plan's gate asserts `grep -c "currentHz" …/tuning-panel.js >= 4`. The
  implemented fix yields **3**. The fix requires exactly three uses of the identifier
  (`let currentHz = 440;`, `currentHz = hz;` inside `updateKnob`, `startValue = currentHz;`
  in `mousedown`) and a fourth would be redundant.
- **Verification the threshold, not the code, was wrong:** the reference fix itself —
  `grep -c "currentHz" plugins/O-Formant/Source/ui/public/js/tuning-panel.js` — also
  returns **3**. The port is structurally identical to 420cfe49.
- **Resolution:** recorded the observed count and its three `grep -n` locations
  (`:1034`, `:1037`, `:1056`) instead of asserting a threshold that the reference fix
  would itself fail. No code changed to satisfy a number.
- **Files modified:** none.

**2. [Rule 3 — plan assumed an outcome the tool cannot produce] O-Reed's boot-all-uis runtime arm**

- **Found during:** Task 2, step 5.
- **Issue:** The plan expected `boot-all-uis --plugin O-Reed` to yield "console-error /
  unknownNativeFns evidence that the panel's load aborts", with the only alternative
  being "the finding is wrong". Neither occurred: the run is clean because the panel is
  lazy-mounted behind a tab click and the stub invents values for unknown native fns, so
  the panel is never exercised.
- **Resolution:** reported the null result plainly rather than reading a verdict into it,
  and wrote a purpose-built runtime arm (`probe-tuning-disconnect.js`) that reproduces the
  real bridge semantics. The finding is proven — and the proof corrected the survey's
  stated mechanism from "throws" to "hangs silently".
- **Files modified:** none tracked; scratchpad only.

## Known Stubs

None. No stub, placeholder, TODO or skipped test was introduced.

## Threat Flags

None. The threat register's two `mitigate` dispositions were both implemented as
specified: **T-wq3-01** — the A4 read accepts a value only when
`typeof hz === 'number' && Number.isFinite(hz)` and clamps it to
`Math.max(400, Math.min(480, hz))`; **T-wq3-02** — the read sits in its **own**
`try/catch`, so a throwing native fn cannot skip `updateIntervalList()` /
`updateVisualization()`. T-wq3-02 is not hypothetical: it is precisely the failure mode
O-Reed exhibits, which is why the separate try was kept rather than folded into the
enclosing one. No package was installed (T-wq3-SC holds).

## Self-Check: PASSED

Files verified present:

- `plugins/O-MicrotonalSampler/Source/PluginEditor.cpp` — FOUND, `getMasterTune` ×2 (get + set)
- `plugins/O-MicrotonalSampler/Resources/ui/js/tuning-panel.js` — FOUND, `currentHz` ×3
- `plugins/O-MicrotonalSampler/CHANGELOG.md` — FOUND, `## [1.25.2] - 2026-08-31` present
- `plugins/O-MicrotonalSampler/CMakeLists.txt` — FOUND, `VERSION "1.25.2"` at line 19
- `PLUGINS.md` — FOUND, row at line 59 reads 1.25.2
- `<scratch>/O-MicrotonalSampler/probe-a4.js` — FOUND
- `<scratch>/O-Reed/native-fn-inventory.txt` — FOUND (138 lines)

Commits verified in `git log`: `56fc8bbc` (4 files, all under
`plugins/O-MicrotonalSampler/`), `5ecd6df2` (1 file, `PLUGINS.md`).

Final tree state: `git status --short -- plugins scripts modules` is **empty** — no stray
modification under `scripts/`, `modules/`, or any of the six unbumped plugins. The only
untracked path is this quick task's own `.planning/quick/` directory, left for the
orchestrator's docs commit. `.planning/ROADMAP.md` was not touched.
