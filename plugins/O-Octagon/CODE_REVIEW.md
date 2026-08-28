---
plugin: O-Octagon
version_reviewed: 1.8.0
reviewed: 2026-08-27
span: v1.3.3 -> v1.8.0 (commits 12e5e1ae..2ba236a1 — alignment delay, decorrelator, hover help, monitor fold-down, motion engine)
depth: deep
method: >
  Five parallel subsystem reviewers (DSP/audio thread, processor state/params/presets, editor-WebView
  bridge, front-end JS/HTML/CSS, test-gate validity) produced 34 raw findings and 95 refuted
  candidates against committed HEAD. Every Warning was then re-read from disk by an independent
  agent instructed to REFUTE it; two were confirmed by MUTATION (harness rebuilt against a wrong
  MonitorFold; SmoothedValue NaN recovery probed against JUCE 8.0.14) and one by hash recomputation
  of the AU parameter order. Two Warnings were downgraded to Info, one was confirmed by the
  orchestrator directly. All seven machine gates were RUN, not read: render 73/0, geometry 57/0,
  ui_frontend 43/43, ui_layout 31/31, monitor-fold 15/0, DBAP fixture 102 OK, check-i18n pass.
  The v1.7.0 digest anchor (0xb8c5a2d0c7518204) was independently reproduced from a scratch build
  of 2e03020e, and both CHANGELOG negative controls (NC1 dirty-check, NC2 accumulator) were re-run.
findings:
  critical: 0
  warning: 4
  info: 30
  total: 34
raw_findings_before_verification: 34
adversarially_verified: 6
refuted_in_verification: 0
downgraded_in_verification: 2
status: issues_found
previous_review: .planning/CODE_REVIEW-v1.3.0.md (IN-01..IN-34 of that review remain open; not re-filed here)
scope_note: >
  Reviewed at committed HEAD 2ba236a1 (v1.8.0). A concurrent session began an uncommitted Stage G
  label-localisation edit of index.html / app.js / i18n.js / venue.js at 15:08 during the review.
  Those edits are OUT OF SCOPE; note that the dirty tree currently fails ui_frontend_check section 6
  and check-i18n [15] while the HEAD export passes both — that belongs to the in-flight work.
---

# O-Octagon v1.8.0: Code Review Report

**Reviewed:** 2026-08-27 · **Depth:** deep · **Span:** v1.3.3 → v1.8.0 · **Files reviewed:** 31

Propose-only review — **no source files were changed**. Resolve with `/improve-review O-Octagon`
(Warnings) and `/improve-review-info O-Octagon` (the Info tail).

## Summary

The v1.4–v1.8 span is in good shape and the review can say so with measured evidence, not
narration. Every gate the CHANGELOG cites ran green from real output; the v1.7.0 bit-identity anchor
that the motion engine rests on was reproduced from an independent build of the pre-motion commit;
both negative controls the CHANGELOG describes actually fail the probes it says they fail. The audio
thread gained a decorrelator, eight alignment delay lines, a headphone fold and a motion generator
without picking up a single allocation, lock, string or unbounded loop. The WR-01 single-publish
discipline from the v1.3.2 review survived four feature releases untouched. The native-function
surface is 27 ↔ 27 with the count living in the gate rather than a comment. The
**Handled correctly** section lists 60-odd things this span gets right.

**No Critical findings. Four Warnings, none of which is a crash or a wrong-audio-in-the-common-path
defect — but two of them are the kind that are cheap now and expensive after the first public
release:**

- **WR-01 — the Sync table is 4× slower than its own labels, and two menu entries are duplicates.**
  `kSyncMultipliers` is in cycles-per-beat with `1/4 = 0.25`, which the header and the hover help
  gloss as "one cycle per bar". But the same menu offers "1 Bar" = 0.0625 — four bars — and "4 Bars"
  = 16 bars; the shipped *Slow Orbit* preset says "4 Bars" and orbits once every 32 s at 120 BPM.
  Triplets use 4/3 instead of 3/2, so "1/16D" ≡ "1/8T" and "1/8D" ≡ "1/4T" bit-identically. The
  table was byte-copied from O-Orbit, which has the same defect with no gloss at all. This is a
  user-facing musical-semantics bug in a feature whose whole point is host-locked reproducibility;
  fixing it moves the meaning of a saved `motionSync` value, so it must land **before** release.
- **WR-02 — all 11 new parameters carry version hint 1, so the AU parameter list re-sorted and Logic
  automation lanes from ≤v1.4 retarget** (`outputGain` → `motionHeight`, `hullAtten` → `decorr`,
  `airAmount` → `outputGain`, `rolloff` → `hullAtten`). Mechanism confirmed against the JUCE 8.0.14
  AU wrapper and the hash order recomputed independently. Downgraded from the reviewer's severity
  only because nothing has shipped — which is exactly why it is a ship gate: set `decorr` to hint 2
  and `motion*` to hint 3 now and the baseline is monotone forever; ship with all-1 and the
  accidental hash order becomes the contract.
- **WR-03 — the elevation strip's live marker is drawn at the anchor's depth.** The DSP shapes the
  source at `anchor.y + offset.y` and evaluates ear height there; the strip adds only `offset.z` at
  the anchor's `srcY`. On any raked venue the "Source" readout is wrong by up to ~0.5 m per cycle,
  "Ear" never moves, and the section marker never sweeps front-to-back — the one thing a section
  view exists to show.
- **WR-04 — five of the fifteen monitor-fold checks measure a local Woodworth lambda, not
  `MonitorFold`.** Proven by mutation: θ-only, 0.5×, 2×, a 45° clamp and a naive 90° clamp all pass
  15/15; only a sign flip fails. The CHANGELOG's "31 smp vs 31.5 model max" is printed, not
  asserted. This is the repo's own *"a probe that passes both ways is decoration"* pattern,
  certifying shipped headphone audio.

### What verification changed

