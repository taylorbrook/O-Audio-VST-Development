---
milestone: audio-rate-pulsar-grain-mode
domain: mixed
execute_agent: general-purpose
version_bump: major
base_version: 1.9.0
target_version: 2.0.0
created: 2026-10-28
---

# Plan: Audio-Rate Pulsar Grain Mode

**Plugin:** O-simpleGrain
**Milestone:** audio-rate-pulsar-grain-mode
**Created:** 2026-10-28
**Phase:** Plan

---

## Summary

**Improvement:** a second synthesis mode, Pulsar, beside the Cloud engine. The key sets the pulse rate f_p, Formant sets f_form/f_p (= 1/duty), and the content is a one-cycle sine or a slice of the loaded sample. A window-aware Nyquist clamp, auto Blackman-Harris pulsarets under ~1 ms and fractional onsets keep it alias-free. Extras: deterministic Mask, random Skip, sine rate FM, Glisson.
**Approach:** a separate double-precision pulse clock and a `g.pulsaret` render branch inside `GrainVoice`, sharing the existing 24 + 4 grain pool. Cloud code is branched around, never edited, so it stays bit-exact vs 1.9.0 by construction, and a hard null gate proves it before any UI work lands.
**Version:** 1.9.0 → 2.0.0 (MAJOR, the user's choice for a headline mode; the change is additive)

### Decisions settled in Plan (open questions from RESEARCH.md)

| # | Question | Decision |
|---|---|---|
| 1 | Duty level compensation | **None.** Pulsar normGain = per-pulsaret window-power term only. Formant 32× sits ~15 dB under 1×, which is the pulsar character. |
| 2 | Mask convention | **Mute 1 of N:** pulse k is muted when `k mod N == N − 1`. |
| 3 | Default content | **Sine.** |
| 4 | Formant ceiling | **Accept `kFormantCeil = 0.10·fs`** (4.41 kHz at 44.1 kHz, 9.6 kHz at 96 kHz). Multi-cycle pulsarets are noted as a v2.1 candidate in the CHANGELOG. |
| 5 | Lesson presets | **Yes, 2:** "Pulsar Vowel" and "Subharmonic Mask". The header lesson row grows to 10 buttons and is re-pinned at the widest language. |

### Corrections carried from RESEARCH.md
- Polyphony is **8** voices (`kMaxVoices = 8`). CPU is measured at 8.
- The clamp is **window-aware**: `f_form,max ≤ 0.10·fs`, applied to the highest content rate (`f_form · 2^(max(0,glisson)/12) · 2^(pitchSpray/12)`), not the §3.6 `0.45·fs`.
- Pitch bend stays out of scope (`pitchWheelMoved` is empty in both modes).

### New parameters (appended, `ParameterID{…, 2}`, existing IDs/ranges/order untouched)

| ID | Type | Range | Default | Smoothed |
|---|---|---|---|---|
| `grainMode` | Choice | Cloud, Pulsar | Cloud | — |
| `formant` | Float, log skew | 1–32× | 4× | per-sample linear ramp |
| `pulsaretSource` | Choice | Sine, Sample | Sine | — (latched per pulsaret) |
| `pulseMask` | Choice | Off, 2…8 | Off | — |
| `fmDepth` | Float | 0–100 % → D ∈ [0, 0.95] linear | 0 % | per-sample linear ramp |
| `fmRatio` | Float, skew centre 1 | 0.25–8 | 1 | — |
| `glisson` | Float | −24…+24 st | 0 | — (latched per pulsaret) |

---

## Wave A — DSP (Cloud null gate is a hard stop before Wave B)

### Task A0: Pre-flight — reference build, measurements, reader contract

**Outcome:** a seeded out-of-tree 1.9.0 reference VST3 exists for nulls; the research numbers and the BandlimitedReader contract are confirmed before any engine code changes.

**Steps:**
- Create backup `backups/O-simpleGrain/v1.9.0/` (skill pre-execute step; HALT if it fails).
- Build the 1.9.0 reference from `git archive HEAD` into the scratchpad with a fixed seed, copying in the gitignored `Source/ui/public/modules/*.js` (memory: `pattern_null_test_needs_seeded_out_of_tree_pair`, `pattern_git_archive_ui_tree_misses_gitignored_module_copies`).
- Re-run the numpy check: BH μ₁ = 0.35875, μ₂ = 0.25796; one-cycle BH pulsaret −60 dB extent = 4.61× f_form.
- Read `modules/synthesis/grain-engine/cpp/BandlimitedReader.h`: confirm `SincBank::setFor` accepts an over-filtered residual in [1, 2) and that `read()` takes positions in level-0 units.

**Verification:**
- [x] Backup exists and diffs clean against HEAD.
- [x] Reference VST3 loads in pedalboard and renders the existing blocksize-null configs.
- [x] Measurements match RESEARCH.md within 1 % (μ₁ 0.35875, μ₂ 0.257963, −60 dB extent 4.62×); reader contract in the Wave A report (SUMMARY.md still to be written from it — the subagent harness refused the .md write).

**Dependencies:** None
**Estimated effort:** Small

---

### Task A1: Parameters and plumbing (no audible change)

**Outcome:** 7 new parameters exist, are loaded into `GrainVoiceParams` each block, and nothing reads them yet.

**Files:** `PluginProcessor.h` (ParamIDs, pointers, `kFormantCeilRatio = 0.10`), `PluginProcessor.cpp` (layout appended with version hint 2, per O-TextureForge `PluginProcessor.cpp:172`), `GrainVoice.h` (`GrainVoiceParams` fields only).

**Verification:**
- [x] Build clean, no new warnings.
- [x] Parameter count +7; existing IDs, ranges, defaults and order unchanged (diff of the layout section shows only an appended block).
- [x] Cloud null vs the A0 reference: bit-exact on every blocksize-null config.

**Dependencies:** A0
**Estimated effort:** Small

---

### Task A2: Blackman-Harris table and Grain fields

**Outcome:** `WindowLuts` carries a 6th internal BH table (`kShapeBH`, not in the user Choice) with a dedicated `readBH(phase)`; `Grain` carries the pulsaret fields.

**Files:** `dsp/WindowLuts.h` (table, `readBH`, header doc; `read()` clamp to `kNumShapes − 1` left untouched; IN-05 contract comment updated), `dsp/Grain.h` (`pulsaret`, `sineContent`, `contentPhase` double, `contentInc`, `glideMul`, `bhMix`, `gUser`, `gBh` — POD, so `moveToFadeSlot` copies them).

**Verification:**
- [x] Unit check in the render harness: BH table mean = 0.35875 ± 1e-4, mean-square = 0.25796 ± 1e-4.
- [x] The 5 existing tables are byte-identical (hash before/after).
- [x] Cloud null bit-exact.

**Dependencies:** A1
**Estimated effort:** Small

---

### Task A3: Pulse clock, Sine pulsarets, duty clamp, BH crossfade, level law

**Outcome:** Pulsar mode with Sine content sounds: keyboard f_p, Formant moves the formant, alias-free up to the ceiling.

**Implementation (per RESEARCH.md §Implementation Approach):**
- `renderNextBlock` branches per block on `params.grainMode`. The Cloud scheduler and Cloud overlap-add are untouched; pulsar grains are handled by `if (g.pulsaret) { renderPulsaret(g, …); continue; }` ahead of the Cloud expressions.
- Clock in member state: double `φp`, `φm`, `uint64 k`. At note-on, `φp = 1 − ε`, `φm = 0`, `k = 0`. Onset test happens before render in the same sample; `δ = φp / (fInst/fs)`.
- f_p = `getMidiNoteInHertz(note) · 2^(grainPitch/12)`, capped at the ceiling.
- `d = clamp(1/formant, f_p/ceil_eff, 1)` with `ceil_eff = 0.10·fs / (2^(max(0,glisson)/12) · 2^(pitchSpray/12))`. L from the nominal f_p.
- `spawnPulsaret(δ)`: window phase `δ·phaseInc`, content phase `δ·contentInc`; latch `bhMix = smoothstep((1.25 − L_ms)/0.5)`, `gUser = √(μ₂_Hann/μ₂_user)` from `windowLuts.meanSquare`, `gBh = 1.206`; taper via `taperEndFor(L)`; tilt skipped.
- `renderPulsaret`: `env = (1−a)·gUser·w_user(φ) + a·gBh·readBH(φ)`, content `std::sin(2π·φc)`.
- Formant smoothing: per-voice per-sample linear ramp.
- Pulsar normGain = window-power term only (no duty lift); Cloud's `normGain` block runs only in Cloud mode, unchanged. The 20 ms `outputGain` ramp absorbs the switch.
- Mode switch never clears the pool; in-flight grains of the other kind window out.

**Verification:**
- [x] A3 at Formant 1×: FFT fundamental 220 Hz ± 0.1 cent at 44.1 / 48 / 96 kHz.
- [x] Formant 2× → 8×: spectral centroid rises monotonically, fundamental unchanged.
- [x] C8 + Formant 32× at 44.1 kHz: folded energy ≤ −60 dB; with the ceiling temporarily set to 0.45·fs the same probe FAILS (stimulus control).
- [x] Duty sweep through 0.75–1.25 ms: no level step — static per-L flatness 0.094 dB (≤ 0.5), no-gain control 3.34 dB (fails). Sine content needs sine-weighted gains (BH 1.469), see SUMMARY.
- [x] Cloud null bit-exact.

**Dependencies:** A2
**Estimated effort:** Large

---

### Task A4: Sample-content pulsarets

**Outcome:** `pulsaretSource = Sample` reads one C3 period of the source per pulsaret, transposed to fit L.

**Implementation:** rate `r = f_form / 130.81 Hz`; read start = playhead + Position Spray via the existing `playheadAt()` and spray code (Position, Scan, Freeze work); `r > 1` → `BandlimitedReader` with mip/sinc latched from the over-filtered `r·γ`, `γ = 0.5 / max(0.05, 0.5 − 4/L_samp)`, while readPos advances by `r`; `r ≤ 1` → Lagrange. Read position offset `r·δ` at spawn. Sine mode never touches `sourcePtr`/`sourceMip`.

**Verification:**
- [x] Sample content at C8 + 32× (fire, voice, piano sources): alias ≤ −60 dB.
- [x] Sine ↔ Sample switch mid-note: click probe passes vs the same-motion control.
- [x] Sine mode with no source loaded still sounds; source buffer reads = 0 (NaN-poisoned source stays finite; Sample over it goes non-finite — harness P2).
- [x] Cloud null bit-exact.

**Dependencies:** A3
**Estimated effort:** Medium

---

### Task A5: Mask, Skip, Rate FM, Glisson

**Outcome:** all §3.6 extras work, and each is bit-exact with its code absent at its neutral setting.

**Implementation:**
- Mask (mute 1 of N): `k mod N == N − 1` → no spawn; Off evaluates no modulo.
- Skip: `skipThisGrain()` on `kSlotSkip` with tick = k; Skip 0 draws nothing.
- FM: `fInst = f_p·(1 + D·sin(2πφm))`, `φm += ratio·f_p/fs` (double, wrapped); D = 0 takes a branch that skips the FM arithmetic. FM Depth smoothed per sample.
- Glisson: content rate × `m = 2^(glisson/(12·L_samp))` per sample; Sample mode latches mip/sinc from the glide's maximum rate; 0 takes the no-multiply branch. Ceiling already includes it (A3).
- RNG: existing slots 0 (position), 1 (pitch), 2 (pan), 4 (skip) with tick = k; no new slots; Cloud draw order untouched.

**Verification:**
- [x] Mask 3: spectral line at f_p/3; Mask Off + Skip 0 renders bit-identical to a code-absent build.
- [x] FM depth 0 bit-identical (vs a code-absent build); ratio 0.5 / 1.5 show the f_p/2 line (−20.8 / −30.9 dB, at 110.0000 Hz); ratio 1 / 2 inert vs no-FM (−106.9 / −105.4 dB, bound ≤ −100). Default FM Ratio 0.5. Wave B tooltip: whole-number ratios are inert by construction (onsets stay 1/f_p apart).
- [x] Glisson 0 bit-identical; +12 st on a 5 Hz pulse train shows the rising glide (half-cycle zero-crossing 0.4302 L vs 0.4293 theory).
- [x] Alias sweep: C8 × 32× × FM 100 % × Glisson +24 × Pitch Spray 12, both content types: ≤ −60 dB.
- [x] Cloud null bit-exact.

**Dependencies:** A4
**Estimated effort:** Medium

---

### Task A6: Viz decimation

**Outcome:** pulsaret GrainEvents publish at most once per ~10 ms per voice; Cloud spawns publish exactly as now.

**Implementation:** per-voice sample countdown, independent of the clock and RNG; Cloud path unchanged.

**Verification:**
- [x] 8 voices at C8, 512-sample blocks: events per block ≤ 8 × ceil(512/480) — never near `kMaxEvents = 256`; no drops counted.
- [x] Pulsar render with viz frame attached vs detached: bit-identical audio.
- [x] Cloud null bit-exact.

**Dependencies:** A3
**Estimated effort:** Small

---

### Task A7: HARD GATE — Cloud null vs 1.9.0 + Pulsar block-size null

**Outcome:** proof that Cloud is untouched and Pulsar is block-size independent. **Wave B does not start until this passes.**

**Verification:**
- [x] Every existing `tests/blocksize-null` config, Cloud mode, vs the A0 reference: bit-exact.
- [x] `null_v180.py`-style state nulls (pre-2.0.0 session state, no new keys) vs the reference: bit-exact.
- [x] Negative control: a 1-ULP-perturbed Cloud build FAILS the null.
- [x] `tests/blocksize-null` extended with Pulsar configs (Sine, Sample, FM on, Mask 3, Skip 30 %): blocks 32 / 441 / 7 vs 512 on fresh instances ≤ −120 dBFS.
- [x] `tests/render-harness`: 0 allocations in processBlock in Pulsar mode (all features on); CPU at 8 voices, C8, Sample content recorded vs Cloud.
- [x] Existing gates still pass: alias-gate, skip-tilt-gate, state-persistence, preset-library-gate.

**Dependencies:** A5, A6
**Estimated effort:** Medium

---

## Wave B — UI / i18n / lessons (after A7)

### Task B1: Mode switch and Pulsar group in the WebView

**Outcome:** a Mode select in the Grain group and a new `group-pulsar` section (Formant, Content, Mask, FM Depth, FM Ratio, Glisson). In Pulsar, Density, Grain Size, Scatter, Vel→Density and Grain Tilt get `.is-inactive` (dimmed, not hidden, so pinned widths hold); in Cloud the pulsar group is dimmed.

**Files:** `index.html`, `js/app.js` (bindings, readouts `×` / `st` / `%` via `getScaledValue()`, mode-driven dimming that also re-applies on preset load and state restore), `css/styles.css`.

**Verification:**
- [x] Every new control binds (7 relays declared before the WebView, attachments after; 22 sliders + 5 combos + 2 toggles). The host↔UI automation round-trip in a DAW still needs the hands-on pass; no host was driven in this session.
- [x] Dimming is driven only by the grainMode combo state's value and properties events. Those fire on every parameter change (click, automation, lesson, user preset, state restore), so there is no one-shot push. Verified headless for the click and the Pulsar state; the DAW preset and reopen check belongs to the hands-on pass.
- [x] Viz: cloud dots clamp radius and pitch and skip non-finite events. The window inset ignores Tilt in Pulsar.
- [x] Screenshots reviewed at 900×760 in Playwright WebKit and Chromium. The rebuilt Standalone window could not be captured because `screencapture -l` refused it.

**Dependencies:** A7
**Estimated effort:** Medium

---

### Task B2: EN/FR/zh labels, tooltips, width pins

**Outcome:** all 7 params, 2 + 2 + 8 choice labels and tooltips in three languages; widths pinned at the widest language.

**Steps:** glossary entries first (Formant, Pulsaret, Mask, Glisson in FR/zh; memory: `pattern_parallel_localizers_need_glossary_first`), then `i18n.js`, then pins. Tooltips document: tilt inert in Pulsar, the 0.10·fs formant ceiling, Density/Size ignored in Pulsar.

**Verification:**
- [x] `check-i18n`, the fr/zh lints (plugin and repo-wide) and `check-ui-labels` PASS.
- [x] Nowrap fit at FR and zh. Tour chips carry `white-space: nowrap`; the worst FR slack is 2.98 px. Lesson captions: worst FR 819.4 of 846 px. The Mode caption is floored at 31 px and its select is a fixed 92 px.
- [x] `tests/i18n-states.json` gains a Pulsar-mode state and both Pulsar lesson-caption states.

**Dependencies:** B1
**Estimated effort:** Medium

---

### Task B3: Two Pulsar lessons

**Outcome:** "Pulsar Vowel" (Sine, Formant ~5×, Mask Off, FM 0) and "Subharmonic Mask" (Sine, Formant ~3×, Mask 3) in `applyFactoryPreset()`, with lesson buttons 9 and 10 and captions in EN/FR/zh.

**Verification:**
- [x] Each lesson resets to defaults first. The existing loop also covers the 7 new params, so grainMode returns to Cloud. Output Level stays user-owned.
- [x] The header lesson row (5 × 2 grid, 560 px) fits at FR and zh at the editor's fixed 900 × 760 size; the editor is not resizable.
- [x] Lesson-count references updated in live source comments. CHANGELOG and STATUS history are left for C2.

**Dependencies:** B2
**Estimated effort:** Small

---

## Wave C — Gates and docs

### Task C1: `tests/pulsar-gate/gate.py`

**Outcome:** one pedalboard gate on the **installed** VST3 (via uv) that re-proves every Pulsar acceptance item: f_p 0.1 cent (A3 at 44.1/48/96 kHz), formant moves while f_p stays, alias ≤ −60 dB sweep (with the 0.45·fs stimulus control), BH band no-step + sidelobe improvement vs Hann at 0.3 ms, integer-vs-fractional spur reduction at a 5 kHz-class formant (96 kHz), Mask f_p/N line, FM sidebands at ratio 1/2, glisson slope, Sine without source, mid-note mode-switch click vs the same-motion control.

**Verification:**
- [x] Gate PASS; each check prints its measured value and threshold.
- [x] Each check has a negative control that fails when its feature is broken (13 checks; mode switch harness-side, P6).

**Dependencies:** A7 (can run in parallel with Wave B)
**Estimated effort:** Medium

---

### Task C2: Build, install, validate, docs

**Outcome:** shippable build and up-to-date docs.

**Steps:** `./scripts/build-and-install.sh O-simpleGrain` (cache clear + dual-variant sweep); pluginval strictness 10; `auval`; `tests/state-persistence` with new params at non-defaults; update `parameter-spec.md`, `BRIEF.md` (Pulsar mode), `.planning/STATUS.md` notes; draft the CHANGELOG `## [2.0.0]` entry (version bump itself is applied post-verify).

**Verification:**
- [x] Build clean, no new warnings.
- [x] pluginval 10 and auval PASS.
- [x] State persistence round-trips all 7 new params (case 8).
- [x] SUMMARY.md written with gate numbers (Wave C section).

**Dependencies:** B3, C1
**Estimated effort:** Small

---

## Dependency Graph

```
A0 → A1 → A2 → A3 ─┬→ A4 → A5 ─┬→ A7 ═(hard gate)═┬→ B1 → B2 → B3 ─┐
                   └→ A6 ──────┘                  └→ C1 ────────────┴→ C2
```

**Parallelizable:** A6 alongside A4/A5; C1 alongside Wave B.

---

## Execution Order

| Order | Task | Dependencies | Parallelizable With |
|-------|------|--------------|---------------------|
| 1 | A0 Pre-flight | — | — |
| 2 | A1 Params | A0 | — |
| 3 | A2 BH table + Grain fields | A1 | — |
| 4 | A3 Clock + Sine + clamp + BH | A2 | — |
| 5 | A4 Sample content | A3 | A6 |
| 5 | A6 Viz decimation | A3 | A4, A5 |
| 6 | A5 Mask/Skip/FM/Glisson | A4 | A6 |
| 7 | **A7 Hard gate** | A5, A6 | — |
| 8 | B1 UI | A7 | C1 |
| 8 | C1 Pulsar gate | A7 | B1–B3 |
| 9 | B2 i18n | B1 | C1 |
| 10 | B3 Lessons | B2 | C1 |
| 11 | C2 Build/validate/docs | B3, C1 | — |

---

## Risk Notes

1. **Cloud regression** (top user frustration)
   - Impact: 1-ULP drift in Cloud output breaks every existing session's sound.
   - Mitigation: branch-around only; Cloud null checked in every Wave A task; A7 hard gate with a perturbed-build negative control.
2. **Aliasing at the top of the keyboard**
   - Impact: audible fold-back with glisson, pitch spray, FM overlap or sample bandwidth.
   - Mitigation: ceiling from max content rate (A3), over-filtered reader (A4), full sweep with stimulus control (A5, C1).
3. **Block-size dependence**
   - Impact: Pulsar output varies with host buffer.
   - Mitigation: double clock, per-sample updates, onset before render; A7 null at 32/441/7.
4. **Level jump on mode switch / BH band**
   - Impact: click or loudness step.
   - Mitigation: shared pool + 20 ms outputGain ramp; per-window power gains; click probes vs same-motion controls (A3, A4, C1). The steady-state Cloud↔Pulsar level difference is reported in SUMMARY.md, not equalised.
5. **UI width / i18n**
   - Impact: FR/zh overflow in the new group and the 10-button lesson row.
   - Mitigation: glossary first, nowrap scrollWidth fit, pins at the widest language (B2, B3).
6. **Viz flood**
   - Mitigation: A6 countdown, audio-independent.

---

## Domain Agent Instructions

**Execute Agent:** general-purpose (mixed domain), running Wave A with dsp-agent rules and Wave B with gui-agent rules.

**Domain-specific rules to follow:**
- No allocation, lock or I/O anywhere in `renderNextBlock` / `processBlock`.
- Never edit a Cloud-path expression, RNG tick or the Cloud `normGain` block — branch around them.
- Clock and FM phase in double, updated per sample; onset test before render in the same sample.
- New params appended with version hint 2; no existing ID, range, default or order changes.
- WebView: relays declared before attachments; readouts via `getScaledValue()`; re-push mode state on preset load.
- Path-scoped commits only (`git commit -- plugins/O-simpleGrain …`); re-check branch and staging before each commit. No tags.
- Build via `./scripts/build-and-install.sh O-simpleGrain`; long renders/builds in the background (600 s watchdog).

**Files the agent should read first:**
- `.planning/improvements/audio-rate-pulsar-grain-mode/RESEARCH.md` — formulas, ceiling measurements, control table
- `Source/GrainVoice.h` — scheduler (`samplesUntilNextGrain`), `spawnGrain` (:406), render loop, `skipThisGrain` (:528), `moveToFadeSlot` (:574)
- `Source/dsp/WindowLuts.h`, `Source/dsp/Grain.h`, `Source/dsp/GrainSource.h`
- `Source/PluginProcessor.cpp` — layout (:60–200), normGain (~:1131), `applyFactoryPreset` (:1233)
- `modules/synthesis/grain-engine/cpp/BandlimitedReader.h`
- `research/granular-synthesis-2026-update.md` §3.1, §3.6
- `tests/blocksize-null/gate.py`, `tests/preset-library-gate/null_v180.py`, `tests/render-harness/main.cpp`

---

## Success Criteria

From CONTEXT.md (with research corrections):

1. [ ] Cloud mode + pre-2.0.0 sessions render bit-exact vs 1.9.0 (A7).
2. [ ] Pulsar fundamental = keyboard f_p within 0.1 cent; formant moves independently (A3, C1).
3. [ ] No folded component above −60 dB under any combination of note, Formant, FM depth, Glisson, Pitch Spray, either content (ceiling 0.10·fs) (A5, C1).
4. [ ] BH auto-switch over 0.75–1.25 ms with no level step; sidelobe improvement vs Hann at 0.3 ms (A3, C1).
5. [ ] Pulsar block-size null ≤ −120 dBFS at 32/441/7 vs 512 (A7).
6. [ ] Mask N → f_p/N line; Skip per pulse; FM sidebands; Glisson glide (A5, C1).
7. [ ] 0 allocations in processBlock in Pulsar mode; CPU measured at 8 voices (A7).
8. [ ] UI mode switch + pulsar controls + 2 lessons in EN/FR/zh; check-i18n, fr/zh lint, check-ui-labels pass (B1–B3).
9. [ ] Build clean; pluginval 10 and auval pass; state persistence passes with new params (C2).

---

*Generated by improve-milestone plan phase*
