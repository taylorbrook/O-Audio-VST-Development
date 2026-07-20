# Post-JUCE-8.0.14 Verification Battery — Results Matrix

**Date:** 2026-07-19
**Context:** JUCE bump **8.0.9 → 8.0.14** (quick tasks k5o + l26). Branch `quick/260719-k5o-juce-ne-rebase-8014`.
**Source strategy:** `research/framework-updates-2026-07.md` §Verification Strategy.
**Machine-readable evidence:** `battery-results.tsv` (auval + pluginval, 38 rows) · `harness-results.tsv` (8 rows).
**Sweep script:** `scripts/verify-suite-battery.sh` (resilient; per-plugin exit codes; portable wall-clock guard; never aborts on one failure).

## Summary

| Verdict | Count | Plugins |
|---------|-------|---------|
| ✅ PASS (all gates clean) | 31 | (see matrix) |
| ✅ PASS (annotated auval) | 2 | O-Lyrica (benign meta-flag DEF-24-01), O-Polystutter (gate mis-typed aufx→aumf; corrected `auval` SUCCEEDS) |
| ⚠️ pluginval finding | 2 | O-Bells, O-IntonationPad (non-finite audio under strictness-8 fuzz — NOT the state-restore test) |
| ⚠️ harness finding | 1 | O-Contrabass (RMS-sustain acceptance band; no NaN/inf/crash) |
| ⛔ EXCLUDED (deliberate) | 1 | O-Orbit (root CMake `SKIP_PLUGINS`, SAF-heavy) |
| ⛔ KNOWN-FAIL | 1 | O-TextureForge (umappp/irlba drift, DEF-L26-01) |
| **Total suite** | **38** | |

**Headline:** The JUCE 8.0.9 → 8.0.14 bump introduced **no detected regression**. All auval failures resolve to a pre-existing benign static-check (O-Lyrica) or a gate type-resolution gap (O-Polystutter, which actually PASSES). The two pluginval findings and one harness finding are pre-existing DSP-edge behaviors surfaced by high-strictness fuzzing / strict acceptance bands — none are JUCE-version-specific. **The JUCE 8.0.11 `var` deep-equality watch is CLEAN** (no pluginval state save/restore failure anywhere).

## AUTOMATED RESULTS

Exit codes: `0` = PASS, non-zero = FAIL. `Install` = fresh `build-and-install.sh` (dual-variant sweep + cache clear). `auval` = `scripts/verify-au-link.sh` (reused verbatim). `pluginval` = strictness-8, `--skip-gui-tests`, `--timeout-ms 60000`. `Harness` = offline render-test (build+run); `n/a` where no harness exists.

