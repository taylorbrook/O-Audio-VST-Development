---
title: "O-MicrotonalSampler Phase 4.1 — Version-pill plumbing"
created: 2026-04-28
phase: 4.1
status: complete
verifies_requirements: []
---

# Phase 4.1 — Version-pill plumbing (SUMMARY)

## Goal recap

Replace the hard-coded `v0.1.0` literal in `index.html` `.about-card`
with a runtime-resolved `JucePlugin_VersionString` via a
`getPluginVersion` native function, mirror-of
`O-FreqPulse/Source/PluginEditor.cpp:215`.

## What landed

| Task | Outcome |
|---|---|
| 1. `getPluginVersion` native function | Inserted in `Source/PluginEditor.cpp` between `getOctaveStretch` and `getEmbeddedTuningList`. Returns `juce::var(JucePlugin_VersionString)` — verbatim shape from O-FreqPulse:215. |
| 2. HTML version-pill empty div | `Resources/ui/index.html:100` now `<div class="about-version" id="about-version"></div>`. No `v0.1.0` literal anywhere in `Resources/` or `Source/`. |
| 3. `refreshAboutVersion` JS + JUCE-init wire-up | Added to `Resources/ui/js/sampler-app.js` modeled on `refreshTuningReadout`. Called once at the `DOMContentLoaded` boot block alongside the existing `refreshTuningReadout()` call (the actual single-shot init site — the plan's "around line 256" reference is the per-tab-activation handler; the boot block is the correct single-shot location). |
| 4. Phase 4.1 gate | Triple build green; cache-clear + install (`-dev` variant); pluginval-5 SUCCESS; auval AU VALIDATION SUCCEEDED. |

## Plan deviation note

**Wire-up site:** Plan §Task 3 cited "around line 256" of
`sampler-app.js`. That line is inside the `activateTab` handler — it
fires every time the user clicks the Tuning tab, NOT JUCE-init. The
plan's stated intent is "single shot, no cost, no need to gate on
About-tab activation." The DOMContentLoaded boot block (around line
1313) is the actual single-shot init site that already calls
`refreshTuningReadout()`, so `refreshAboutVersion()` was wired in
there. This satisfies the plan's intent verbatim.

## Gate evidence

### Triple build (Release, dev branding)

```
ninja O-MicrotonalSampler_VST3 O-MicrotonalSampler_AU O-MicrotonalSampler_Standalone
```

- `O-MicrotonalSampler-dev.vst3` linked + ad-hoc signed
- `O-MicrotonalSampler-dev.component` linked
- `O-MicrotonalSampler-dev.app` (Standalone) linked
- 19/19 targets green; only pre-existing 3rd-party warnings
  (VST3 SDK `FUnknown` shadow + `delete-non-abstract-non-virtual-dtor`)
  surfaced.

### Cache-clear + install (per CLAUDE.md)

- `killall -9 AudioComponentRegistrar`
- `rm -rf ~/Library/Caches/AudioUnitCache/`
- `rm -rf ~/Library/Caches/com.apple.audiounits.cache`
- Removed prior `O-MicrotonalSampler-dev.{vst3,component}` from system folders
- Copied fresh bundles into `~/Library/Audio/Plug-Ins/{VST3,Components}/`

### Invariant greps

```
grep -rn "v0\.1\.0" plugins/O-MicrotonalSampler/Resources/ plugins/O-MicrotonalSampler/Source/
→ exit 1 (zero hits)  ✓
```

### Smoke validations

- **pluginval-5**: `--strictness-level 5 --validate-in-process --skip-gui-tests` → `SUCCESS`
- **auval**: `aumu OMtS OuDv` → `AU VALIDATION SUCCEEDED`

(Strictness-10 lives in Phase 4.4; this is the smoke pass.)

### Visual confirmation (deferred to user)

Visual confirmation that `v1.0.0` shows in the About-tab pill is the
single human-eyes step. Standalone is installed at
`~/Library/Audio/Plug-Ins` (and Standalone app at
`build/.../Standalone/O-MicrotonalSampler-dev.app`). User to confirm
during Phase 4.2 setup or via `/show-standalone`.

## Files modified

- `Source/PluginEditor.cpp` — `getPluginVersion` block added
- `Resources/ui/index.html` — version pill div now empty + IDed
- `Resources/ui/js/sampler-app.js` — `refreshAboutVersion` function + boot wire-up

## Files NOT modified (invariants held)

- All Stage 2 audio-thread code paths (`MicrotonalSamplerVoice`,
  `LoopDetector`, `SampleLoader` audio path) — frozen.
- `CMakeLists.txt` — `PLUGIN_VERSION "1.0.0"` is the source of truth, untouched.
- `modules.json` — no new module deps.

## Next phase

Phase 4.2 — PERF-02 Logic Pro CPU-meter measurement.
**User-driven manual measurement** per RESEARCH §RQ4-3 protocol.