- The **NaN host-clock funnel** (reviewer: "permanent silence latch") was downgraded to Info. Probing
  `juce::SmoothedValue` shows it flushes a NaN 240 samples after the next finite target, motion-off
  re-solves at the finite anchor, and no real host emits non-finite tempo/PPQ. The funnel gap is
  real and the fix is three lines — but it is hygiene, not a defect.
- The **DF/DH/DI/DJ/DD "read liveOffset not audio"** finding was downgraded: the log shows exactly
  what the reviewer measured (NC1 leaves them green), but DH/DI/DJ/DD correctly certify the
  published-series claims they actually make, and the CHANGELOG already attributes NC1 to DE and DG
  alone. Only DF's `motionSolves > 0` liveness gate is over-claimed.
- The version-hint tables were **reproduced exactly** once the Studio-One top-bit mask (JUCE default)
  was applied — the verifier's first attempt without it disagreed, which is worth remembering the
  next time anyone recomputes an AU order by hand.

---

## CRITICAL

None.

---

## WARNING

### WR-01 — Sync divisions run 4× slower than their labels; triplet entries use 4/3 instead of 3/2, making two menu pairs bit-identical

**File:** `plugins/O-Octagon/Source/DSP/MotionClock.h:76-96` (table), `:128-136` (`cyclesAt`); `plugins/O-Octagon/Source/Data/PresetPolicy.h:179-182` (*Slow Orbit*); `plugins/O-Orbit/Source/DSP/MotionEngine.h:58-74` (same table)

`cyclesAt` computes `heldCycles = beats · mult` with `beats = ppq + Δsamples/sr · bpm/60` — Hz is exactly `bpm/60 · mult`, no hidden factor. The table is annotated "cycles per BEAT … '1/4' is one cycle per 4 beats — one bar at 4/4", and the hover help repeats that convention (`i18n.js:227/229`, EN+FR). Under that convention every note-value entry is one quarter of its musical meaning (a quarter-note LFO cycles once per beat, i.e. `mult = 1.0`, not 0.25) — which could be defended as a documented choice, except that the same menu carries "1 Bar" = 0.0625 = one cycle per **16 beats** = four bars, and "4 Bars" = 1/64 = sixteen bars. The "N Bars" labels contradict the plugin's own convention by exactly 4× inside one menu.

Separately, the triplet math is wrong: a triplet is 2/3 the duration of its parent (1.5× the rate), a dotted note 1.5× the duration. The table uses 4/3 for triplets, so `1/16D = 2/3` and `1/8T = 2/3` are the same number, as are `1/8D = 1/3` and `1/4T = 1/3`. Two menu choices produce identical motion.

**Scenario:** Logic session at 120 BPM 4/4. User picks "1 Bar" expecting one orbit per bar (2 s); gets one orbit per 8 s. Loads *Slow Orbit* ("4 Bars"): one orbit per 32 s. Picks "1/8T" to get a triplet feel; gets exactly the dotted-sixteenth.

**Evidence:**
```
MotionClock.h
 76: /// Sync choices, in MENU ORDER: ... in CYCLES PER BEAT (Hz = bpm/60 · mult). "1/4" is one cycle per 4 beats — one bar at 4/4.
 82:    4.0 / 3.0,    // 1/16T
 83:    1.0,          // 1/16
 84:    2.0 / 3.0,    // 1/16D
 85:    2.0 / 3.0,    // 1/8T
 90:    0.25,         // 1/4
 93:    0.0625,       // 1 Bar
 95:    0.015625      // 4 Bars
128:    const double mult = kSyncMultipliers[syncIndex];
135:    const double beats = clock->ppq + (static_cast<double> (samplesSinceBlockStart) / sr) * (bpm / 60.0);
136:    st.heldCycles = beats * mult;
```

**Fix:** Rewrite the table as cycles-per-beat from the note value at 4/4 (Free 0; 1/16T 6; 1/16 4; 1/16D 8/3; 1/8T 3; 1/8 2; 1/8D 4/3; 1/4T 3/2; 1/4 1; 1/4D 2/3; 1/2 1/2; 1/2D 1/3; 1 Bar 1/4; 2 Bars 1/8; 4 Bars 1/16). Correct the header comment and both hover-help strings ("1/4 is one cycle per beat; Bars assume 4/4 — the plugin reads no time signature"). *Slow Orbit* index 14 then means what it says. **Probe DD must be re-fixtured**: its `ppqStart = 1.0` relies on index 8 being 0.25 (verifier finding); MP7/MP8 and DF–DM are table-agnostic. Mirror the fix into O-Orbit `MotionEngine.h:58-74`, or drop the "O-Orbit's fourteen divisions" wording so the two plugins are not claimed identical. Add a unit probe asserting `kSyncMultipliers` has no duplicate entries and that `1 Bar == 4 × (1/4)`.

**Adversarially verified — CONFIRMED (WARNING).** Verifier confirmed no hidden ×4 in `cyclesAt`, confirmed the convention IS documented at `MotionClock.h:77` and `i18n.js:227/229` (correction to the reviewer, who called it undocumented), and confirmed that the "N Bars" entries contradict it within the same menu. Duplicate pairs enumerated: (1/16D, 1/8T) and (1/8D, 1/4T). O-Orbit shares the table without the gloss, so O-Orbit is straightforwardly 4× slow versus its labels. Probe-impact analysis: only DD breaks under the fix.

### WR-02 — All 11 new parameters carry version hint 1, so the AU parameter list re-sorts and Logic automation lanes recorded on ≤v1.4 retarget to the wrong parameter

**File:** `plugins/O-Octagon/Source/PluginProcessor.cpp:58`, `:72`, `:81` (every `ParameterID { id, 1 }`); `:122` (`decorr`), `:176-191` (motion group)

JUCE's AU wrapper builds the parameter list sorted by masked ID hash, then stable-sorted by version hint, and Logic/GarageBand key automation by **index** in that list — JUCE's own `AudioProcessorParameter(int versionHint)` doc says recall "will work as expected in Logic and GarageBand" only if later parameters carry a higher hint. Every O-Octagon parameter is hint 1, so the eleven parameters added in v1.5.0/v1.8.0 interleave with the originals by hash accident.

