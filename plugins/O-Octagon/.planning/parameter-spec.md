# O-Octagon — Parameter Specification

> **Promoted spec.** This file supersedes `parameter-spec-draft.md` and is the single source of
> truth for the shipped parameter set. Sources: `research/ARCHITECTURE.md` §6.1 (ranges, defaults,
> skews), `stages/1-foundation/RESEARCH.md` §3.2 (labels, display precision),
> `stages/1-foundation/PLAN.md` P1 (grouping).
>
> **Status:** final for Stage 1. Promoted 2026-08-11.

---

## Resolved — the three places the draft is stale

The draft still marks these open. All were resolved at Stage 0 and are **closed**:

| Draft item | Resolution | Authority |
|---|---|---|
| "18 musical parameters" vs a 17-row table | **17 at Stage 1.** The 18 was an arithmetic slip in BRIEF.md; no 18th parameter was invented *then*. `4 + 2 + 8 + 2 + 1 = 17`. **Superseded by v1.5.0's `decorr` — see Amendments.** The count is 18 today, and the coincidence with the draft's slip is exactly that | ARCHITECTURE §11 |
| **OQ5** — venue in APVTS or a separate tree? | **Separate `ValueTree`**, a `VENUE` child of `apvts.state` (root `OOctagon`). Not automatable, never written by a musical preset | ARCHITECTURE §4.1 |
| **OQ3 / OQ4** — blur cap, air curve, venue default scale | Resolved at Stage 0; they constrain Stage 2 DSP, not the parameter set. Ranges and defaults below are final regardless | ARCHITECTURE §6.1, §OQ3/OQ4 |

The venue's 42 values are **not** parameters. They are specified in ARCHITECTURE §6.2 and land at
Phase 2.1. Nothing in this file describes them.

---

## Musical parameters — 17, APVTS, automatable

All are `juce::AudioParameterFloat`. **All skews linear** — every range is already perceptually
linear in its own units, and the headline gesture is *automating* `w1..w8` and position, where a
skewed lane would draw a curve that does not match what is heard.

There are **no** Choice or Bool parameters in the musical set; every "defeat" is a value of 0.

### Group `position` — "Position"

| # | ID | Name | Range | Default | Skew | `withLabel` | Display |
|---|----|------|-------|---------|------|-------------|---------|
| 1 | `srcX` | Source X | 0.0 – 1.0 | 0.5 | linear | *(none)* | 2 dp, normalised |
| 2 | `srcY` | Source Y | 0.0 – 1.0 | 0.5 | linear | *(none)* | 2 dp, normalised |
| 3 | `srcZ` | Source Z | −2.0 – 8.0 | 0.0 | linear | `m` | 2 dp |
| 4 | `width` | Width | 0.0 – 6.0 | 0.0 | linear | `m` | 2 dp |

### Group `solve` — "Solve"

| # | ID | Name | Range | Default | Skew | `withLabel` | Display |
|---|----|------|-------|---------|------|-------------|---------|
| 5 | `rolloff` | Rolloff | 3.0 – 6.0 | 4.0 | linear | `dB/2x` | 1 dp |
| 6 | `blur` | Blur | 0.0 – 1.0 | 0.10 | linear | *(none)* | 2 dp |

### Group `weights` — "Weights"

| # | ID | Name | Range | Default | Skew | `withLabel` | Display |
|---|----|------|-------|---------|------|-------------|---------|
| 7–14 | `w1` … `w8` | Weight 1 … Weight 8 | 0.0 – 1.0 | 1.0 | linear | *(none)* | 2 dp |

### Group `space` — "Space"

| # | ID | Name | Range | Default | Skew | `withLabel` | Display |
|---|----|------|-------|---------|------|-------------|---------|
| 15 | `hullAtten` | Hull Atten | 0.0 – 3.0 | 1.0 | linear | `dB/m` | 2 dp |
| 16 | `airAmount` | Air | 0.0 – 1.0 | 0.35 | linear | *(none)* | 2 dp |

### Group `output` — "Output"

| # | ID | Name | Range | Default | Skew | `withLabel` | Display |
|---|----|------|-------|---------|------|-------------|---------|
| 17 | `outputGain` | Output | −24.0 – 12.0 | 0.0 | linear | `dB` | 1 dp |

---

## Units — why these labels

- **`dB/2x`, not `dB/doubling`.** Logic truncates the unit field hard and the prose form does not
  survive it (RESEARCH §3.2).
- **Five parameters take no label deliberately** — `srcX`, `srcY`, `blur`, `airAmount`, `w1..w8`.
  A bare `0.00–1.00` reads correctly. Inventing "norm" or "%" would misrepresent `w1..w8`, which are
  DBAP weights and not percentages of anything.

## `srcX` / `srcY` display: normalised in the host, metres in the UI

Stored **and displayed in the host lane as normalised 0.00–1.00.**

