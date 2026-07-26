---
plugin: O-ReverseDelay
version: 1.7.1
date: 2026-07-25
candidates:
  high: 7
  medium: 7
  low: 6
  total: 20
paths_relative_to: plugins/O-ReverseDelay/
---

# O-ReverseDelay v1.7.1 — Simplification Audit (PROPOSE ONLY)

## Summary

The redundancy did not concentrate where the line counts suggested. `PluginProcessor.cpp`
is 2075 lines, but roughly 1100 of those are comment blocks that carry real derivations
(the coherence argument above `kLoopCountTrimExponent`, the regen ladder, the two-stream
RNG rationale) and are worth every line. The executable body of `processBlock` is one
readable pass with no repeated stanza — the per-release parameter reads at lines 1051–1119
each do a genuinely different clamp, and the 25 cached `std::atomic<float>*` members are
named and greppable in a way a table-driven lookup would destroy. **I am explicitly not
proposing the "collapse the parameter plumbing into a table" refactor**; it would trade
25 self-documenting names for an index and save nothing that matters.

Where the accretion actually landed, in order of payoff:

1. **The factory-preset table** (`PluginProcessor.cpp:132–248`). Eight presets, each
   repeating a byte-identical 15-key no-op tail. Verified programmatically: all eight
   share the exact same tail signature. That is ~70 lines of copy, and — more importantly
   — it is the structure that makes release #8 a nine-site edit where one missed row is a
   silently re-voiced factory preset.
2. **`WindowLut::computeStats()` and `computeTaperStats()`** (`WindowLut.h:640–698` /
   `720–793`) run the same half-window integration and the same pair of 1e-6 canonicalisation
   gates against two different inputs. ~40 lines, constructor-time only, no RT exposure.
3. **The note-division list exists in five places** and only one of them is checked.
4. **Comment drift is the largest single comprehension cost in the plugin, and two
   instances of it state the opposite of a load-bearing invariant.** `PluginEditor.h`'s
   header is factually wrong in five places against the `.cpp` it pairs with, and
   `GrainScheduler.h` still tells the reader there is ONE shared xorshift stream — the
   thing v1.1.0 spent a release proving must not be true.
5. **Two silent test-fixture mirrors.** The v1.7.1 release exists because a paper sum
   mirrored a comment; the check written to prevent a recurrence contains a hand-summed
   `CHROME` literal that nothing renders, and the stub range check next to it asserts
   `delayTime` against literals `50`/`4000` while the `grainSize` check three lines below
   correctly reads the C++ constants.

**The honest total win is roughly 190 lines and four drift traps closed** — of which
~130 lines come from the two duplication candidates HIGH-01 and HIGH-02. That is a
modest line count for a 14.7k-line plugin, and it should be: `CaptureBuffer.h`,
`ReverseGrain.h` and the `processBlock` body are already clean, `styles.css` is
disciplined apart from two identical rule pairs, and `app.js` has exactly the structure
its own TDZ note demands. The value here is concentrated in the drift traps, not the
diff size.

Nothing in this audit touches an APVTS ID, range, skew or default; nothing introduces
allocation, locking or `std::function` on the audio path; and no candidate declares an
audio change. `juce::dsp::IIR::ArrayCoefficients` is already used correctly at
`PluginProcessor.cpp:1379` and `:1386` — no `Coefficients::makeXXX` appears on any RT
path, so there is no correctness note to file there.

---

### [HIGH-01] Factory-preset table repeats a byte-identical 15-key no-op tail eight times

**File:** `Source/PluginProcessor.cpp:132-248`
**Type:** duplication
**Risk:** MEDIUM
**Current:**
```cpp
{ "Reverse Bloom",
  {{"syncMode", 0.0f}, ... {"mix", 40.0f},
   {"jitter", 0.0f}, {"delayScatter", 0.0f},
   {"sizeRandom", 0.0f}, {"gainRandom", 0.0f},
   {"grainTilt", 0.5f}, {"grainShape", 0.0f},
   {"grainCount", 8.0f}, {"tukeyTaper", 0.5f},
   {"freeze", 0.0f}, {"direction", 0.0f}, {"regenMakeup", 0.0f},
   {"sourceMode", 0.0f}, {"duck", 0.0f},
   {"driftRate", 0.30f}, {"driftDepth", 0.0f}}, {} },
// ...the same 15-key tail, verbatim, in all seven remaining presets
```
**Proposed:**
```cpp
// The no-op tail every factory preset shares: each key added since v1.1.0,
// pinned to the value at which the engine does exactly what v1.0.0 did.
// ONE definition, so a release that adds a 26th parameter edits one place
// instead of eight — and cannot half-edit it.
// Named constants, not literals: grainCount's 8 IS kLegacyOverlapMax and
// driftRate's 0.30 IS kDriftRateCentreHz.
static const std::vector<std::pair<juce::String, float>> kShippedNoOpTail {
    {"jitter", 0.0f}, {"delayScatter", 0.0f},
    {"sizeRandom", 0.0f}, {"gainRandom", 0.0f},
    {"grainTilt", 0.5f}, {"grainShape", 0.0f},
    {"grainCount", kLegacyOverlapMax}, {"tukeyTaper", WindowLut::kTukeyTaperDefault},
    {"freeze", 0.0f}, {"direction", 0.0f}, {"regenMakeup", 0.0f},
    {"sourceMode", 0.0f}, {"duck", 0.0f},
    {"driftRate", kDriftRateCentreHz}, {"driftDepth", 0.0f},
};

for (auto& preset : factoryPresets)
    preset.parameters.insert (preset.parameters.end(),
                              kShippedNoOpTail.begin(), kShippedNoOpTail.end());
// ...then the existing convertTo0to1 loop, unchanged.
```
**Rationale:** Not "shorter" — *one-editable*. Five consecutive releases have appended
keys here, and the comment block above the table (lines 61–114) spends 50 lines
explaining which no-op is 0 and which is not, precisely because getting one row wrong is
silent. The table's own structure guarantees that a sixth release has eight chances to
get it wrong and no check that all eight agree; verified today they do, but nothing
enforces it. Extracting the tail makes "every preset carries the shipped no-op" a
structural fact rather than a fact about eight hand-maintained rows. It also removes two
literal-vs-constant drifts the test suite already flags elsewhere: `ui_frontend_check.js:900`
asserts `grainCount`'s *parameter default* is written as `kLegacyOverlapMax` and not a
literal `8`, while this table writes a literal `8.0f` in eight places; `driftRate`'s
`0.30f` has the same relationship to `kDriftRateCentreHz`. The per-preset comments about
intent stay — they move to the one place the values now live.

**Test impact:** Render-harness **probe N** loads all eight factory presets and
round-trips them (`Near-Infinite` for 30 s), so it is the standing guard. The tail must
be appended *before* the `convertTo0to1` loop at line 255 or the values are stored
denormalised — the failure would be a 10–30× recall error on the skewed params
(`pattern_factory_preset_normalized_ignores_skew`). New check needed: assert the merged
parameter map for each preset has exactly 25 keys, and diff a preset JSON written before
and after the change byte-for-byte. Because `CMakeLists.txt VERSION` gates the
`.factory-version` sentinel, verification must delete
`~/Library/O-ReverseDelay/Presets/Factory` first or the edit is a silent no-op.
**Lines saved:** 70