**Recomputed order** (31-multiplier `String::hashCode`, bit 31 masked because `JUCE_USE_STUDIO_ONE_COMPATIBLE_PARAMETERS` defaults on, unsigned sort, stable by hint):
```
≤v1.4 : w1..w8, blur, srcX, srcY, srcZ, width, outputGain, hullAtten, airAmount, rolloff
v1.8.0: w1..w8, blur, srcX, srcY, srcZ, width, motionHeight, decorr, outputGain, hullAtten,
        motionAngle, motionPhase, motionRatio, airAmount, rolloff, motionPath, motionRate, ...
  outputGain 13 → 15   (index 13 is now motionHeight)
  hullAtten  14 → 16   (index 14 is now decorr)
  airAmount  15 → 20   (index 15 is now outputGain)
  rolloff    16 → 21   (index 16 is now hullAtten)
```
Indices 0–12 (the eight weights, position, width, blur — the parameters a spatializer actually automates) never move. VST3 identifies by hash and never reads the hint — unaffected.

**Scenario:** Any Logic session saved against a ≤v1.4 build with an `outputGain` lane drives `motionHeight` after upgrade. Today that means only the developer's own v1.1.0 in-space test sessions; after the first public release it means every user's.

**Evidence:**
```
PluginProcessor.cpp:58   return std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { id, 1 },
PluginProcessor.cpp:72   return std::make_unique<juce::AudioParameterBool> (juce::ParameterID { id, 1 },
PluginProcessor.cpp:81   return std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { id, 1 },
juce_audio_plugin_client_AU_1.mm:2347  vec.push_back ({ generateAUParameterID (*param), param->getVersionHint() });
juce_audio_plugin_client_AU_1.mm:2349  std::sort        (... a.identifier  < b.identifier ...);
juce_audio_plugin_client_AU_1.mm:2350  std::stable_sort (... a.versionHint < b.versionHint ...);
```
The comment at `PluginProcessor.cpp:57` ("mandatory in JUCE 8") and `GainStage.h:71` ("APPENDED so every pre-existing index is untouched") both reason about the enum/APVTS index, which is irrelevant to AU ordering.

**Fix:** Give `makeFloat`/`makeBool`/`makeChoice` a `versionHint` argument: 1 for the 17 originals, **2 for `decorr`**, **3 for the ten `motion*`**. This restores the ≤v1.4 order exactly and gives a monotone baseline for every future addition. Cost: any v1.5–v1.8 **dev** Logic session with lanes on the nine re-sorted parameters retargets once more — say so in the CHANGELOG. Add a harness probe asserting `getVersionHint() > 1` for every id not in the original 17, evaluated on the live parameter objects (not a mirrored count). Do this **before** the first public release; after it the all-1 order is the contract.

**Adversarially verified — CONFIRMED-DOWNGRADED.** Verifier confirmed the mechanism from the JUCE 8.0.14 wrapper and the JUCE parameter doc, confirmed the reviewer's tables exactly (after applying the Studio-One mask, which the verifier's first pass omitted), confirmed no `JUCE_FORCE_USE_LEGACY_PARAM_IDS` in the build, and confirmed VST3 ignores the hint. Downgraded from the reviewer's "would be CRITICAL if released" to LOW **because nothing has shipped**; recorded here as WARNING because the user's stated next step is publishing, and this is a one-line decision that becomes irreversible at that moment.

### WR-03 — The elevation strip draws the live source at the ANCHOR's depth: "Source" and "Ear" readouts disagree with the DSP on any raked venue, and the section marker never sweeps

**File:** `plugins/O-Octagon/Source/ui/public/js/elevation.js:291-294`, `plugins/O-Octagon/Source/ui/public/js/app.js:1260-1263`; DSP reference `plugins/O-Octagon/Source/DSP/GainStage.cpp:444-451`, `plugins/O-Octagon/Source/DSP/SourceShaper.cpp:86-89`

The audio thread shapes the source at `{anchor.x + offset.x, anchor.y + offset.y}` and `heightAt(y)` evaluates `plane::earHeight(rakeFront, rakeRear, …, y)` at that **live** y. The strip's `drawMarker()` reads only `srcY` (the anchor), computes `earM` there, and adds `motionZ` — the y half of the offset never reaches it (`onMotion` forwards `offset[2]` only).

**Scenario:** Default rig (15.6 × 19.5 m), rake front 0.00 / rear 1.20 m, motion on, Orbit, Size 12 m. The audible source's depth swings ±6 m, so the DSP's ear height under it swings ±0.48 m. The strip's "Source" number is wrong by up to 0.48 m at the front/rear of every cycle, "Ear" reads a constant, and the marker's x never moves while the source is sweeping front-to-back. The CHANGELOG ("The elevation strip shows the live height") and the `elevation` tip ("Source is its absolute height") both fail for every path except a 0°/180° Pendulum/Sweep. Measured on the HEAD export with the stub: `elev-src` moved to "−2.15 m" through a cycle while `elev-ear` stayed "0.00 m".

**Evidence:**
```
elevation.js:291  const depthM = normToMetres(0, srcY.state.getNormalisedValue(), geometry).y;
elevation.js:293  const earM = earHeight(depthM, geometry);
elevation.js:294  const srcM = earM + srcZ.state.getScaledValue() + motionZ;
app.js:1262       if (elevation !== null) elevation.setMotionZ(running ? Number(offset?.[2]) || 0 : 0);
GainStage.cpp:449 subPoints = shaper::shapeAt (snapshot, Vec2 { anchor.x + offset.x, anchor.y + offset.y }, zEff, widthMetres);
SourceShaper.cpp:88  return plane::earHeight (v.rakeFront, v.rakeRear, v.bbMinY, v.bbMaxY, y) + srcZ;
```

**Fix:** Replace `setMotionZ(z)` with `setMotion(offset, running)` storing `motionY` and `motionZ`; in `drawMarker()` use `depthM = anchorDepthM + motionY` for the marker x, `earM` and `srcM`. Optionally draw the anchor as a hollow ghost tick at its own depth, mirroring the plan. Extend `ui_layout_check` §32 (or a new section) to assert `elev-ear` changes across a cycle on a raked fixture — the current gate cannot see this because the stub's rake is flat.

