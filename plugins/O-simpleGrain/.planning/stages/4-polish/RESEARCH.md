# Stage 4 (Validation / Polish) — Research: Implementation-Ready Reference

**Researched:** 2026-06-25
**Domain:** Final validation gate for a shipped JUCE 8 granular synth — pluginval (VST3) + auval (AU) sweep, offline DSP-harness re-run, factory-preset state audit, fresh-build install discipline, changelog authoring, and the consolidated human DAW-listen checklist.
**Confidence:** HIGH (every command, path, flag, and artifact below is verified against the live repo, the project's `validation-agent` spec, and the already-built Stage-2 harness; no novel tooling).

> **The strategy is gate, not build.** Per `4-polish/CONTEXT.md` (3 locked decisions), Stage 4 writes **no new product code unless a defect surfaces**. The "implementation" here is a sequence of *validation runs* against artifacts that already exist, plus one new doc (CHANGELOG.md). Everything below is a verified recipe; the plan phase just orders these into commits and the execute phase runs them.

---

## User Constraints (from CONTEXT.md — locked 2026-06-25)

| # | Decision | Consequence for this research |
|---|----------|-------------------------------|
| D1 | **Automated first, human listen last (batched).** | Sections 1–5 below are fully automatable and run before any DAW work. Section 6 is the single consolidated human checklist handed over at the end. |
| D2 | **Baseline validation only — no new code unless a defect is found.** | Keep the diff to CHANGELOG.md (+ version bump) unless a run fails. Any fix is in-scope but must be minimal and re-gated. |
| D3 | **Windows deferred entirely to publish/CI.** | No local Windows work. Cross-platform CMake flags already verified static in Stage 1/3. Stage 4 does **not** block on Windows. |

**Success criteria (from CONTEXT.md):** pluginval VST3 pass + auval SUCCEEDED; harness 8/8; 8 presets write valid APVTS (no NaN/denormal/out-of-range); fresh build installed with cache clear + dual-variant sweep; CHANGELOG authored; 7-item human checklist handed over; Windows marked deferred-to-CI.

---

## Validation Surface — what exists to validate

| Artifact | Verified path |
|----------|---------------|
| VST3 (dev branding) | `build/plugins/O-simpleGrain/O-simpleGrain_artefacts/Release/VST3/O-simpleGrain-dev.vst3` |
| AU component (dev) | `build/plugins/O-simpleGrain/O-simpleGrain_artefacts/Release/AU/O-simpleGrain-dev.component` |
| Installed VST3 | `~/Library/Audio/Plug-Ins/VST3/O-simpleGrain-dev.vst3` (present) |
| Installed AU | `~/Library/Audio/Plug-Ins/Components/O-simpleGrain-dev.component` (present) |
| Offline DSP harness | `plugins/O-simpleGrain/tests/render-harness/{main.cpp,CMakeLists.txt}` — console app, target `O-simpleGrain-render-test`, gated by `-DOUARICON_BUILD_TESTS=ON` |
| pluginval CLI | `/Applications/pluginval.app/Contents/MacOS/pluginval` (verified executable) |
| AU identity | `aumu OsGr OuDv` (`PLUGIN_CODE OsGr`, manufacturer `OuDv` dev) |
| Version | `0.1.0` (`plugins/O-simpleGrain/CMakeLists.txt:17`; mirrored in harness `JucePlugin_VersionString="0.1.0"`) |
| CHANGELOG | **Does not exist yet** — must be authored (siblings have one; see §5) |

---

## 1. pluginval — VST3 strictness sweep

**Project-canonical invocation** (from `.claude/agents/validation-agent.md`, the authoritative spec). pluginval locate-pattern:

```bash
if [ -x "/Applications/pluginval.app/Contents/MacOS/pluginval" ]; then
    PLUGINVAL="/Applications/pluginval.app/Contents/MacOS/pluginval"
elif command -v pluginval >/dev/null 2>&1; then
    PLUGINVAL="pluginval"
else echo "pluginval not found"; exit 1; fi

VST3="$HOME/Library/Audio/Plug-Ins/VST3/O-simpleGrain-dev.vst3"
```

**Two-tier run — recommended for Stage 4:**

```bash
# Tier A (primary gate): functional, headless. Mirrors validation-agent Stage 2 flags.
"$PLUGINVAL" --validate "$VST3" --skip-gui-tests --strictness-level 10 --timeout-ms 180000

# Tier B (optional, slower): full suite incl. editor open/close. validation-agent Stage 3 flags.
"$PLUGINVAL" --validate "$VST3" --strictness-level 10 --timeout-ms 600000
```

### Research recommendation: make Tier A the gate; treat Tier B as best-effort.

**Why.** O-simpleGrain's editor is a `juce::WebBrowserComponent` (WKWebView). pluginval's GUI-open tests instantiate the real editor → spins up a WebView process. That is *exactly* the surface CONTEXT defers to the human DAW-listen checklist (live viz/UI). Running Tier B headlessly can be slow/flaky for WebView editors and adds nothing the human listen won't cover. So:

- **Tier A is the recorded pass/fail gate** (parameter automation, state save/restore, thread-safety, allocation-in-process checks, bus layouts) — all the audio-engine correctness that matters for a synth, and all headless-safe.
- **Tier B is run if it completes cleanly; a Tier-B-only GUI-open hiccup is NOT a Stage-4 blocker** — it folds into the human listen. Note any Tier-B result in SUMMARY but gate on Tier A.

**Pass pattern:** `All tests PASSED (N/N)`. **Fail patterns:** `❌ [k/N] … FAIL`, non-zero exit, `Segmentation fault` / `Exception thrown` in output.

**Gotcha — re-validate the INSTALLED bundle, not just the build-tree one.** pluginval loads from the path given; point it at the freshly-installed `~/Library/...` bundle (the one a DAW loads) so the gate matches reality. Install (Section 4) *before* the pluginval run, or run twice.

**Gotcha — thread-safety / allocation checks are the ones most likely to bite a granular synth.** The engine is RT-safe by construction (preallocated `std::array<Grain,24>`/voice, global cap 192, atomic source hot-swap — see STATUS gotchas), so strictness-10 should pass. If `--strictness-level 10` flags an allocation/lock in `processBlock`, that is a real DEF and triggers the D2 "fix-if-defect" path.

---

## 2. auval — AU validation

```bash
auval -v aumu OsGr OuDv          # full AU validation for the dev identity
auval -a | grep -i simplegrain    # confirm registration
```

- **Expected:** `AU VALIDATION SUCCEEDED` (Stage 1 + Stage 3 already reported auval SUCCEEDED; this is a regression re-check on the fresh build).
- **Why both validators:** pluginval's AU support is limited; `auval` is Apple's own AU conformance tool and the project standard for the AU format. `auval` is macOS-only — there is no AU on Windows (CONTEXT D3).
- **Cache discipline is load-bearing for auval** (see Section 4): a stale AudioUnit cache makes `auval` validate the *old* component. Always clear cache + sweep variants *before* the auval run.

---

## 3. Offline DSP harness — 8-gate re-run (regression check)

The Stage-2 harness is the **DSP correctness gate** and the automatable substitute for most "manual-listen" checks. Re-running it proves no regression crept in during Stage 3's editor rewrite (the editor shares `PluginProcessor.cpp`, so an engine-side regression is possible in principle).

**Build + run:**

```bash
cd /Users/taylorbrook/Dev/VST-development
cmake -S . -B build -G Ninja -DOUARICON_BUILD_TESTS=ON      # enable the console-app target
cmake --build build --target O-simpleGrain-render-test
./build/plugins/O-simpleGrain/tests/render-harness/O-simpleGrain-render-test
echo "exit=$?"    # 0 iff all 8 gates pass
```

**The 8 gates** (from `main.cpp` header — these ARE the automatable half of the human checklist):

| # | Gate | Maps to human criterion / req |
|---|------|-------------------------------|
| 1 | makes-sound — held note → non-trivial finite RMS, grains spawn | criterion 1, 4 (FUNC-01) |
| 2 | density→continuity — overlap<1 gaps; overlap>1 fuses | criterion 1 (FUNC-01/DSP-02) |
| 3 | pitch-tracks-MIDI — octave up ≈ 2× spectral centroid | (FUNC-02) |
| 4 | window-rect-clicks — rect injects more top-octave energy than Hann | criterion 6, window inset (DSP-03) |
| 5 | freeze-sustains — freeze on → finite bounded non-silent pad | criterion 5 (FUNC-03/QUAL-01) |
| 6 | scatter-async — scatter 0% peakier than 100% | criterion 2 (DSP-05) |
| 7 | stress-bounded — 5-note chord, max everything → finite, peak bounded, **grains ≤ 192** | (PERF-01/02) |
| 8 | uptranspose-stable — +24 st + high spray stays finite/bounded | AA half of DSP-08 |

**Note:** the harness shares the engine via direct source inclusion (`../../Source/PluginProcessor.cpp` + `PluginEditor.cpp`) and depends on `O-simpleGrain` + `O-simpleGrain_Samples` (for `BinaryData::fire_wav` etc.). It pumps the message loop (`JUCE_MODAL_LOOPS_PERMITTED=1`) so the AsyncUpdater source-decode/hot-swap runs. **No new harness code needed** (D2) — re-run as-is. 8/8 PASS = no regression.

---

## 4. Fresh build + install discipline (cache clear + dual-variant sweep)

**This is the single most regression-prone step in the whole project** (project memory: *dev/release variant shadowing* — a stale `O-simpleGrain.component` would pin Logic's AU slot to the wrong build; the AU cache would make `auval` validate stale bits).

### Recommendation: use the wrapper, do not hand-roll.

```bash
./scripts/build-and-install.sh O-simpleGrain
```

Verified: its **Phase 4** sweeps BOTH the `-dev` and unsuffixed variant bundles and emits `⚠ Sweeping ALTERNATE-variant …` when an orphan is found (`build-and-install.sh:319`); it kills `AudioComponentRegistrar` and clears `~/Library/Caches/AudioUnitCache` (lines 455–457, 422). This is exactly the CLAUDE.md cache-clear sequence, automated.

**If hand-running** (e.g. to install the already-built artifacts without rebuild), the CLAUDE.md sequence is mandatory and must sweep both variants:

```bash
killall -9 AudioComponentRegistrar 2>/dev/null || true
rm -rf ~/Library/Caches/AudioUnitCache/ ~/Library/Caches/com.apple.audiounits.cache
# sweep BOTH variants (dev + unsuffixed) for VST3 and AU before copying fresh:
rm -rf ~/Library/Audio/Plug-Ins/VST3/O-simpleGrain.vst3 ~/Library/Audio/Plug-Ins/VST3/O-simpleGrain-dev.vst3
rm -rf ~/Library/Audio/Plug-Ins/Components/O-simpleGrain.component ~/Library/Audio/Plug-Ins/Components/O-simpleGrain-dev.component
cp -R build/plugins/O-simpleGrain/O-simpleGrain_artefacts/Release/VST3/O-simpleGrain-dev.vst3 ~/Library/Audio/Plug-Ins/VST3/
cp -R build/plugins/O-simpleGrain/O-simpleGrain_artefacts/Release/AU/O-simpleGrain-dev.component ~/Library/Audio/Plug-Ins/Components/
```

**Ordering matters:** build → cache-clear+sweep+install → *then* `auval` and pluginval (so both validate the fresh, correctly-registered bundle).

---

## 5. Factory-preset state audit (8 snapshots → valid APVTS)

**What to prove:** each of the 8 `applyFactoryPreset` snapshots writes APVTS state that is in-range, finite (no NaN/Inf), and denormal-free, and produces bounded non-silent audio.

**The 8 presets** (verified parity — 8 `data-preset` buttons in `index.html` ≡ 8 `name ==` branches in `PluginProcessor.cpp:788+`):

`Single Grain` · `Pitched Buzz` · `Fragments` · `Smooth Cloud` · `Frozen Pad` · `Asynchronous Cloud` · `Granular Fire` · `Rect Click`

### Two complementary audit methods:

**(a) Static/state audit — no new code (preferred under D2).**
`applyFactoryPreset` calls `setValueNotifyingHost` on parameters whose `NormalisableRange` already clamps to [min,max] — so an out-of-range write is structurally impossible and a denormal can't be stored in a clamped float param. The audit reduces to **confirming each branch only touches valid param IDs with literals inside their declared ranges**. Read the 8 branches in `PluginProcessor.cpp` against `parameter-spec.md` ranges; this is a desk-check, zero code, and satisfies "writes valid APVTS without NaN/out-of-range."

**(b) Audible/render audit — covered two ways:**
- The harness gates *already exercise the preset concepts* via raw params (gate 1 makes-sound, gate 5 freeze≈Frozen Pad, gate 6 scatter≈Asynchronous Cloud, gate 4 rect≈Rect Click, gate 2 density≈Smooth Cloud). So preset *character* is indirectly validated.
- **Audible distinctness per preset is a human-ear criterion** (CONTEXT marks "audible char = human") → folds into the listen checklist item 6.

### Recommendation
Do the **(a) static desk-check** as the Stage-4 automatable gate (records "8/8 presets write in-range finite APVTS"); defer **audible distinctness** to the human checklist. **Do NOT write a new per-preset render harness** — that is new test code, out of scope under D2 unless the desk-check surfaces a suspect literal. If a literal looks out-of-range, *then* a targeted render check is justified.

---

## 6. Changelog authoring + version decision

**No CHANGELOG.md exists** for O-simpleGrain. Author one following the sibling pattern.

**Pattern (verified — `O-simpleFM/CHANGELOG.md`):**
- Title `# Changelog — O-simpleGrain`; "Format loosely follows [Keep a Changelog]."
- Reverse-chronological `## [version] — YYYY-MM-DD` blocks.
- Subsections: **Added / Changed / Fixed / Validation**. The sibling's `### Validation` line records "build + auval/pluginval pass" — replicate that, citing the Stage-4 gates.

**Version decision — for the plan to confirm with the user:**
- Current is `0.1.0` (pre-release). This is the **first complete, validated build** of a 4-stage plugin.
- **Recommended:** author the changelog as the **`1.0.0` initial release** entry (or `0.1.0 → 1.0.0` bump) summarizing the whole build (granular engine, 18 params, WebView field-guide UI, 8 presets, drag-drop source load) — since Stage 4 is the ship gate. Bump `VERSION "0.1.0"` in `CMakeLists.txt:17` **and** the harness's `JucePlugin_VersionString`/`JucePlugin_VersionCode` to match.
- **Alternative:** keep `0.1.0` and let the `/publish` stage own the 1.0.0 bump. Either is defensible; **flag this as the one decision the plan should confirm** (the version bump is the only product-file edit Stage 4 makes under D2, so it deserves an explicit nod).

**Single-source-of-truth note:** version appears in **two** files (`plugins/O-simpleGrain/CMakeLists.txt:17` and `tests/render-harness/CMakeLists.txt` `JucePlugin_VersionString`/`Code`). Bump both or the harness build will report a stale version (cosmetic, but catch it).

---

## 7. The consolidated human DAW-listen checklist (handed over LAST)

The 7 deferred Stage-3 runtime criteria (cannot be driven headlessly) become the single batched checklist at the end of Stage 4. Load `O-simpleGrain-dev` (`aumu OsGr OuDv`) in a DAW/Standalone with MIDI and confirm:

1. **Grain cloud accumulates** — density thickens it, spray widens it (UI-01).
2. **Spectrum** shows discrete sidebands at scatter=0, smears to noise at high scatter (UI-04/DSP-05).
3. **Scope** moves with output (UI-04).
4. **Grain/overlap/CPU readout** counts `N/192` live (UI-05).
5. **Freeze pins the playhead** — ❄ pin + shaded spray band; freeze/unfreeze click-free (FUNC-03/QUAL-01).
6. **Window inset** redraws on Window combo change (UI-03); **8 presets** each snap knobs/combos/toggle + caption/active state and sound audibly distinct (FUNC-06); **every control** shows its hover tooltip (FUNC-07).
7. **Drag-drop a .wav AND Load…** both granulate a user source (FUNC-05); **host-automation → UI** round-trip.

Stage-4 verify records these as `human_needed` until the user confirms (CONTEXT success criterion 6).

---

## Pitfalls (from project memory + this stage's shape)

| Pitfall | Mitigation |
|---------|-----------|
| **Stale AU cache → `auval` validates old bits** | Clear `~/Library/Caches/AudioUnitCache/` + kill `AudioComponentRegistrar` *before* auval. `build-and-install.sh` Phase 4 does this. |
| **Dev/release variant shadowing** (project memory) | Sweep BOTH `O-simpleGrain.{vst3,component}` and `O-simpleGrain-dev.{…}` before install. Wrapper warns `⚠ Sweeping ALTERNATE-variant`. |
| **pluginval GUI-open flaky/slow for WebView editor** | Gate on Tier A (`--skip-gui-tests`); treat Tier B as best-effort, fold live-UI into the human listen. |
| **`MemoryBlock::fromBase64Encoding` vs `Base64::convertFromBase64`** (project memory) | Already correct in-tree from Stage 2.3; only re-touch if drag-drop smoke test fails. No change planned. |
| **Version drift across 2 CMake files** | Bump `CMakeLists.txt:17` and harness `JucePlugin_Version*` together. |
| **Writing new test code** | D2 forbids it unless a defect surfaces. Re-run existing harness; desk-check presets; do not author new render tests speculatively. |
| **Windows scope creep** | D3 — explicitly out of scope; CMake flags already static-verified. Record "deferred to CI", do not run/build locally. |

---

## Recommended Stage-4 execution order (for the plan phase)

1. **Build + install fresh** — `./scripts/build-and-install.sh O-simpleGrain` (cache clear + dual-variant sweep). *Commit boundary: none — prep.*
2. **auval** — `auval -v aumu OsGr OuDv` → expect SUCCEEDED.
3. **pluginval Tier A** — `--skip-gui-tests --strictness-level 10` on the installed VST3 → expect `All tests PASSED`. (Tier B best-effort.)
4. **Harness re-run** — `cmake -DOUARICON_BUILD_TESTS=ON` build + run → expect 8/8, exit 0.
5. **Preset state desk-check** — 8 branches vs `parameter-spec.md` ranges → in-range/finite.
6. **CHANGELOG.md** + version decision (confirm 1.0.0 vs hold) → author; bump both CMake files if bumping. *Commit: the only product-file diff.*
7. **Hand over the 7-item human listen checklist** — record verify as `human_needed`. Mark Windows deferred-to-CI.

A defect at any step → minimal in-scope fix → re-run that gate (D2).

---

## Open Questions for the Plan Phase

1. **Version:** bump `0.1.0 → 1.0.0` now (recommended — Stage 4 is the ship gate) or hold for `/publish`? *(Only product-file edit at stake.)*
2. **pluginval Tier B:** attempt the full GUI-open suite, or skip entirely and rely on the human listen for UI? *(Research leans skip/best-effort.)*
3. **Changelog granularity:** single `1.0.0` "initial release" entry summarizing all 4 stages, or stage-by-stage retro entries? *(Recommend single initial-release entry.)*