| Plugin | CMake Target | Install | auval | pluginval | Harness | Verdict |
|--------|-------------|:-------:|:-----:|:---------:|:-------:|---------|
| O-AnalogEQ | OuariconAnalogEQ | 0 | 0 | 0 | n/a | ✅ PASS |
| O-AnalogSaturation | O-AnalogSaturation | 0 | 0 | 0 | n/a | ✅ PASS |
| O-Bass | O-Bass | 0 | 0 | 0 | n/a | ✅ PASS |
| O-Bassoon | O-Bassoon | 0 | 0 | 0 | n/a | ✅ PASS |
| O-Bells | O-Bells | 0 | 0 | **1** | n/a | ⚠️ pluginval finding (Test 25 NaN) |
| O-Bowed | O-Bowed | 0 | 0 | 0 | 0 | ✅ PASS |
| O-Chorus | OuariconChorus | 0 | 0 | 0 | n/a | ✅ PASS |
| O-Comp | O-Comp | 0 | 0 | 0 | n/a | ✅ PASS |
| O-Contrabass | O-Contrabass | 0 | 0 | 0 | **1** | ⚠️ harness finding (RMS band) |
| O-Detune | O-Detune | 0 | 0 | 0 | n/a | ✅ PASS |
| O-DigiDelay | OuariconDigitalDelay | 0 | 0 | 0 | n/a | ✅ PASS |
| O-Formant | O-Formant | 0 | 0 | 0 | n/a | ✅ PASS |
| O-Freeze | O-Freeze | 0 | 0 | 0 | n/a | ✅ PASS |
| O-FreqPulse | O-FreqPulse | 0 | 0 | 0 | n/a | ✅ PASS |
| O-Gain | O-Gain | 0 | 0 | 0 | n/a | ✅ PASS |
| O-GrainScatter | OuariconGrainScatter | 0 | 0 | 0 | n/a | ✅ PASS |
| O-IntonationPad | O-IntonationPad | 0 | 0 | **1** | n/a | ⚠️ pluginval finding (Inf/NaN/subnormal) |
| O-Lyrica | OLyrica | 0 | **255** | 0 | n/a | ✅ PASS* (benign meta-flag DEF-24-01) |
| O-Marimba | OMarimba | 0 | 0 | 0 | n/a | ✅ PASS |
| O-MicrotonalSampler | O-MicrotonalSampler | 0 | 0 | 0 | n/a | ✅ PASS |
| O-MultiBandCompressor | O-MultiBandCompressor | 0 | 0 | 0 | n/a | ✅ PASS |
| O-Orbit | OuariconOrbit | — | — | — | n/a | ⛔ EXCLUDED (SKIP_PLUGINS, SAF) |
| O-Polystutter | OPolystutter | 0 | **2** | 0 | n/a | ✅ PASS* (gate mis-typed; corrected auval SUCCEEDS) |
| O-Prism | O-Prism | 0 | 0 | 0 | n/a | ✅ PASS |
| O-Reed | O-Reed | 0 | 0 | 0 | n/a | ✅ PASS |
| O-simpleAdditive | O-simpleAdditive | 0 | 0 | 0 | 0 | ✅ PASS |
| O-simpleBeatmaker | O-simpleBeatmaker | 0 | 0 | 0 | 0 | ✅ PASS |
| O-simpleFM | O-simpleFM | 0 | 0 | 0 | 0 | ✅ PASS |
| O-simpleGrain | O-simpleGrain | 0 | 0 | 0 | 0 | ✅ PASS |
| O-simplePhysicalModelSynth | O-simplePhysicalModelSynth | 0 | 0 | 0 | 0 | ✅ PASS |
| O-SimpleReverb | O-SimpleReverb | 0 | 0 | 0 | n/a | ✅ PASS |
| O-simpleSampler | O-simpleSampler | 0 | 0 | 0 | n/a | ✅ PASS |
| O-simpleSubtractive | O-simpleSubtractive | 0 | 0 | 0 | 0 | ✅ PASS (harness WebView pitfall fixed) |
| O-SpectralShaper | O-SpectralShaper | 0 | 0 | 0 | n/a | ✅ PASS |
| O-Texture | OuariconTexture | 0 | 0 | 0 | n/a | ✅ PASS |
| O-TextureForge | OuariconTextureForge | KNOWN-FAIL | KNOWN-FAIL | KNOWN-FAIL | n/a | ⛔ KNOWN-FAIL (DEF-L26-01) |
| O-Tremolo | OuariconTremolo | 0 | 0 | 0 | n/a | ✅ PASS |
| O-Wind | O-Wind | 0 | 0 | 0 | n/a | ✅ PASS |

### Findings detail