**Verified by the orchestrator — CONFIRMED (WARNING).** Both sides re-read from `git show HEAD:` and disk: `elevation.js` has exactly one motion input (`motionZ`, line 139/352-355) and `app.js:1262` forwards only `offset[2]`; `GainStage.cpp:449` and `SourceShaper.cpp:88` use the live y. The "Handled correctly" claim in the CHANGELOG is false on the y axis.

### WR-04 — monitor-fold: five of fifteen checks assert a local Woodworth transcription, never `MonitorFold`; the class's ITD magnitude is not asserted anywhere

**File:** `plugins/O-Octagon/tests/monitor-fold/main.cpp:59-84` (lambda block), `:150` (the only class-driven ITD check)

The block the README credits with "the model's own maximum, asserted against `kMaxItdSeconds`" and "the piecewise seam" defines its own lambda `w` and asserts properties of *that lambda*. `MonitorFold` is not constructed in it. The only check that reaches the class's ITD is check 4, which asserts direction only (`tR < tL`). The "31 smp vs 31.5 model max" figure in the v1.7.0 CHANGELOG is printed, not asserted; `kMaxItdSeconds` is the class's own constant (a mirror, not an independent expectation).

**Evidence (mutation, verifier):** harness rebuilt in scratch against five wrong `MonitorFold.cpp` ITD models — θ-only (no `+ sin θ`), 0.5× Woodworth, 2× Woodworth, a 45° clamp, a naive 90° clamp (rear ITD 0.66 ms) — **all pass 15/15**. Only a sign flip fails (check 4).
```cpp
59:        auto w = [] (float th)
63:            const float f = a <= 1.5707963f ? (a + std::sin (a)) : ((3.14159265f - a) + std::sin (a));
64:            return sign * (oo::MonitorFold::kHeadRadiusM / oo::plane::kSpeedOfSoundMps) * f;
75:        check ("woodworth-max-at-90",     std::fabs (atRight - oo::MonitorFold::kMaxItdSeconds) < 1.0e-9f,
150:        check ("itd-right-source-reaches-right-ear-first", tR >= 0 && tL >= 0 && tR < tL,
```

**Fix:** Drive the class with an impulse from speakers at known azimuths (the octagon gives 0°, ±45°, ±90°, 180°) and assert the measured `tL − tR` in samples against `round(w(θ) · sr) ± 1` where `w` is the published Woodworth formula written in the test (`r/c · (θ + sin θ)`), **not** `kMaxItdSeconds`. Add a rear-lane case (catches the naive clamp) and a seam case at 90°. Rename the five lambda checks (`woodworth-formula-*`) so the ledger stops counting them as class coverage. Also `geometry-gated-on-generation` at `:276` is `check(..., true, ...)` — see IN-22.

**Adversarially verified — CONFIRMED (WARNING) by mutation.** Verifier read the README and v1.7.0 CHANGELOG claims ("guards against a naive clamp", "31 smp vs 31.5") and found neither backed by an assertion; checks 4/5 assert a FAR-ear band and mirror symmetry (correct and kept), but no check bounds ITD magnitude.

---

## INFO

Opt-in via `/improve-review-info O-Octagon`. Grouped by subsystem.

### DSP / audio thread

### IN-01 — Host clock is the one audio-thread input with no `isfinite` funnel (transient NaN, not a latch) **Resolved in v1.10.1**
**File:** `Source/PluginProcessor.cpp:767-782`, `Source/DSP/MotionClock.h:129-136`
`getBpm()`/`getPpqPosition()` are copied raw; `bpm > 0.0` rejects NaN but admits `+inf` (→ `0 · inf = NaN` at `samplesSinceBlockStart == 0`), and a NaN/±inf PPQ flows `cyclesAt → evaluate → updateControl → dbap::solve → setTargetValue(NaN)`. Verifier probed JUCE 8.0.14 `SmoothedValue`: a NaN target is flushed 240 samples after the next finite target, and motion-off re-solves at the finite anchor, so the worst case is NaN/garbage output for the duration of the bad clock + 5 ms, not permanent silence. No real host emits non-finite tempo/PPQ. Filed because the codebase's own doctrine (P17/P29) is "single funnel, structurally impossible to bypass" for every audio-thread input, and this is a third input that bypasses it.
**Fix:** `if (bpm && std::isfinite(*bpm) && *bpm > 0.0)` / `ppqValid = ppq && std::isfinite(*ppq)`; optionally `std::isfinite(clock->bpm)` in `MotionClock.h:129` and a unit probe "non-finite clock → finite cycles".

### IN-02 — `kFoldTrim`'s "structurally impossible to exceed the lane peak" is false once `kGainCeil = 4` is in the product **Resolved in v1.10.1**
**File:** `Source/DSP/MonitorFold.h:99-113, 147-148`, `MonitorFold.cpp:218-221`
Cauchy–Schwarz covers Σv² = 1, but per-speaker gain is `gL · dist · kFoldTrim` with `dist = jlimit(0.05, 4.0, refR/radius)`. A speaker at ≤ ¼ of the mean radius carrying most of the energy folds at up to 4 · 0.354 = 1.41 → +3 dB over the lane. No clip; doc claim is wrong.
**Fix:** Doc: "bounded by kGainCeil · (1/√8) · lane peak".

### IN-03 — Free ↔ Sync switch is a phase discontinuity the re-base does not cover
**File:** `Source/DSP/MotionClock.h:99-111, 131-146`
The PPQ branch never touches `lastRate`/`phaseBase`, so returning to Free re-bases against the last Free rate, not the synced phase — the puck teleports up to `size` metres. Ramps absorb the click.
**Fix (optional):** on entering the free branch after a PPQ evaluation, `phaseBase = heldCycles − rate · t`.