Metres are a **Stage 3.1 UI-side conversion**. The metre readout depends on the current venue's
bounding box, so the value→text conversion is venue-dependent; a host `value→text` lambda is
captured at parameter-construction time and therefore cannot read a live venue. ARCHITECTURE §6.1's
"displayed in metres" mandate is about the *UI readout*, which is a UI concern.

Per `pattern_webview_knob_readout_scaled_value`, the UI must not maintain its own min/max map — it
must ask the processor.

**Do not attempt a `this`-capturing value→text lambda in `createParameterLayout()`.**

---

## Grouping — five `AudioProcessorParameterGroup`s

| Group ID | Group name | Members |
|---|---|---|
| `position` | Position | `srcX` `srcY` `srcZ` `width` |
| `solve` | Solve | `rolloff` `blur` |
| `weights` | Weights | `w1`..`w8` |
| `space` | Space | `hullAtten` `airAmount` |
| `output` | Output | `outputGain` |

Separator `"|"`. The headline gesture is automating eight weights; a flat 17-entry menu buries them.
Group membership does not participate in parameter identity, so this is reversible if Stage 4 host
testing disagrees.

---

## Construction rules (Stage 1)

1. **`juce::ParameterID { "srcX", 1 }`** — the version hint is mandatory in JUCE 8.
2. **Two-argument `NormalisableRange<float>(min, max)`** — yields interval 0, skew 1. Do *not* write
   the 4-argument form with an explicit `1.0f` skew; it reads as though a skew were intended and
   invites a future "fix".
3. Labels via `juce::AudioParameterFloatAttributes().withLabel(...)`.
4. **Parameter-ID hazards checked:** none of these 17 IDs shadows a `juce::` free function
   (cf. `critical_paramid_shadows_juce_free_function`, where `end`/`begin` collided). All 17 are
   safe as both IDs and C++ identifiers.

## State

APVTS root identifier is **`OOctagon`** — not the sibling O-Orbit's `OOrbitParams` idiom. Phase 2.1
attaches the `VENUE` child to this exact node; changing it later orphans every saved session and
every `.venue` file written in between. **This identifier must never change.**

---

## Count

| Group | Count |
|-------|-------|
| Position | 4 |
| Solve | 2 |
| Weights | 8 |
| Space | 2 |
| Output | 1 |
| **Total musical (automatable)** | **17** |
| Venue values (separate `ValueTree`, ARCHITECTURE §6.2) | 42 |

*(Stage-1 figures. Live totals: 18 musical, 50 venue — see Amendments.)*

---

## Amendments after Stage 1

**This file is a Stage-1 snapshot and the tables above are frozen at that boundary.** Ranges and
counts have moved since; `oo::params` in `Source/DSP/GainStage.h` and `createParameterLayout()` in
`Source/PluginProcessor.cpp` are the live source of truth, and `ui_frontend_check.js` §16 closes
them against the UI four ways. What follows is the ledger of what changed, not a rewrite.

| Version | Change |
|---|---|
| v1.3.0 | `width` max 6 → 12 m; `rolloff` max 6 → 12 dB/2x; `blur` default 0.10 → 0.03 (`kBlurScale` tripled). Pre-1.3 presets re-mapped by the editor's migration hook |
| v1.4.0 | Venue values 42 → 50: a per-speaker alignment delay. **Not** a parameter — venue-scoped, so no automation lane reaches it |
| v1.5.0 | **18th musical parameter: `decorr`** |
| v1.7.0 | The monitor fold-down adds **NO parameter** — deliberately. See below |
| v1.8.0 | **Ten motion parameters** (`motion` group), kCount 18 → 28. First Bool, first Choices, first non-linear skew. See below |

### v1.5.0 — `decorr`

| # | ID | Name | Range | Default | Skew | `withLabel` | Display |
|---|----|------|-------|---------|------|-------------|---------|
| 5 | `decorr` | Decorrelate | 0.0 – 1.0 | **0.0** | linear | *(none)* | 2 dp |

Group `position`, inserted directly after `width` — the enum order is the control-block snapshot
layout, so `rolloff`…`outputGain` shift by one index. Nothing persists an index (`w1 + i` is the
only index arithmetic in the codebase and `w1..w8` stay contiguous), and every id-keyed path is
unaffected.

**`AudioParameterFloat`, deliberately, so "there are no Choice or Bool parameters in the musical
set" above stays true.** A bool would have been the natural spelling for a defeatable feature and
was rejected: it would have broken the uniform `WebSliderRelay` loop, and the depth axis turns out
to be genuinely continuous — it scales the decorrelation network's delay lengths, so the control
sweeps dispersion time rather than switching a mode.

**The 0.0 default is a compatibility guarantee, not a taste.** At 0 the network is bypassed
structurally, so every session and preset written before v1.5.0 renders bit-identically — held by
probe CU against the v1.4.0 binary's own render digest.