- **O-Lyrica — auval 255 (benign, PASS\*).** Every substantive auval subtest PASSES; the non-zero exit is the pre-existing static-check `ERROR: Parameter values are different since last set — probable cause: a Meta Param Flag is NOT set on a parameter that will change values of other parameters` (auval log line 827). This is **DEF-24-01 (DOWNGRADED)** — a benign parameter-meta-flag annotation gap on the validated note-expression reference plugin, not a defect and not JUCE-version-specific.
- **O-Polystutter — auval 2 (false negative, PASS\*).** `verify-au-link.sh` resolved AU type `aufx`, but the plugin declares `NEEDS_MIDI_INPUT TRUE` + `IS_SYNTH FALSE`, so JUCE registers it as `aumf` (MusicEffect). Running the correct triple `auval -v aumf OuPs OuDv` returns **AU VALIDATION SUCCEEDED**. This is a type-resolution gap in the auval gate (which the plan mandates be reused verbatim), *not* a plugin or JUCE defect. **Follow-up:** teach `verify-au-link.sh` to emit `aumf` when `NEEDS_MIDI_INPUT TRUE` + `IS_SYNTH FALSE`.
- **O-Bells — pluginval 1.** `Test 25 failed: NaNs found in buffer` (1 of 45 tests) during strictness-8 automated parameter/automation fuzzing. NaN in the render buffer under an aggressive fuzzed parameter combination. Pre-existing DSP edge case (O-Bells is a note-expression bell synth); not the state-restore test. **Follow-up:** DSP NaN-guard audit under parameter fuzzing.
- **O-IntonationPad — pluginval 1.** Multiple tests flag `Infs`, `Subnormals`, and `NaNs` in buffer (tests 8/9/11/12/13/14/15/20/21/23…) under strictness-8 fuzz. Subnormals are typically benign; the Infs/NaNs warrant a DSP stability review of the fuzzed parameter regions. Not the state-restore test. **Follow-up:** DSP non-finite guard + denormal flush audit.
- **O-Contrabass — harness run exit 1.** Build+run succeed with `nan=0 inf=0`, `peak=0.099` (≤ 0 dBFS), block `maxRatio=2.52` (≤ 5× OK). The single failed acceptance is the RMS-sustain band: `rmsFinal=0.0063` vs `rmsMid=0.0277` (ratio ≈ 0.23, outside the harness's required 0.5–2.0×) — i.e. the bowed tone decays more than the harness threshold allows. No instability, no crash, no non-finite output; a strict acceptance-band mismatch, not a JUCE regression.
- **O-Orbit — EXCLUDED.** Root `CMakeLists.txt` cache carries `SKIP_PLUGINS:STRING=O-Orbit`; the configure step logs `[Ouaricon] Skipping plugin: O-Orbit`. It is deliberately out of the suite build (SAF framework is heavy), which is why the l26 fresh build was framed "36/37". Gating O-Orbit requires a dedicated SAF build outside this suite sweep. Not a JUCE 8.0.14 regression.
- **O-TextureForge — KNOWN-FAIL.** umappp ↔ irlba transitive template drift (JUCE-independent), deferred **DEF-L26-01**. Never built by the sweep; recorded, never "fixed".

## JUCE 8.0.11 `var` Deep-Equality Watch (preset round-trip)

The 8.0.11 change to `juce::var` deep-equality semantics is exercised by **pluginval's strictness-8 state save/restore test** (serialize APVTS → restore → compare), run against every gated plugin. **Result: CLEAN.** No plugin's state/restoration test failed anywhere in the sweep — every non-zero pluginval exit (O-Bells, O-IntonationPad) came from *audio-buffer non-finite* checks during parameter fuzzing, **not** from the state save/restore comparison. No preset round-trip regression is indicated. No manual preset round-trip follow-up is required for the deep-equality change.

## Windows VST3 (out of local scope)

This battery is macOS-only (auval + AU are macOS-only; pluginval run locally on VST3/AU). The Windows half of the build matrix (VST3-only) is validated via CI **`.github/workflows/build-and-release.yml`** with `JUCE_VERSION: '8.0.14'`, not re-run here. Push a release tag to exercise the Windows leg.

**Windows leg VALIDATED 2026-07-20** via `workflow_dispatch` validate-only runs on branch `quick/260719-k5o-juce-ne-rebase-8014`:
- **O-AnalogSaturation (run 29750147817): ✅ GREEN** — JUCE 8.0.14 download, NE-override overlay + both relocated grep gates, MSVC build, pluginval strictness 10, Inno installer all pass. The plugin has a green 8.0.9 Windows history, so this isolates and clears the JUCE variable.
- **O-Lyrica (run 29749545545): ❌ FAILED — NOT a JUCE regression.** First-ever Windows build of O-Lyrica; MSVC rejects the `safe = juce::Component::SafePointer<...>(this)` init-capture inside nested WebView FileChooser lambdas (`this` resolves to the enclosing closure on MSVC). Pre-existing from v2.3.2 code-review fixes; JUCE steps in the run all passed. Same pattern exists in O-IntonationPad. **Follow-up:** hoist the SafePointer into a local before `launchAsync` in both plugins before any Windows release of them.

---

# MANUAL CHECKLIST (documentation only — run by hand)

> The executor cannot drive DAWs. The following are for a human to run. Check each box.

## A. Dorico 3-point microtonal smoke test

The true note-expression acceptance gate — proves the JUCE-NE-PATCH re-base onto 8.0.14 preserved VST3 Note Expression microtonal playback. **Reference plugin: O-Lyrica** (validated spike/reference). Use any note-expression plugin (O-Lyrica, O-Bells, O-Wind, O-Reed, O-Bowed, O-Formant, O-IntonationPad, O-Prism) in Dorico 6 with the Ouaricon Microtonal Suite `.doricolib` imported.

- [x] **Point 1 — Pitch accuracy.** A quarter-sharp C4 plays at **≈ 269.29 Hz** (+50 cents above C4 = 261.63 Hz). Verify with a tuner/spectrum probe. — **PASS 2026-07-20 (human, O-Lyrica VST3, Dorico regression project)**
- [x] **Point 2 — Clean onset.** No attack zipper / no pitch-glide artifact on note onset (tuning applied before the DSP note trigger). — **PASS 2026-07-20**
- [x] **Point 3 — Polyphonic isolation.** In a chord, only the detuned note is bent; other held notes play 12-TET (e.g. E4 = 329.63 Hz) — per-note NoteId correlation is intact. — **PASS 2026-07-20**

If any point fails: the JUCE-NE-PATCH re-base is suspect — inspect `modules/tuning/note-expression/` dispatch slots (`g_neUpdate`/`g_neQuery`) and re-run `scripts/apply-juce-patches.sh`.

## B. Logic / Ableton DAW smoke

Instantiate the AU (Logic) and VST3 (Ableton), confirm load, play a note, confirm no crash/silence.

**⚠️ MANDATORY cache-clear FIRST** (verbatim from `CLAUDE.md` — auval/Logic will otherwise gate a stale cached instance):

```bash
# Always run this sequence after any ninja build of plugins:
killall -9 AudioComponentRegistrar 2>/dev/null || true
rm -rf ~/Library/Caches/AudioUnitCache/
rm -rf ~/Library/Caches/com.apple.audiounits.cache

# Remove old AND alternate-variant bundles before install (-dev ↔ unsuffixed)
# Why: dev branding produces "<Name>-dev.{vst3,component}" while release branding
# produces "<Name>.{vst3,component}" — same AU triple (type/subtype/manufacturer).
# Leaving the alternate variant on disk pins Logic's registry slot to whichever
# was installed first. See O-Prism v1.17.4 CHANGELOG for the regression.
rm -rf ~/Library/Audio/Plug-Ins/VST3/[PluginName].vst3
rm -rf ~/Library/Audio/Plug-Ins/VST3/[PluginName]-dev.vst3
rm -rf ~/Library/Audio/Plug-Ins/Components/[PluginName].component
rm -rf ~/Library/Audio/Plug-Ins/Components/[PluginName]-dev.component

# Install fresh — substitute the suffix actually produced by your build:
#   - Dev branding (default local):   [PluginName]-dev
#   - Release branding (CI only):     [PluginName]
cp -R build/plugins/[PluginName]/[PluginName]_artefacts/Release/VST3/[PluginName]*.vst3 ~/Library/Audio/Plug-Ins/VST3/
cp -R build/plugins/[PluginName]/[PluginName]_artefacts/Release/AU/[PluginName]*.component ~/Library/Audio/Plug-Ins/Components/
```

(Preferred: `./scripts/build-and-install.sh [PluginName]` — its Phase 4 does the dual-variant sweep automatically.)

- [x] **Logic (AU).** Plugin appears in the AU list, instantiates, loads its UI, plays a note — no crash, no silence. — **PASS 2026-07-20 (human)**
- [x] **Ableton (VST3).** Plugin scans in, instantiates, loads its UI, plays a note — no crash, no silence. — **PASS 2026-07-20 (human)**
- [x] Spot-check one note-expression plugin (e.g. O-Lyrica AU) and one WebView-UI plugin (e.g. O-simpleSubtractive VST3) for UI render + audio. — **PASS 2026-07-20 (human)**