### IN-04 — Alignment lines keep stale audio across a SAFE stretch; monitor crossfade stalls there too
**File:** `Source/DSP/GainStage.cpp:853-868, 989-998`, `MonitorFold.h:183`
F3 flips REAL → SAFE → REAL without `prepareToPlay()`: `delayEngaged` stays true, ramps advance but lines are not clocked; the first REAL chunk pops up to 50 ms of pre-SAFE audio. `monitorFold.mix` similarly completes only on return. Rare (device channel-count change mid-session).
**Fix:** reset the eight lines on the SAFE→REAL edge (`wasMapped` bool), or document beside the F3 note.

### IN-05 — First engage of an alignment delay > 5 ms reads past the line's history for the ramp length **Closed in v1.10.1 sweep** (stale premise — ramp reads a freshly zeroed line; already documented)
**File:** `Source/DSP/GainStage.cpp:664-672, 687-691`
false→true edge resets the line, then `delaySamples` ramps 0 → d over 240 samples; for d > 240 the read position exceeds history, so that lane reads zeros (a dropout) until the ramp lands. Setup-time only; documented as "the one cold start".

### IN-06 — Headphone fold carries the hall's alignment delays but not the propagation delays they compensate **Resolved in v1.10.1**
**File:** `Source/DSP/MonitorFold.cpp:218, 227-237`, `GainStage.cpp:925-938`
The fold models 1/r gain and ITD but no r/c distance delay, while the lanes it reads already carry per-speaker alignment delays whose purpose is to cancel r/c in the hall — so headphones hear the NEGATIVE of the hall's delay pattern. Stated design ("what you hear is the actual rig feed"); recorded as a known trade-off.

### Processor state / parameters / presets

### IN-07 — Version stamps: NOTES.md still says 1.7.0 and its history stops at v1.5.0; no `backups/O-Octagon/v1.8.0/` snapshot exists **Resolved in v1.10.1**
**File:** `NOTES.md:6`, `NOTES.md:39-85`; `backups/O-Octagon/` (v1.0.0 … v1.7.0 present, each stamped with its own version)
CMakeLists 1.8.0 / CHANGELOG v1.8.0 / PLUGINS.md 1.8.0-dev / installed AU+VST3 CFBundleShortVersionString 1.8.0 all agree; NOTES.md is the outlier, and the per-version backup convention (post-change snapshot named by resulting version) was not followed for v1.8.0.
**Fix:** bump the NOTES header, add v1.6.0/v1.7.0/v1.8.0 history bullets, write the v1.8.0 backup.

### IN-08 — Choice-string arrays are transcribed over `motion::kNumPaths` / `kNumSyncChoices` with no assert **Resolved in v1.10.1**
**File:** `Source/PluginProcessor.cpp:179-184`; `Source/DSP/MotionPath.h:62-71`; `Source/DSP/MotionClock.h:78`
Counts match today (6 / 15). A future table edit without the StringArray edit silently shifts what a lane labelled "1 Bar" does (`cyclesAt` degrades to Free on OOB, so no crash). No probe compares `getAllValueStrings().size()` to the constants.
**Fix:** constructor `jassert` + harness probe for both choices.

### IN-09 — Stored alignment delay is unrailed on the model; the table can show a value the audio thread does not honour **Closed in v1.10.1 sweep** (stale premise — deliberate and documented; v1.4.0 CHANGELOG makes no load-rail claim)
**File:** `Source/Data/VenueModel.cpp:421-432`, `Source/PluginEditor.cpp:412`, `Source/PluginProcessor.cpp:525-526`
A hand-edited `.venue` with `delayMs="-5"` or `"1e6"` passes `readFloat` (finite), displays as typed, and runs 0 / 50 ms. `VenueModel.h:132` declares this deliberate (matches trim); the v1.4.0 CHANGELOG reads as though the range is validated at load.
**Fix (optional):** rail in the `getVenueGeometry` payload, or state "stored and displayed unrailed" in the CHANGELOG.

### IN-10 — Stale comments contradicted by v1.4–v1.8 **Resolved in v1.10.1**
`PluginProcessor.cpp:38-40` "All 17 ranges are linear by design" (motionRate is skewed, `:173-174`); `VenueModel.cpp:315-317` "version 1 is the only version that exists" (schema 2 since v1.4.0); `PluginProcessor.h:34-46` still describes "Phase 2.2 … 17 smoothed gains … no WebView editor"; `PluginEditor.h:27,38` / `PluginEditor.cpp:25,304` "18 parameters" / "EXACTLY THIRTEEN" / "THREE native functions" (28 / 27 — IN-18 class from the previous review, drifted further).

### Editor / WebView bridge

### IN-11 — Live-puck triple is three independent relaxed atomics — a torn x/y read is possible (cosmetic)
**File:** `Source/DSP/GainStage.cpp:365-367`, `GainStage.h:242-247`
A 64-sample boundary can land between the x and y loads of the 30 Hz poll; at Orbit 4 Hz / 24 m that is up to ~0.4 m disagreement for one frame. Audio never reads these.
**Fix (optional):** seqlock generation around the three stores, or pack x,y into one `atomic<uint64_t>`.

### IN-12 — `getMotionTrace` reads raw APVTS floats with no finite guard, unlike `getFieldGrid` and `snapshotParameters` **Resolved in v1.10.1**
**File:** `Source/PluginEditor.cpp:1159-1168` vs `:1355-1369`
A host-written NaN motion param yields 128 NaN points; `var(NaN)` is not valid JSON on the page side, the completion is lost and the previous trace stays. Audio is protected by `snapshotParameters()`.
**Fix:** reuse the `readParam` finite-or-default shape.

### IN-13 — Bypassed plugin freezes the live puck off-anchor while the page still reports "running"
**File:** `Source/PluginProcessor.cpp:670-685`, `Source/PluginEditor.cpp:1135-1136`
`processBlockBypassed` never reaches `GainStage::process`, so `liveOffset` holds; `getMeters` reports `motionOn` from the parameter. Self-heals on un-bypass.
**Fix (optional):** zero `liveOffset` in `processBlockBypassed`, or derive the page's running flag from "a boundary ran this poll window".

### Front-end