---

### [HIGH-02] `computeStats()` and `computeTaperStats()` duplicate the half-window integration and both canonicalisation gates

**File:** `Source/dsp/WindowLut.h:640-698` and `Source/dsp/WindowLut.h:720-793`
**Type:** duplication
**Risk:** MEDIUM
**Current:** Both functions contain the same body against different inputs — the two
half-range accumulate loops, the `jmax(1, …)` divisors, the six moment assignments, and
the two relative-1e-6 symmetry gates:
```cpp
// computeTaperStats, lines 655-692        // computeStats, lines 732-783
double lo = 0.0, hi = 0.0, aLo = 0.0, aHi = 0.0;
for (int i = 0;    i < half; ++i) { const double v = ...; lo += v*v; aLo += v; }
for (int i = half; i < size; ++i) { const double v = ...; hi += v*v; aHi += v; }
const double nLo = (double) juce::jmax (1, half);
const double nHi = (double) juce::jmax (1, size - half);
st.meanSqLo = (float)(lo/nLo);  st.meanSqHi = (float)(hi/nHi);
st.meanSq   = 0.5f * (st.meanSqLo + st.meanSqHi);
st.meanLo   = (float)(aLo/nLo); st.meanHi   = (float)(aHi/nHi);
st.mean     = 0.5f * (st.meanLo + st.meanHi);
if (std::abs (st.meanSqLo - st.meanSqHi) <= 1.0e-6f * juce::jmax (...))
    st.meanSqLo = st.meanSqHi = st.meanSq;
if (std::abs (st.meanLo - st.meanHi) <= 1.0e-6f * juce::jmax (...))
    st.meanLo = st.meanHi = st.mean;
```
**Proposed:**
```cpp
/** The two duty cycles of one window, integrated from the samples the render
    loop will actually produce, with a symmetric window's two halves collapsed.
    Shared by computeStats() (fixed shapes) and computeTaperStats() (Tukey's
    per-alpha grid) so the two grids cannot integrate differently.
    shapeNorm is NOT set here — the two callers anchor it differently. */
static ShapeStats integrateHalves (const float* w, int size) noexcept
{
    /* ...verbatim body, in the same operation order... */
}

// computeTaperStats:
auto& st = taperStats[(size_t) k];
st = integrateHalves (w.data(), size);
st.shapeNorm = (st.meanSq > 0.0f) ? std::sqrt (stats[(size_t) hann].meanSq / st.meanSq) : 1.0f;

// computeStats:
stats[(size_t) s] = integrateHalves (tables[(size_t) s].data(), size);
// ...then the existing separate shapeNorm loop against `ref`, unchanged.
```
**Rationale:** These are two grids that must integrate *identically* or the "α = 0.50
entry reproduces v1.3.0's constants bitwise" guarantee at `WindowLut.h:164-166` quietly
stops holding. Today that guarantee rests on two hand-kept copies of the same arithmetic
agreeing — the exact structure the file's own comment at line 158 argues against ("a
closed form that drifts from the window is the exact silent error the normalisation
exists to prevent"). The two long comments justifying the 1e-6 gates (lines 680–685 and
759–776) also say the same thing twice; one home for the arithmetic gives them one home
too. Constructor-time only — nothing here is on the audio path, so there is no RT
consideration.
**Test impact:** Must be a *verbatim* move preserving operation order, or the float
result moves. Covered by **probe Z1b** (`getTiltNorm` exactly `1.0f` at every tilt for
all symmetric shapes, `main.cpp:2393-2404`), **probe AI** (the closed forms
`meanSq = 1 − 0.625α`, `mean = 1 − 0.5α` cross-checked against the grid), **probe AH**
(the α = 0.5 entry equals the non-α overload exactly, `main.cpp:3400-3405`) and
**probe Z4** (decay rate across all five shapes). New check needed: a full 138-probe
harness run plus a byte-diff of a defaults render before and after.
**Lines saved:** 40

---

### [HIGH-03] The 13 note divisions are encoded in five places and validated in none