**Preset scope: preserved, not authored.** `decorr` joins `oo::presets::kPreserved` (11 → 12), so
a factory preset load leaves it alone. It describes the *material* — is this stem effectively
mono? — not the room the preset is painting.

### v1.7.0 — the monitor fold-down adds NO parameter, and that is the feature

The count stays at **18**. An `AudioParameterBool` named `monitor` would have been the natural
spelling for a defeatable feature — `decorr`'s note above explains why that spelling was rejected
there for UI-uniformity reasons — and here it is rejected for a stronger one:

**An APVTS parameter has an automation lane, and an automation lane is a recording of the arm that
a bounce can replay.** The monitor fold writes a headphone mix into two carrier channels and hard-
zeroes the other six. A lane that could restore that state during an offline render is precisely
the failure the whole feature is built to be incapable of, so the arm is a bare `std::atomic<bool>`
on the processor, reachable only from the editor's `setMonitorArmed` native function.

It is also **absent from `getStateInformation()`**, which makes it unlike `tooltipsEnabled` and
`uiLanguage` — the two other non-parameter UI booleans, both of which legitimately ride the
session. A reopened session is always disarmed. That is the only guard covering a *realtime*
bounce, where `isNonRealtime()` correctly reports real time and cannot help.

Consequences worth stating once, because each is a question someone will ask:

- **Preset scope: none.** It is not in `oo::presets::kPreserved` and does not need to be — a preset
  cannot see it at all.
- **No migration.** No id, no range, no index; nothing to version-gate.
- **`params::kCount` is untouched**, so the control-block snapshot layout and its `memcmp` dirty
  check are byte-for-byte what v1.6.0 had.

### v1.8.0 — the `motion` group (28 parameters)

Appended at the END of `oo::params::Index`, so every pre-existing index — and the control-block
snapshot layout the dirty check memcmps — is untouched. Motion is an internal metric OFFSET added
downstream of `srcX/srcY/srcZ` (CONTEXT D1); the three position lanes are never written.

| # | ID | Name | Type | Range / choices | Default | Skew | `withLabel` | Display |
|---|----|------|------|-----------------|---------|------|-------------|---------|
| 19 | `motionOn` | Motion On | Bool | off / on | **off** | — | — | On/Off |
| 20 | `motionPath` | Motion Path | Choice | Orbit, Figure-8, Sweep, Drift, Pendulum, Spiral | Orbit | — | — | name |
| 21 | `motionSync` | Motion Sync | Choice | Free, 1/16T, 1/16, 1/16D, 1/8T, 1/8, 1/8D, 1/4T, 1/4, 1/4D, 1/2, 1/2D, 1 Bar, 2 Bars, 4 Bars | Free | — | — | name |
| 22 | `motionRate` | Motion Rate | Float | 0.01 – 4.0 | 0.1 | **centre 0.3** | `Hz` | 2 dp |
| 23 | `motionSize` | Motion Size | Float | 0.0 – 24.0 (extent / diameter) | 6.0 | linear | `m` | 1 dp |
| 24 | `motionRatio` | Motion Ratio | Float | 0.0 – 1.0 | 1.0 | linear | *(none)* | 2 dp |
| 25 | `motionAngle` | Motion Angle | Float | 0 – 360 | 0 | linear | `deg` | 0 dp |
| 26 | `motionHeight` | Motion Height | Float | 0.0 – 8.0 | 0.0 | linear | `m` | 1 dp |
| 27 | `motionPhase` | Motion Phase | Float | 0 – 360 | 0 | linear | `deg` | 0 dp |
| 28 | `motionSeed` | Motion Seed | Float, step 1 | 1 – 64 | 1 | linear | *(none)* | 0 dp |

**Three firsts, each deliberate.** `motionOn` is the first Bool and `motionPath`/`motionSync` the
first Choices — a host lane must read "Figure-8", not 0.2 — so the editor gains a
`WebToggleButtonRelay` and two `WebComboBoxRelay`s beside the float loop. `motionRate` is the
first non-linear range: 0.01–4 Hz with the centre at 0.3 Hz, because a slow orbit is the musical
default and a linear lane would spend most of its travel above 0.4 Hz.

**`motionOn = off` is a compatibility guarantee**, exactly as `decorr = 0` was: GainStage takes the
v1.7.0 `shape()` call and dirty-check predicate verbatim on the off branch, held by probe DC
against the v1.7.0 binary's digest (`0xb8c5a2d0c7518204`).

**Preset scope: AUTHORED.** All ten join `oo::presets::kAuthored` (6 → 16) so motion travels in
presets (R7). The six original factory rows write nothing for them, so WR-01 lands `motionOn = 0`
on every one of those loads; a pre-1.8.0 preset omits the keys and loads motion-off (probe DK).
**No migration hook** — no range moved.