### IN-14 — `getMotionTrace` is re-fetched once per parameter echo with no debounce
**File:** `Source/ui/public/js/app.js:476-500`
Measured on the HEAD export: 20 key steps → 20 fetches; 30 synthetic echoes 16 ms apart → 30 fetches. The `setTimeout(0)` coalesces per task turn and each echo is its own task. A host automation ramp on any of the six shape ids does one native round trip (128 evaluations + 384-number JSON + 128-point `d` rebuild) per tick. Contrast the field recompute's dirty-flag-on-2 Hz-tick discipline (`app.js:800-808`).
**Fix:** mark dirty on echo and spend it on the 30 Hz meters tick, or a trailing 50–100 ms timeout; keep the seq guard.

### IN-15 — A dropped `getMotionTrace` completion leaves a stale trace with no retry; Path→Drift then keeps drawing the previous path's loop instead of the tail **Resolved in v1.10.1**
**File:** `Source/ui/public/js/app.js:474-475, 487-491`; `roomplan.js:521, 552, 858`
Hidden-editor drop exactly on the Orbit→Drift echo: `trace.cyclic` stays true, the tail never appears, and nothing but another shape echo repairs it. Hidden-editor only.
**Fix:** re-issue `refreshTrace()` from `setMotionOn(true)` or on the first `getMeters` tick after a visibility return.