**File:** `Source/PluginProcessor.cpp:531-535` (the `StringArray`), `Source/PluginProcessor.cpp:1169-1177` (`kDivisionBeats` + a literal `12`), `tests/ui-stub/juce-stub.js:103-109`, `tests/render-harness/main.cpp:1522`
**Type:** duplication
**Risk:** MEDIUM
**Current:**
```cpp
// PluginProcessor.cpp:531 — the display names
juce::StringArray { "1/16", "1/16D", "1/16T", "1/8", "1/8D", "1/8T",
                    "1/4",  "1/4D",  "1/4T",  "1/2", "1/2D", "1/2T", "1/1" }, 6));

// PluginProcessor.cpp:1169 — the beats, 640 lines away, tied only by a comment
static constexpr double kDivisionBeats[13] = {
    0.25, 0.375, 1.0/6.0, 0.5, 0.75, 1.0/3.0,
    1.0,  1.5,   2.0/3.0, 2.0, 3.0,  4.0/3.0, 4.0 };
const int div = juce::jlimit(0, 12, static_cast<int>(pNoteDivision->load()));
```
**Proposed:**
```cpp
// PluginProcessor.h — ONE table. The name and the beat value for a division are
// the same fact; separating them is what lets a reordering land in one list only.
struct NoteDivision { const char* name; double beats; };
static constexpr NoteDivision kNoteDivisions[] = {
    { "1/16", 0.25 }, { "1/16D", 0.375 }, { "1/16T", 1.0 / 6.0 },
    { "1/8",  0.5  }, { "1/8D",  0.75  }, { "1/8T",  1.0 / 3.0 },
    { "1/4",  1.0  }, { "1/4D",  1.5   }, { "1/4T",  2.0 / 3.0 },
    { "1/2",  2.0  }, { "1/2D",  3.0   }, { "1/2T",  4.0 / 3.0 },
    { "1/1",  4.0  },
};
static constexpr int kNumNoteDivisions = (int) std::size (kNoteDivisions);

// createParameterLayout(): build the StringArray by iterating kNoteDivisions.
// processBlock(): const int div = juce::jlimit (0, kNumNoteDivisions - 1, ...);
//                 kNoteDivisions[div].beats * 60000.0 / jmax(1.0, *bpm)
```
**Rationale:** The comment at line 1166 (`noteDivision -> beats, contract order:` followed
by the names retyped a third time) is doing the work a data structure should do. The
literal `12` at line 1177 is a fourth encoding of the same length — the same class of
defect as the v1.0.0 A1 bug the header at `PluginProcessor.h:147-152` was written to
prevent ("the v1.0.0 defect was a literal 2000.0 in the sync clamp drifting from the
parameter's own max, so there is exactly one definition now"). That discipline was applied
to `delayTime` and never to `noteDivision`. Inserting a division would today require four
correct edits, three of them silent on failure: a mismatched pair means the UI names a
division the engine is not playing, which is exactly the v1.0.0 symptom.
**Test impact:** **Probe I** (sync spacing at 120 BPM, 1/4 → 0.5 s first echo) and
**probe P1** (1/1 at 60 BPM = 4000 ms, `main.cpp:1688-1723`) are the standing guards for
the beat values; **probe M**'s all-param sweep drives `noteDivision` 0→12 (`main.cpp:1522`)
and would catch a length change. New check needed: `ui_frontend_check.js` should validate
the stub's `noteDivision` array against the C++ table the same way it already validates
`grainShape` at line 1003-1009 — see MED-06. Session round-trip proof required: the
`AudioParameterChoice` string list must come out byte-identical, or a host's automation
display changes.
**Lines saved:** 12

---

### [HIGH-04] `PluginEditor.h`'s header block is factually wrong in five places against the `.cpp` it pairs with

**File:** `Source/PluginEditor.h:1-26` and `Source/PluginEditor.h:57`
**Type:** naming
**Risk:** LOW
**Current:**
```
Stage 3 (GUI): ... Four framed group panels in signal-flow order
(TIME | GRAIN | FEEDBACK | OUTPUT) binding all 10 APVTS parameters ...
Stage 4 (Polish) grows the window to 940 × 484 ...
No visualization, no Timer, no C++→JS polling bridge, no drag-drop ...
The native-function surface is exactly ELEVEN:
  - getParameterDefaults (dblclick-reset)
  - 10 preset fns
...
// 1. RELAYS — 17 sliders + 3 combos + 1 toggle.
```
**Proposed:**
```
Stage 3-4 (GUI): TWELVE framed group panels in three rows binding all 25 APVTS
parameters — row 1 TIME | GRAIN | FEEDBACK | OUTPUT (signal flow, D9), row 2
RANDOM | WINDOW | COUNT | MOTION (character), row 3 SOURCE | DUCK | DRIFT |
COLOUR (space & response, COLOUR reserved). Window is 940 × 768 (v1.7.1).

D10's "no visualization, no Timer, no C++->JS polling bridge" was REVERSED at
v1.3.0 and v1.4.0 — see the PluginEditor.cpp header. There is still no
juce::Timer here: the grain meter is a JS-side PULL on an interval, and the
window-shape curve is pulled on change.

The native-function surface is exactly THIRTEEN:
  - getParameterDefaults (dblclick-reset)
  - getGrainMeter        (v1.3.0, B2)
  - getWindowCurve       (v1.4.0)
  - 10 preset fns        (the contract js/preset-manager.js fetches)
...
// 1. RELAYS — 20 sliders + 4 combos + 1 toggle.
```
**Rationale:** Every one of the five claims is contradicted by the file 40 lines away.
`PluginEditor.cpp:6-9` lists 20 slider relays and 4 combo relays; `:201` says "exactly 13";
`:24-38` explicitly records that D10 was reversed and *why*; `:530` sets 940 × 768. The
count "ELEVEN" is the specific problem: `PluginEditor.cpp:201-206` and `app.js:21-24` both
tell the reader to grep-diff the native-function surface against a stated count, citing
`pattern_webview_native_fn_bridge_gap` — and the header states a count that would make a
correct surface look two-over. A reviewer trusting this header would either delete two
working native functions or conclude the grep-diff already failed. This is not cosmetic;
it is a stated invariant with the wrong number in it.
**Test impact:** `tests/ui_frontend_check.js` already diffs `kSliderIds` / `kComboIds` /
`kToggleIds` against the APVTS in both directions and counts the native-function
registrations, so the *code* is guarded and only the prose is wrong — which is why this
survived. Compile + `auval` is sufficient proof for a comment-only change. New check
worth adding: have `ui_frontend_check.js` assert the digit in the header's
"exactly THIRTEEN" against the count of `withNativeFunction` calls in the `.cpp`, so the
prose is checked the way the lists already are.
**Lines saved:** 0 (net; ~4 lines added, five false statements removed)

---

### [HIGH-05] `GrainScheduler.h` documents ONE shared RNG stream — the two-stream split is a correctness requirement and has been since v1.1.0

**File:** `Source/dsp/GrainScheduler.h:31-35` and `Source/dsp/GrainScheduler.h:160-168`
**Type:** naming
**Risk:** LOW
**Current:**
```
The RNG is injected rather than owned: PluginProcessor holds one xorshift32
stream shared by the pan spread and all four v1.1 randomisations, and a
single stream is what makes the whole engine reproducible from one seed.
...
// LOAD-BEARING: at jitterAmount == 0 this draws NOTHING.
//
// The processor's xorshift is a single shared stream — pan spread, delay
// scatter, size random and gain random all pull from it. An unconditional
// draw here would advance that stream one step per spawn even with jitter off...
```
**Proposed:**
```
The RNG is injected rather than owned, and the caller passes the JITTER stream
specifically. PluginProcessor holds TWO xorshift32 streams, split by WHEN they
are consumed: `jitterRng` is drawn from here, inside the per-sample countdown;
`grainRng` is drawn by the spawn handler, after a whole pass is scheduled.
Sharing one stream would interleave the two differently at different pass
lengths — i.e. differently at 512 than at 4096 samples — and the engine would
stop being block-size invariant. Render-harness probe W2 asserts 512-vs-4096
bit equality and caught exactly this. Both derive from one instanceSeed, so the
whole engine is still reproducible from one seed.
...
// LOAD-BEARING: at jitterAmount == 0 this draws NOTHING.
//
// Not because the stream is shared with the other randomisations — it is not,
// see above — but because the jitter stream's consumption must be a pure
// function of the spawn INDEX. An unconditional draw here would advance it once
// per spawn even with jitter off, so turning jitter on later would land its
// draws at different offsets than a fresh session's. Every v1.1 randomisation
// follows the same rule at its own call site, which is what makes "all four at
// 0" render bit-identically to v1.0.1 (probe T asserts exactly this).
```
**Rationale:** `PluginProcessor.h:685-708` spends 24 lines establishing that the split
into two streams is *a correctness requirement, not tidiness*, and names the probe (W2)
that caught the single-stream version producing different offline bounces than live
monitoring. `GrainScheduler.h` is the file that consumes the second stream, and it is the
only file in the plugin that still asserts there is one. A maintainer optimising here — or
adding a randomisation — would read this file first and reach precisely the conclusion W2
was written to refute. The `nextInterval` note is doubly misleading: its stated *reason*
for the zero-gate ("would shift every subsequent pan value") is a consequence of the
single-stream design that no longer exists, so the correct rule is stated with an argument
that no longer supports it.
**Test impact:** Comment-only; compile + `auval` is sufficient. **Probe W2**
(512-vs-4096 bit equality with all four randomisations on) and **probe T** (all-zero
bit-identity) remain the code-level guards and are untouched.
**Lines saved:** 0 (net; two false statements corrected)

---

### [HIGH-06] `ui_frontend_check.js` asserts the stub's `delayTime` range against literals while the `grainSize` check three lines below reads the C++ constants

**File:** `tests/ui_frontend_check.js:1024-1025`
**Type:** duplication
**Risk:** LOW
**Current:**
```js
check(/delayTime:\s*\{\s*start:\s*50,\s*end:\s*4000/.test(stub),
    'ui-stub delayTime range tracks the v1.0.1 widening (50-4000, not 50-2000)');

// v1.5.0: grainSize's max AND skew centre both moved (500->4000, 158->316).
// Asserted against the C++ CONSTANTS rather than against literals repeated
// here, because a literal in the test drifts exactly as silently as the
// literal in the stub did ...
const gMin = cppNum('kGrainSizeMinMs'); /* ...reads PluginProcessor.h... */
```
**Proposed:**
```js
// Same treatment as grainSize below, and for the same reason: a literal here
// drifts exactly as silently as the literal in the stub. delayTime's endpoints
// AND its skew centre all have named constants — kDelayTimeSkewCentreMs is the
// one v1.0.1 deliberately held at 316 while the max moved, so it is worth
// asserting rather than assuming.
const dMin    = cppNum('kDelayTimeMinMs');
const dMax    = cppNum('kDelayTimeMaxMs');
const dCentre = cppNum('kDelayTimeSkewCentreMs');
const stubDelay = stub.match(
    /delayTime:\s*\{\s*start:\s*([0-9.]+),\s*end:\s*([0-9.]+),\s*skew:\s*skewForCentre\(\s*([0-9.]+),\s*([0-9.]+),\s*([0-9.]+)\)/);
check(dMin !== null && stubDelay !== null
        && parseFloat(stubDelay[1]) === dMin
        && parseFloat(stubDelay[2]) === dMax
        && parseFloat(stubDelay[5]) === dCentre,
    'ui-stub delayTime range/skew match the C++ kDelayTime* constants');
```
**Rationale:** This is `pattern_test_fixture_mirrors_drift_silently` sitting three lines
above the comment that names it. The `grainSize` block exists *because* the stub sat at
50–500 after the range moved and the suite stayed green; the identical exposure on
`delayTime` was left in place. `kDelayTimeMinMs`, `kDelayTimeMaxMs` and
`kDelayTimeSkewCentreMs` all already exist as named constants in `PluginProcessor.h:153-155`
and `cppNum()` is already defined in the same block, so the fix is mechanical. The current
check also never validates the skew centre at all — `delayTime`'s centre is the value
v1.0.1 deliberately held at 316 ms while widening the max, so a stub drift there produces a
browser-rendered readout that disagrees with the plugin across the whole useful range while
the endpoints still match.
**Test impact:** Change is *inside* the test. Verify by running
`node tests/ui_frontend_check.js` (must stay all-pass) and by deliberately editing the stub's
`delayTime.end` to 2000 and confirming the new check fails where the old one also would,
then editing the *skew centre* to 200 and confirming the new check fails where the old one
would **not** — that second case is the coverage this adds.
**Lines saved:** -8 (this candidate adds lines; the win is a closed drift trap)

---

### [HIGH-07] `ui_frontend_check.js`'s `CHROME` is a hand-summed mirror of rendered geometry that nothing renders

**File:** `tests/ui_frontend_check.js:449-460`
**Type:** duplication
**Risk:** MEDIUM
**Current:**
```js
// Chrome is measured, not guessed — these are rendered values from the
// stub page at the shipping viewport, and ui_tooltip_clamp_check.js renders
// the same page, so a drift here shows up there as an overflow.
const CHROME = 6 + 32 + 70.5 + 44 + 23;   // border, padding, header, band, footer
const groupsH = shipH - CHROME;
const slack   = groupsH - rowsTotal;
check(slack >= 18 && slack <= 40, ...);
```
**Proposed:** Move the slack assertion into `tests/ui_tooltip_clamp_check.js`, which
already drives the real page at 940 × 768 in a browser:
```js
// ui_tooltip_clamp_check.js — measure, do not re-derive.
const box = await page.evaluate(() => {
  const g = document.querySelector('.groups');
  const rows = [...document.querySelectorAll('.group-row')];
  const gr = g.getBoundingClientRect();
  return { groupsH: gr.height,
           rowsH: rows.reduce((a, r) => a + r.getBoundingClientRect().height, 0)
                  + (rows.length - 1) * 14 };
});
const slack = box.groupsH - box.rowsH;
check(slack >= 18 && slack <= 40, `.groups slack ${slack.toFixed(1)} px (measured)`);
```
`ui_frontend_check.js` keeps only its static checks (`setSize`, the two `768px`
declarations, the row-height regexes, the ≤ 900 budget) and drops `CHROME` entirely.
**Rationale:** The comment claims these are rendered values, but nothing renders them —
they are five literals summed on paper in a file that never opens a browser. Three of the
five (`6` frame border, `32` frame padding, `44` preset band) are derivable from
`styles.css` and are not derived; the other two (`70.5` header, `23` footer) are rendered
line heights that *cannot* be derived from CSS at all, which is exactly why they must be
measured rather than mirrored. A change to `.header { padding-bottom }` or
`.footer { padding-top }` leaves `CHROME` stale, makes `slack` wrong by that amount, and
the `[18, 40]` gate then passes or fails for the wrong reason with no signal. This is the
same failure the v1.7.1 release was written to correct — "the fixture agreed with the
comment because it mirrored the same sum", quoted in this file's own comment at line 445 —
reproduced one screen lower. The claim that "a drift here shows up there as an overflow" is
also not currently true: `ui_tooltip_clamp_check.js` measures tooltip rects and the
viewport (lines 239, 274, 299) and never touches `.groups`, so no overflow check exists to
catch it.
**Test impact:** Change is inside the test suite; no shipping code moves. Verify by running
both scripts (must stay all-pass) and by temporarily setting `.footer { padding-top: 30px }`
in `styles.css` — the current `CHROME`-based check passes unchanged, the proposed rendered
check must fail. That is the whole point of the candidate and should be recorded.
**Lines saved:** 6

---

### [MED-01] `.meter-label`/`.knob-label` and `.meter-value`/`.knob-value` are byte-identical rule pairs, with a comment claiming they are shared

**File:** `Source/ui/public/css/styles.css:634-649` and `Source/ui/public/css/styles.css:683-698`
**Type:** duplication
**Risk:** LOW
**Current:**
```css
/* line 613: "Borrows .knob-value's type treatment rather than restating it" */
.meter-label { font-size: 9.5px; letter-spacing: 0.9px; text-transform: uppercase;
               color: var(--brown-frame); white-space: nowrap;
               text-shadow: 1px 1px 2px var(--text-emboss); }
.meter-value { font-size: 11px; letter-spacing: 0.4px; color: var(--brown-text);
               white-space: nowrap; font-variant-numeric: tabular-nums; }
/* ...44 lines later, byte-for-byte the same declarations... */
.knob-label  { /* identical to .meter-label */ }
.knob-value  { /* identical to .meter-value */ }
```
**Proposed:**
```css
/* The meter's rows genuinely DO borrow the knob type treatment — one rule each,
   rather than the comment claiming it while a copy sits 44 lines away. */
.knob-label,
.meter-label { font-size: 9.5px; letter-spacing: 0.9px; text-transform: uppercase;
               color: var(--brown-frame); white-space: nowrap;
               text-shadow: 1px 1px 2px var(--text-emboss); }

.knob-value,
.meter-value { font-size: 11px; letter-spacing: 0.4px; color: var(--brown-text);
               white-space: nowrap; font-variant-numeric: tabular-nums; }
```
**Rationale:** Verified byte-identical, all six and all five declarations respectively.
The `.meter-cell` comment at line 611-613 states the rules are borrowed and not restated;
they are restated. Merging makes the comment true and gives the page's caption/value type
scale one home — today a font-size tweak to `.knob-label` silently desynchronises the
COUNT panel's meter from every other caption on the page.
**Risk note (required by the CSS constraint):** this is a *merge*, not a deletion, and it
is justified by a selector-reference check rather than arithmetic. Confirmed: no element in
`index.html` carries both `knob-label` and `meter-label` (or both value classes) — the
meter classes appear only inside `.meter-row` (`index.html:340-345`), the knob classes only
inside `.knob-cell`. Cascade order changes (the merged rule sits at `.meter-*`'s earlier
position or `.knob-*`'s later one); the only rules between them are `.group-label` and
`.group-body`, neither of which targets these selectors, and the `.group-window`-scoped
overrides are higher specificity and unaffected either way.
**Test impact:** `ui_frontend_check.js` parses `styles.css` by regex for geometry only and
does not reference these selectors. Needs a visual smoke test — render the page through
`tests/ui-stub/` at 940 × 768 and confirm the COUNT panel's Active/Overlap rows are
pixel-unchanged.
**Lines saved:** 14

---

### [MED-02] `.group-motion` and `.group-source` segment rules are duplicated verbatim, 20 lines apart, with a comment claiming they are listed together

**File:** `Source/ui/public/css/styles.css:467-479` and `Source/ui/public/css/styles.css:492-499`
**Type:** duplication
**Risk:** LOW
**Current:**
```css
.group-motion .segments { flex: 0 0 100%; flex-direction: row;
                          justify-content: center; gap: 8px; }
.group-motion .segment  { width: 75px; }
/* ...13 lines of comment, including:
   "SOURCE reuses MOTION's segment layout verbatim rather than sharing a class
    with it: ... and the two selectors are listed together so the arithmetic
    (2 x 75 + 8 = 158, the panel's exact content box) has one home." */
.group-source .segments { flex: 0 0 100%; flex-direction: row;
                          justify-content: center; gap: 8px; }
.group-source .segment  { width: 75px; }
```
**Proposed:**
```css
/* MOTION (v1.6.0) and SOURCE (v1.7.0) both lay a full-width horizontal pair over
   a 190 px panel. 2 x 75 + 8 = 158 is exactly that panel's content box
   (190 - 2x14 padding - 2x2 border), so the pair and the knob row below share
   one edge-to-edge width. Scoped to these two panels: .segments and .segment are
   shared with TIME's VERTICAL Free/Sync pair, and an unscoped edit turns that
   one sideways too. */
.group-motion .segments,
.group-source .segments { flex: 0 0 100%; flex-direction: row;
                          justify-content: center; gap: 8px; }

.group-motion .segment,
.group-source .segment  { width: 75px; }
```
**Rationale:** The v1.7.0 comment states the intent ("the two selectors are listed
together so the arithmetic has one home") and the code does not implement it — the
arithmetic is currently explained twice, 20 lines apart, and the declarations are
duplicated. Both selectors have identical specificity (0,2,0) and identical declarations,
so a comma-merge is provably behaviour-identical.
**Risk note:** merge, not deletion. Selector-reference check: `.group-motion .segments`
matches only `index.html:379`, `.group-source .segments` only `index.html:428`; no element
carries both panel classes. No rule between the two blocks targets either selector.
**Test impact:** `ui_frontend_check.js:466-470` greps for `.group-source,` in the *width
contract* list (line 405), not in these rules, so it is unaffected. Needs a visual smoke
test of the MOTION and SOURCE panels at 940 × 768 through `tests/ui-stub/`.
**Lines saved:** 8

---

### [MED-03] Five module-level `let` bindings in `app.js` are written and never read

**File:** `Source/ui/public/js/app.js:208-211`, `Source/ui/public/js/app.js:234`
**Type:** dead-code
**Risk:** MEDIUM
**Current:**
```js
let syncState     = null;      // written at :407, never read
let sourceState   = null;      // written at :487, never read
let freezeState   = null;      // written at :454, never read
let divisionState = null;      // written at :964, never read
let envLastCurve  = null;      // "last curve received, for a DPR redraw with no
                               //  fetch" — written at :657, read only on :658
```
**Proposed:** delete the four state bindings and their assignments; make `envLastCurve` a
local:
```js
// syncState / sourceState / freezeState / divisionState removed — each binder
// already closes over its own `st`, and nothing outside a binder ever needed
// one. shapeState STAYS: refreshTaperEnabled() and initEnvelope() both read it.
if (Array.isArray(curve) && curve.length >= 2) drawEnvelope(curve.map(Number));
```
**Rationale:** Proven by repo-wide grep: each of the four appears exactly twice — the
declaration and one assignment. `shapeState` is the counter-example and must stay (read at
:548, :552, :704, :709). `envLastCurve`'s comment promises a DPR-redraw path that does not
exist; `envResize()` is called from inside `drawEnvelope()` on every draw, so the redraw
always refetches. The payoff is specifically about this file's stated invariant: the header
at :30-34 declares the top declaration block load-bearing because a top-level reference to
a not-yet-initialised binding is a `ReferenceError` that kills the entire UI
(`pattern_module_toplevel_init_tdz`). Every binding in that block is a thing a future
editor must reason about; five of them carry no information.
**Test impact:** `ui_frontend_check.js:764` parses `KNOB_IDS` and `:849` parses `FORMAT`,
neither of which is affected. Risk is MEDIUM rather than LOW because JS has no compiler to
catch a missed reference — proof must be a *render*, not a read: serve the page through
`tests/ui-stub/`, confirm zero console errors, and drive every control (segments, both
selects, the freeze pair, a knob drag, dblclick-reset) plus a full
`ui_tooltip_clamp_check.js` run. `pattern_module_extraction_regression_check` — a JS
`ReferenceError` on load passes build, `auval` and every static check while killing the
whole WebView UI.
**Lines saved:** 9

---

### [MED-04] The segment-pair paint block is written three times in `app.js`

**File:** `Source/ui/public/js/app.js:426-429`, `Source/ui/public/js/app.js:463-466`, `Source/ui/public/js/app.js:496-499`
**Type:** duplication
**Risk:** MEDIUM
**Current:** the same four lines in `bindSyncSegments`, `bindFreezeSegments` and
`bindSourceSegments`:
```js
segFree.classList.toggle("active", !isSync);
segSync.classList.toggle("active", isSync);
segFree.setAttribute("aria-pressed", String(!isSync));
segSync.setAttribute("aria-pressed", String(isSync));
```
**Proposed:**
```js
// Paint a two-segment pair. CLASSES AND aria-pressed ONLY — this function must
// never write textContent. The FREE/SYNC, OFF/FREEZE and MONO/STEREO copy is
// authored in index.html, and a shared updater that writes textContent is
// exactly what erased HTML-authored labels in every DAW since launch
// (pattern_js_state_updater_overwrites_html_labels).
function paintSegmentPair(segA, segB, bActive) {
  segA.classList.toggle("active", !bActive);
  segB.classList.toggle("active", bActive);
  segA.setAttribute("aria-pressed", String(!bActive));
  segB.setAttribute("aria-pressed", String(bActive));
}
```
Each binder's `refresh()` becomes one call; the three binders keep their distinct state
APIs (`getChoiceIndex`/`setChoiceIndex` for the two combos, `getValue`/`setValue` for the
toggle) and `bindSyncSegments` keeps its extra time-slot swap.
**Rationale:** This is the *shared* half of three functions that the file's own comments
(:444-448, :477-484) correctly argue must not be merged wholesale — the state APIs are not
interchangeable and `bindSyncSegments` owns the UI-02 slot swap. Those arguments are about
the *binding*, not the painting, and the painting is identical in all three. Extracting only
the painted half keeps every reason the three binders exist separately.
**Named risk (required):** the consolidated updater must touch `classList` and
`aria-pressed` and nothing else. `pattern_js_state_updater_overwrites_html_labels` — a
shared JS state updater that writes `textContent` erases HTML-authored labels, was live in
every DAW since launch, and reading the source did not catch it. The proposed function has
no `textContent` write and no element-text access at all; that constraint is stated in the
function's own comment above so a later edit has to delete the warning to break it.
**Test impact:** `ui_frontend_check.js:256` already asserts `app.js` never assigns
`textContent` on the syncMode segments — that check must be widened to cover the new shared
function and all three pairs. Proof must be a render: confirm the page shows FREE/SYNC,
OFF/FREEZE and MONO/STEREO (not blank or duplicated) after clicking each segment, in the
ui-stub AND in Standalone.
**Lines saved:** 8

---

### [MED-05] The two preset-dialog native functions duplicate `makeResult` and the chooser scaffolding

**File:** `Source/PluginEditor.cpp:371-382` and `Source/PluginEditor.cpp:420-434`
**Type:** duplication
**Risk:** LOW
**Current:** an identical 8-line `makeResult` lambda is defined inside each of
`savePresetWithDialog` and `loadPresetFromFile`, followed by the same re-entry guard and
the same `std::make_shared<juce::FileChooser>` construction differing only in title:
```cpp
auto makeResult = [] (bool ok, const juce::String& name)
{
    auto* obj = new juce::DynamicObject();
    obj->setProperty ("success", ok);
    obj->setProperty ("name", name);
    return juce::var (obj);
};
if (fileDialogOpen) { complete (makeResult (false, {})); return; }
fileDialogOpen = true;
fileChooser = std::make_shared<juce::FileChooser> ("Save Preset", …, "*.json");
```
**Proposed:** hoist to the anonymous namespace at the top of the file, beside
`makeBinaryResource`:
```cpp
// Both dialog fns MUST resolve {success, name} — preset-manager.js checks
// `result && result.success`, so a bare bool silently no-ops the bar. ONE
// definition, because "both" is the whole contract.
juce::var makePresetDialogResult (bool ok, const juce::String& name)
{
    auto* obj = new juce::DynamicObject();
    obj->setProperty ("success", ok);
    obj->setProperty ("name", name);
    return juce::var (obj);
}
```
**Rationale:** The comment at :358 ("Both dialog fns MUST resolve {success, name}") states
a contract that spans two functions and is then implemented twice. One definition makes the
contract enforceable. Deliberately **not** proposed: merging the two `launchAsync` bodies.
The `SafePointer` hoist at :369 and :418 is required separately in each lambda
(`critical_msvc_safepointer_init_capture_nested_lambda`), and the completion bodies
genuinely differ (`savePresetWithDialog` branches on `isAChildOf` the user dir; the
open-mode path checks `existsAsFile`). Both also correctly use a bare `return` on the null
`safeThis` path (`pattern_webview_launchasync_safepointer_no_complete`) and that must not
be routed through any shared helper that could call `complete()`.
**Test impact:** Compile + `auval` is sufficient for the lambda hoist. Manual smoke test of
the preset bar's Save-with-dialog and Load-from-file legs in a DAW, including the
double-click re-entry case that exercises the `fileDialogOpen` guard, since a wrong result
shape silently no-ops the bar rather than erroring.
**Lines saved:** 10

---

### [MED-06] The ui-stub mirrors four C++ `StringArray`s and one default-index map; only `grainShape` is validated

**File:** `tests/ui-stub/juce-stub.js:95-112`, checked at `tests/ui_frontend_check.js:1003-1009`
**Type:** duplication
**Risk:** LOW
**Current:**
```js
// juce-stub.js — four mirrored choice lists and four mirrored defaults
const CHOICES = {
  syncMode:   ["Free", "Sync"],
  sourceMode: ["Mono Sum", "Stereo"],
  grainShape: ["Hann", "Tukey", "Gaussian", "Triangular", "Expo-Decay"],
  noteDivision: [ "1/16", "1/16D", … 13 entries … ],
};
const DEFAULT_CHOICE = { syncMode: 1, noteDivision: 6, grainShape: 0, sourceMode: 0 };
```
```js
// ui_frontend_check.js — validates ONE of them
check(… names(shapeDecl) === names(stubShape) && …length === 5,
    'ui-stub grainShape choices match the C++ StringArray (5 entries)');
```
**Proposed:** generalise the existing check to a loop over all four, parsing each
`StringArray` out of `createParameterLayout()` and each default index from the same
declaration:
```js
// Every stub choice list, against its C++ StringArray — and every stub default
// index, against the C++ trailing default. grainShape was checked from v1.2.0;
// the other three were added later and inherited the mirror without the check.
// ORDER is load-bearing for all four: index 0 is what an absent key in an older
// session or preset resolves to, so it must be the shipped behaviour.
for (const id of ['syncMode', 'noteDivision', 'grainShape', 'sourceMode']) { … }
```
**Rationale:** The `grainShape` check exists because a mirrored list drifting from the C++
means the browser render shows options the plugin does not have — and the render is what
the whole ui-stub exists for. That reasoning applies unchanged to the other three, and
`noteDivision` is both the longest list (13 entries, where a transposition is easiest to
make and hardest to see) and the one whose order is tied to `kDivisionBeats` (see HIGH-03).
`DEFAULT_CHOICE` has the same exposure: `ui_frontend_check.js:995` asserts `grainShape`
defaults to index 0 in the C++ but never that the *stub* agrees, so a stub whose
`syncMode` defaulted to 0 would render the page in Free mode and every clamp measurement
would be taken against the wrong TIME control.
**Test impact:** Change is inside the test suite. Verify by running
`node tests/ui_frontend_check.js` (all-pass) and by temporarily transposing two entries in
the stub's `noteDivision` array — the new check must fail, the current suite does not.
**Lines saved:** -6 (adds lines; closes three unchecked mirrors)

---

### [MED-07] Stale geometry and capacity numbers scattered across six files after the v1.7.1 shrink and the v1.5.0/v1.7.0 ring growth

**File:** `Source/ui/public/css/styles.css:484`, `Source/ui/public/index.html:356`, `Source/ui/public/js/app.js:526`, `Source/PluginProcessor.h:651`, `Source/PluginProcessor.cpp:997`, `Source/dsp/ReverseGrain.h:16`, `tests/ui_frontend_check.js:49`, `tests/ui_frontend_check.js:66`
**Type:** naming
**Risk:** LOW
**Current:**
```
styles.css:484        "its 215 px height from the base .group rule"    (base is 145)
styles.css:88         "re-measured at 940 x 972"                       (is 940 x 768)
index.html:356        "the 940 x 743 frame is untouched"               (is 940 x 768)
app.js:526            "Defaults for dblclick-reset (the only native function)"
                                                            (three are fetched in app.js; 13 total)
PluginProcessor.h:651 "CaptureBuffer capture;  // 5.5 s stereo ring"   (kCaptureSeconds = 14.0)
PluginProcessor.cpp:997 "capture ring ~3.5 s stereo"                   (14 s; ~5.4 MB at 48 kHz)
ReverseGrain.h:16     "e = windowLuts.readAt (win, q);"    (render loop calls readShaped since v1.4.0)
ui_frontend_check.js:49 "Geometry is 940 x 484"     (the assertion below it checks 768)
ui_frontend_check.js:66 "the real 940 x 743 shipping size"             (768)
```
**Proposed:** correct each to the current value in place. Where a superseded number is
worth keeping (styles.css's history block already does this deliberately and well), keep it
under an explicit "superseded" heading rather than in a live sentence.
**Rationale:** Individually cosmetic; collectively these are the numbers a reader uses to
orient. The ring ones are the most costly: `PluginProcessor.h:651` and
`PluginProcessor.cpp:997` both understate `kCaptureSeconds` by 2.5–4×, and the ring size is
the subject of a `static_assert` (`:628`) and of `pattern_ring_invariant_needs_static_assert`
— a reader checking "does the ring cover gD_max + 2·G_max" against the 5.5 s in the member
comment reaches the wrong conclusion. `ReverseGrain.h:16`'s pseudo-code of the render loop
names a function the loop no longer calls, which matters because that block is the
file's summary of the hot path. Note `styles.css`'s main header block (lines 9-58) is
*correct and excellent* and should not be touched — the drift is only in the older
sections kept below it and in the row-3 block at :481-491.
**Test impact:** Comment-only; compile + `auval` + `node tests/ui_frontend_check.js`.
Worth adding: the frontend check already extracts `shipH` from `setSize` — it could assert
that no superseded height string (`484`, `743`, `972`) appears in `index.html` or `app.js`
prose, the way it already does for `styles.css` at line 401-405.
**Lines saved:** 0

---

### [LOW-01] `WindowLut::read()` and `WindowLut::getSize()` have no callers

**File:** `Source/dsp/WindowLut.h:223-226`, `Source/dsp/WindowLut.h:493`
**Type:** dead-code
**Risk:** LOW
**Current:**
```cpp
/** Convenience read for non-hot paths (tests, offline analysis). */
float read (int shape, float phase) const noexcept
{
    return readAt (getTable (shape), phase);
}
...
int getSize() const noexcept { return size; }
```
**Proposed:** delete both.
**Rationale:** Repo-wide grep across `Source/` and `tests/` finds zero call sites for
either. `read()`'s doc comment offers it to "tests, offline analysis" and the render
harness uses `readAt`/`readShaped`/`getTable` directly instead; the only `getSize()` hit in
the repo is `juce::MemoryBlock::getSize()` at `main.cpp:1737`. Every other accessor in this
class — including all four convenience overloads (`getShapeNorm(int)`,
`getTiltNorm(int,float)`, `getLoopNorm(int,float)`, `getMeanSquare`, `getMean`) — **is**
live from the harness and must be kept; I checked each individually before filing this.
**Test impact:** Compile of both the plugin and the harness (`-DOUARICON_BUILD_TESTS=ON`)
is sufficient proof — an unreferenced member function's removal cannot change behaviour.
**Lines saved:** 6

---

### [LOW-02] Six `data-choice` attributes in `index.html` are read by nothing

**File:** `Source/ui/public/index.html:87-88`, `:382-383`, `:431-432`
**Type:** dead-code
**Risk:** LOW
**Current:**
```html
<button type="button" class="segment" id="seg-free"         data-choice="0" aria-pressed="false">Free</button>
<button type="button" class="segment" id="seg-sync"         data-choice="1" aria-pressed="true">Sync</button>
<button type="button" class="segment" id="seg-freeze-off"   data-choice="0" aria-pressed="true">Off</button>
<button type="button" class="segment" id="seg-freeze-on"    data-choice="1" aria-pressed="false">Freeze</button>
<button type="button" class="segment" id="seg-source-mono"  data-choice="0" aria-pressed="true">Mono</button>
<button type="button" class="segment" id="seg-source-stereo" data-choice="1" aria-pressed="false">Stereo</button>
```
**Proposed:** delete the six `data-choice` attributes.
**Rationale:** Grep across `index.html`, `app.js`, `styles.css` and `tests/*.js` finds no
`dataset.choice`, no `[data-choice]` selector and no test reference. All three binders
hardcode the index at the click handler (`st.setChoiceIndex(0)` / `(1)` / `st.setValue(false)`
/ `(true)`). The attribute reads as live wiring and is not — a maintainer changing
`data-choice` would produce no effect, which is the same shape of confusion as a dead
control. It is also misleading on the freeze pair specifically, where the parameter is a
`bool` with no choice index at all.
**Test impact:** Visual smoke test through `tests/ui-stub/` confirming all three segment
pairs still toggle. `ui_frontend_check.js` does not reference the attribute.
**Lines saved:** 0 (6 attributes; no line count change)

---

### [LOW-03] `refreshTaperEnabled()` and `refreshDriftRateEnabled()` are the same function against different conditions

**File:** `Source/ui/public/js/app.js:516-524` and `Source/ui/public/js/app.js:546-555`
**Type:** duplication
**Risk:** LOW
**Current:**
```js
function refreshDriftRateEnabled() {
  const cell = document.getElementById("cell-driftRate");
  const st = sliderState.driftDepth;
  if (!cell || !st) return;
  const live = st.getScaledValue() > 0;
  cell.classList.toggle("knob-cell-inert", !live);
  cell.setAttribute("aria-disabled", String(!live));
}
// refreshTaperEnabled is the same four statements against shapeState.getChoiceIndex() === 1
```
**Proposed:**
```js
// Dim an INAPPLICABLE cell. Class + aria only — the .knob-label text is authored
// in HTML and the .knob-value is owned by updateKnobVisual; neither is touched
// here (pattern_js_state_updater_overwrites_html_labels). The relay stays bound
// either way, so nothing here can make a control dead — only say it is inert.
function setCellApplicable(cellId, live) {
  const cell = document.getElementById(cellId);
  if (!cell) return;
  cell.classList.toggle("knob-cell-inert", !live);
  cell.setAttribute("aria-disabled", String(!live));
}
function refreshTaperEnabled()     { if (shapeState) setCellApplicable("cell-tukeyTaper", shapeState.getChoiceIndex() === 1); }
function refreshDriftRateEnabled() { const st = sliderState.driftDepth;
                                     if (st) setCellApplicable("cell-driftRate", st.getScaledValue() > 0); }
```
**Rationale:** `app.js:511-512` already says "Same shape as refreshTaperEnabled, and for
the same reason". Two panels use this pattern and a third is likely when COLOUR is filled;
one helper makes the next one a one-liner. Same `textContent` constraint as MED-04 applies
and is stated in the helper's comment.
**Test impact:** Render check — switch Shape off and onto Tukey and confirm the Taper cell
dims/undims, and raise Drift Depth off zero and confirm the Rate cell lights, in the
ui-stub. `ui_frontend_check.js` does not parse these functions.
**Lines saved:** 4

---

### [LOW-04] `CaptureBuffer::readAbs()` and `monoSum()` duplicate the double-mod index

**File:** `Source/dsp/CaptureBuffer.h:141-153`
**Type:** duplication
**Risk:** LOW
**Current:**
```cpp
float readAbs (int ch, juce::int64 absIndex) const noexcept
{
    const int idx = static_cast<int> (((absIndex % bufferSize) + bufferSize) % bufferSize);
    return buffer.getSample (ch, idx);
}
float monoSum (juce::int64 absIndex) const noexcept
{
    const int idx = static_cast<int> (((absIndex % bufferSize) + bufferSize) % bufferSize);
    return 0.5f * (buffer.getSample (0, idx) + buffer.getSample (1, idx));
}
```
**Proposed:**
```cpp
/** Absolute (monotonic) index -> ring index. Double-mod handles negatives, so a
    pre-history read lands on the cleared buffer's zeros rather than out of bounds. */
int wrapIndex (juce::int64 absIndex) const noexcept
{
    return static_cast<int> (((absIndex % bufferSize) + bufferSize) % bufferSize);
}
float readAbs (int ch, juce::int64 absIndex) const noexcept
{ return buffer.getSample (ch, wrapIndex (absIndex)); }

float monoSum (juce::int64 absIndex) const noexcept
{
    const int idx = wrapIndex (absIndex);
    return 0.5f * (buffer.getSample (0, idx) + buffer.getSample (1, idx));
}
```
**Rationale:** The wrap is the ring's one non-obvious invariant and it is currently written
twice with the explanatory comment attached to only one of them. Both are on the audio
path; a `const noexcept` member with a single integer expression inlines identically, and
`monoSum` must keep `0.5f * (L + R)` as one expression — `PluginProcessor.cpp:1695-1697`
records that `0.5·(L+R)` is **not** bit-equal to `0.5·L + 0.5·R` once denormals are in
play, so the proposal deliberately does not touch that line.
**Test impact:** The full 138-probe harness, asserting bit-identity against a defaults
render — this is on the hot read path and the whole suite exercises it. `-O3` inlining
should make the emitted code identical; if a probe moves by even one ULP, reject the
candidate.
**Lines saved:** 2

---

### [LOW-05] Three copies of the `typeof raw === "string" ? JSON.parse(raw) : raw` payload normalisation

**File:** `Source/ui/public/js/app.js:530`, `Source/ui/public/js/app.js:654`, `Source/ui/public/js/app.js:749`
**Type:** duplication
**Risk:** LOW
**Current:**
```js
paramDefaults = typeof raw === "string" ? JSON.parse(raw) : raw;   // :530
const curve   = typeof raw === "string" ? JSON.parse(raw) : raw;   // :654
const m       = typeof raw === "string" ? JSON.parse(raw) : raw;   // :749
```
**Proposed:**
```js
// A native fn's var payload arrives as an object on some backends and as a JSON
// string on others; every call site has to tolerate both.
const parsePayload = (raw) => (typeof raw === "string" ? JSON.parse(raw) : raw);
```
**Rationale:** One name for a backend quirk that currently has to be re-recognised at each
of three call sites. Small, but it makes the *reason* nameable — none of the three sites
explains why the string branch exists.
**Test impact:** Render check that all three consumers still work: the grain meter counts
up under audio, the envelope curve paints, and a knob dblclick resets to its default (the
`getParameterDefaults` path). `ui_frontend_check.js` does not parse these expressions.
**Lines saved:** 2

---

### [LOW-06] `initEnvelope()` registers two identical listener bodies on `shapeState`

**File:** `Source/ui/public/js/app.js:704-713`
**Type:** verbose-pattern
**Risk:** LOW
**Current:**
```js
if (shapeState) {
  shapeState.valueChangedEvent.addListener(() => {
    refreshTaperEnabled();
    scheduleEnvelopeRedraw();
  });
  shapeState.propertiesChangedEvent.addListener(() => {
    refreshTaperEnabled();
    scheduleEnvelopeRedraw();
  });
}
```
**Proposed:**
```js
if (shapeState) {
  const onShapeChanged = () => { refreshTaperEnabled(); scheduleEnvelopeRedraw(); };
  shapeState.valueChangedEvent.addListener(onShapeChanged);
  shapeState.propertiesChangedEvent.addListener(onShapeChanged);
}
```
**Rationale:** Matches the shape of the loop 8 lines above it (`:696-702`), which already
registers one callback on both events. The current form makes the two handlers look like
they *might* differ.
**Test impact:** Render check — change Shape and confirm both the Taper cell's dim state
and the envelope curve update, in the ui-stub.
**Lines saved:** 3

---

## Not proposed, and why

Recorded so a later pass does not re-derive them:

- **Table-driving the 25 `std::atomic<float>*` members** (`PluginProcessor.h:764-818`,
  `PluginProcessor.cpp:12-48`). ~50 lines of visually repetitive plumbing, but each
  pointer is a named, greppable symbol that `processBlock` reads directly. An array plus
  an index enum would be shorter and strictly worse to debug.
- **`GrainScheduler::prepare(double /*sampleRate*/)`** (`GrainScheduler.h:102`). An unused
  parameter, documented at :98-101 as deliberately kept for signature symmetry with the
  other DSP components. Leave it.
- **The Tukey table built in `build()`** (`WindowLut.h:582-592`). Not rendered from since
  v1.4.0, but it is the independent `std::cos` reference that harness **probe AH**
  measures the Hann-remap against. Deleting it would make the probe compare the remap to
  itself. Documented as such at :576-581; the documentation is correct.
- **Merging `bindSyncSegments` / `bindFreezeSegments` / `bindSourceSegments` wholesale.**
  The two comments arguing against it (`app.js:444-448`, `:477-484`) are right: the
  `ToggleState` and `ComboBoxState` APIs are not interchangeable, and `bindSyncSegments`
  owns the UI-02 time-slot swap. Only the painted half is proposed — see MED-04.
- **Any CSS deletion justified by row-height arithmetic.** Per
  `pattern_flex1_container_slack_invisible_to_row_sum`; the two CSS candidates here
  (MED-01, MED-02) are merges justified by selector-reference checks, and the one slack
  assertion touched (HIGH-07) is proposed to be *measured* rather than summed.