### IN-16 — i18n scope at HEAD: only hover help is localised; every JS-written status string and all page chrome is English (Stage G inventory) **Partially resolved in v1.10.1** (index compare only; localisation itself landed in v1.9.0)
**File:** `app.js:372, 408, 457, 557, 606-610, 629-631, 666-668, 685-689`; `venue.js:593, 634, 683, 703-705`; `i18n.js:94-100`
Documented scope (the `lang-select` tip says so) and the in-flight uncommitted Stage G edit is addressing it. Two constraints for that work, both verified at HEAD: (a) choice display names arrive from the C++ `StringArray` and the page binds by **index** — a localised display name must never be used as the lookup key (`app.js:457` compares the string `"Drift"` today; that comparison must move to index 5); (b) `data-label` holders must not be written by any shared state updater (the repo's textContent-erases-labels trap). Note the dirty tree currently fails `ui_frontend_check §6` ("Delay" `<th>`) and `check-i18n [15]` (dead keys `oo.direct`, `oo.roles`).

### IN-17 — `preset-list` tip still says "17 parameters" — the count is 28 **Dropped in v1.10.1 sweep** (already fixed in v1.9.0)
**File:** `Source/ui/public/js/i18n.js:452, 454` (EN + FR)
**Fix:** "the 28 parameters", or drop the number.

### IN-18 — No committed O-Octagon gate drives the French tooltips or measures French-length overflow of the v1.8.0 anchors **Dropped in v1.10.1 sweep** (already covered by `scripts/check-ui-labels.js` + `tests/i18n-states.json`)
**File:** `tests/ui_layout_check.js`, `tests/ui_frontend_check.js` (`grep __setLanguage` empty)
Measured here on the HEAD export, both languages, all 12 motion anchors: every tip inside 1100 × 720 with the 8 px margin, French tallest 135.9 px, `motionSize` flips above→below in French, three right-column anchors clamp to `right = 1092`. Nothing to fix on the page — the claim is not gated. The MONITOR banner likewise has no rendered-geometry assertion.
**Fix:** a `ui_layout_check` section that loops `TIP_BINDINGS` in `en` and `fr` via `window.__setLanguage` with the Motion panel and Seed cell revealed as `nudge()` does; a §9-style banner section driven by the stub's `monitorArmed`/`monitorSuppressed`.

### IN-19 — Degree readouts render as "360 °" with a space before the sign **Resolved in v1.10.1**
**File:** `Source/ui/public/js/app.js:171, 173, 248`
**Fix:** `tight: true` on the two entries, or special-case `°` in `formatValue`.

### IN-20 — Drift tail: fixed 48 polls (~1.6 s) regardless of rate, and wiped on every shape echo
**File:** `Source/ui/public/js/roomplan.js:282, 837, 861`
*Wander* (0.05 Hz) leaves a tail covering ~1/12 of a cycle; a Size automation ramp clears it per echo. Bounded memory (verified 48 points).
**Fix:** length in seconds (~8 s = 240 points), clear only when `cyclic` changes.

### IN-21 — The trace is invisible while motion is off, although the Motion tab's tip promises "The map shows the trace before you hear it" **Resolved in v1.10.1**
**File:** `Source/ui/public/js/roomplan.js:521`; `i18n.js:206`
**Fix:** draw the trace at lower opacity while off, or reword the tip.

### IN-22 — Language and tooltip preferences are pulled once at page init and never re-pulled on a session restore with the editor open
**File:** `Source/ui/public/js/app.js:875-879, 1117-1119`
Deliberate v1.6.0 design; pre-existing for the tooltip toggle. Self-corrects on reopen.
**Fix (optional):** a `uiPrefsGen` on `getStatus` (the venueGen idiom).

### Test gates

### IN-23 — DF's liveness gate is `motionSolves > 0`; DD/DH/DI/DJ certify the published offset series, which the CHANGELOG conflates with the audio
**File:** `tests/render-harness/main.cpp:6816` (DF), `6766` (DE), `6692` (DD), `6722` (DI), `6901` (DH), `6942` (DJ); `Source/DSP/GainStage.cpp:365-369`
`liveOffset` is stored before the dirty check, so under NC1 (delete `&& ! motionOn`) the audio freezes at the first solve yet DD/DF/DH/DI/DJ stay green — measured (`ncb-run.log`); only DE and DG fail, which is exactly what the CHANGELOG attributes to NC1. Verifier: DH/DI/DJ/DD correctly measure the published-series/clock claims they make; only DF over-claims ("synced-transport block-size invariance" gated on one solve).
**Fix:** DF (and DD's `moving` clause) assert DE's exact boundary count. Real uncovered gap (verifier's): no probe compares a motion-ON render against a static `srcX/srcY + offset` equivalent in **X/Y** — DD's `bothZ` does it for Z only — so an X/Y consumption bug at `GainStage.cpp:447-449` would leave every D-probe green. Add one.

### IN-24 — DK loads a preset the harness writes with today's code; no captured pre-1.8.0 artefact, and no SESSION-shaped case
**File:** `tests/render-harness/main.cpp:6466-6539`
The "v1.7.0-shaped" JSON is manufactured from `oo::params::id(i)` for `i < 18` — a mirror that cannot detect an id rename or reorder. Separately, JUCE `replaceState` leaves a parameter with no child tree at its CURRENT value, so `setStateInformation()` with a pre-1.8.0 blob into a live instance whose `motionOn` is ON leaves motion ON (reachable via host undo/A-B that reuses the instance). Not claimed by the CHANGELOG (it says presets).
**Fix:** commit a real v1.7.0 preset JSON as a fixture; add a session-restore probe and decide (document or reset).

### IN-25 — MP1 "figure8-closes" is a tautology
**File:** `tests/unit/main.cpp:3601-3602`; `Source/DSP/MotionPath.h` (`frac(cycles)` first)
`frac(1.0) == 0.0` exactly, so `evaluate(m, 0)` and `evaluate(m, 1)` are bit-identical for every path; a `sin(1.5 · t)` typo in the figure-8 arm passes.
**Fix:** compare `evaluate(m, 1 − ε)` to `evaluate(m, 0)` within `2πR · ε`; same at `0.5 ± ε`.

### IN-26 — MP7 compares `cyclesAt` with `cyclesAt`; the synced half is arithmetic identity
**File:** `tests/unit/main.cpp:3670-3699`
The Free half is a genuine stepped-vs-direct invariance (NC2 shows the class can fail); the synced half feeds the function the expression it computes. No-PPQ branch not reached.
**Fix:** assert a hand-derived constant; add `ppqValid = false, playing = true`.

### IN-27 — `HarnessPlayHead` never exercises no-PPQ; no loop-wrap or tempo-change case
**File:** `tests/render-harness/main.cpp:366-399` (`valid` never set false); `Source/DSP/MotionClock.h:133-153`
The header's documented "no PPQ → free-run" and "PPQ withdrawn after rolling → hold" rules have no probe; the CHANGELOG's "accepted" tempo-change behaviour is unmeasured.
**Fix:** a `valid=false` render (moving, block-size invariant) and a synced render whose playhead wraps at bar 4.

### IN-28 — `geometry-gated-on-generation` is `check(..., true, ...)`
**File:** `tests/monitor-fold/main.cpp:262-277`
Proves only that two calls did not crash; the README lists it as guarding "re-derive on a venue publish and NOT on a source move".
**Fix:** fold an impulse before/after `updateGeometry(moved)` (same generation) and assert bit-identical; after `updateGeometry(published)` assert different.

### IN-29 — ui_layout §32 "puck ON the trace" is tautological w.r.t. the stub, and its `moved` clause is wall-clock driven **Partially resolved in v1.10.1** (section-count literal only; stub sharing and wall-clock `moved` deferred)
**File:** `tests/ui-stub/juce-stub.js:674-690`; `tests/ui_layout_check.js:1360-1383`
The stub's `motionOffset()` and `motionTrace()` share `fixtureEvaluate`, so the 1.5 px test proves one projection, never C++ agreement (which §41 covers statically). `moved` samples `Date.now()` over 8 ticks — can false-FAIL on a slow runner. Also the summary literal says 31 sections while 32 headings exist.
**Fix:** drive the stub's cycles from a poll counter.

### IN-30 — CHANGELOG claims with no probe (gap list)
- v1.7.0 guard 2 (arm not persisted): no arm → `getStateInformation` → fresh instance → disarmed probe.
- v1.7.0 guard 4 (`processBlockBypassed` disarms): nothing calls `processBlockBypassed`.
- v1.7.0 refusals (SAFE, unresolved pair) and ping↔monitor mutual exclusion: stub-mirrored only.
- v1.6.0 `uiLanguage` round trip and clamp-to-English on a corrupt value.
- v1.4.0 `applySuggestedDelays` funnel (native fn → `applyVenueEdit`, `sane()` before `jlimit`, NaN delay).
- v1.8.0 seed change mid-render and `motionOn` on→off→on continuity.
- monitor-fold `build-standalone.sh` is not wired into CMake and only runs from its own directory (relative include).

---

## Refuted in verification

None outright — every Warning candidate survived with its mechanism intact. Two were downgraded (IN-01, IN-23) because the consequence claimed did not follow; the reasons are recorded inline.

## Refuted candidates (reviewer stage, selected)

The five reviewers rejected 95 candidates. The ones most likely to be re-raised:

- **Accumulator hiding in `cyclesAt`** — `freeRunCycles` is `rate · t + phaseBase`; the PPQ branch is `ppq · mult`. No per-call integration. NC2 re-run confirms DE/DL are live.
- **Block-size dependence in any new stage** — every ramp is a per-sample JUCE `SmoothedValue`; geometry/engage evaluated only at 64-sample boundaries; `Decorrelator` derives its integer delay from the per-sample smoothed depth. The clamped-float-ramp-rails trap does not apply.
- **Alignment delay line sizing / push-pop order / per-speaker indexing** — `ceil(50 ms · sr) + 2`; `pushSample` then `popSample(d)` is the canonical pair (d = 0 returns the just-pushed sample); delay indexed by speaker, meters read through `speakerToBuffer`. Consistent.
- **Decorrelator bypass not bit-exact** — `decorrEngaged` false → `xL = fL` with no arithmetic.
- **SAFE mode reads a position** — `fold()` only inside the `mapped` arm; SAFE arm reads no position; `monitorOn` requires `mapped`. CHANGELOG claim holds (DM measures it).
- **A third publish path re-opens WR-01 (v1.3.x)** — every new entry point funnels into `applyVenueEditChecked → applyVenueEdit`, which publishes once; `setMonitorArmed` publishes nothing.
- **Factory presets ignore `motionRate` skew** — `PresetPolicy.h:188-191` converts through the live `getParameterRange(id).convertTo0to1`.
- **Migration hook needed** — no range moved in v1.4–v1.8.
- **Bool round-trips as a STRING on the ValueTree** — `tooltipsEnabled`/`uiLanguage` are root XML attributes with typed getters; nothing new on the tree.
- **Native-fn bridge gap** — 27 registered ↔ 27 JS call sites, set-diff empty both ways; gate §3 pins 27.
- **Missing relay/attachment for a new param** — both loops walk `kCount` (28) with the same Bool/Choice predicates; every relay reaches `withOptionsFrom`.
- **Trace generator drifts from the audio thread** — same header-only `oo::motion::evaluate`, same `MotionParams` construction order.
- **MSVC C3493 / SafePointer init-capture** — constants at namespace scope or `static` locals; `safeThis` hoisted.
- **French through `juce::String(const char*)`** — only the two-letter code crosses to C++; choice names ASCII.
- **Seed cell wrong on init with Drift restored** — driven by the combo relay's initial echo, measured.
- **motionRate readout ignores skew** — uses `SliderState.getScaledValue()`.
- **Motion panel overflows 1100 × 720** — measured 113 px both tabs, 592/592, label ink 42.3/44 px even under a Georgia fallback; both languages.
- **New `transform-origin` without `transform-box` (CR-01 class)** — none added.
- **DC digest recorded from the v1.8.0 build itself** — refuted by rebuilding `2e03020e` with the DC scenario appended: same digest.
- **DE/DG use a tolerance** — `memcmp` across {64}, {256}, {1024}, ragged {1,7,64,333,4096} with mid-render events, all six paths.

## Handled correctly

- Motion phase is a pure function of (absolute sample, host PPQ); rate-change re-base at an absolute grid boundary; PPQ extrapolated by exact sample distance; `absoluteSampleCounter` zeroed only in `prepare()`, 64 | 2⁶⁴ static-asserted.
- `prepare()` single-init-site discipline for all new state; engage edges (delay, decorr, monitor) reset on the false→true TRANSITION, not per block; `delayActive` stays true while ramping DOWN.
- ms → samples in double, cast once; integer decorrelator delays with a measured justification; both chains converge at depth 0.
- Eight separate mono `DelayLine`s and sixteen `FirstOrderTPTFilter`s (per-instance trap), `setMaximumDelayInSamples` before `prepare`, Nyquist cap at 0.45 · fs.
- Monitor: not a parameter, not persisted, `isNonRealtime()` bypass, `processBlockBypassed` disarm, `mapped` in the conjunction, slots resolved on the message thread and published in the snapshot, refused on an unresolved pair at both ends; far ear asserted as a BAND with mirror symmetry (the "assert a band, not `>`" lesson).
- NaN guards use the LAST output rather than a running `jmax` for air, decorr and fold; `ScopedNoDenormals` covers all new stages.
- Motion off is structurally the v1.7.0 path (a branch, not `+ 0.0f`); `shape()` delegates to `shapeAt()`; `zEff` reaches both the shaper and the z-cue solve.
- Parameter defaults derived from the parameter objects; NaN reads fall back to those defaults; every venue value that reaches the audio thread passes one funnel with `isfinite` + `jlimit`, NaN handled BEFORE `jlimit`.
- Preset scope: `kPreserved + kAuthored == kCount` build-time partition; reset-to-defaults precedes every apply; `loadPreserving` restores the 12 preserved bit-exact on both paths.
- `readVenueFromState(! preparedYet)` keeps exactly one publish per `setStateInformation`; gesture brackets on every processor-side multi-parameter write.
- `.venue` loader rejects malformed roots / < 8 speakers before touching the model; v1 files zero delays first; `schemaVersion = 2` written.
- Typed relays built from the single `oo::params` table; trace and audible path share one generator; live puck in anchor-relative metres, zeroed at every boundary when off, rides the 30 Hz poll.
- All completions advisory; polls self-heal dropped completions (deadline flag, trace sequence number); FileChooser a member, SafePointer hoisted, dead-pointer path returns bare.
- Read-on-echo never write-on-echo for all three relay kinds; combo `<option>`s built from `state.properties.choices` by index; Position | Motion tab pair is pure view state with authored captions; group height identical on both panels.
- `applyI18n` writes only `data-tip`/`data-tip-title`; `tr()` falls back fr → en and an empty body is refused by `showTip`; tooltip measure-then-pin with viewport clamp holds for every new anchor in both languages.
- Alignment-delay column follows the WR-05 repaint-on-reject pattern, rails against the C++-supplied max, converts units in one function pair.
- Every new render probe carries a liveness clause and resets `instr` counters; cross-version anchors (CU v1.4.0, DC v1.7.0) are literals with the capture commit named; DE asserts the EXACT boundary count; §15 parses `createParameterLayout()` from source and diffs the stub's ranges and choices against it; §11 extends the relay declaration-order rule to toggle/combo relays; §30's `reset()` ledger is itemised.
- CV (decorrelator) measured against analytic expectations; CS/CT (alignment) bit-exact shift + silent head + one lane moved, memcmp 512 vs 4096; `HarnessPlayHead` derives PPQ from `samplesElapsed`, never accumulating.
- WebView2 static-linking define and user-data folder present; every UI asset embedded and served with `charset=utf-8`; no new canvas, no new module-level executable statement, `init();` still last.

## Resolved

- **v1.10.1 — commit 0cc48afb (2026-08-27), `/improve-review-info` sweep.** Resolved: IN-01, 02, 06,
  07, 08, 10, 12, 15, 19, 21. Partially: IN-16 (Drift-by-index residual), IN-29 (count literal).
  Dropped as already fixed: IN-17, IN-18. Closed as stale premise: IN-05, IN-09. Still open —
  design calls: IN-03, 04, 11, 13, 14, 20, 22; fail-first test work: IN-23, 24, 25, 26, 27, 28,
  29 (stub/wall-clock parts), 30. Triage detail in CHANGELOG v1.10.1 Notes.
