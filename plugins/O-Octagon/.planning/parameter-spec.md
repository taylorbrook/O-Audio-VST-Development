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
| "18 musical parameters" vs a 17-row table | **17.** The 18 was an arithmetic slip in BRIEF.md; no 18th parameter exists or is invented. `4 + 2 + 8 + 2 + 1 = 17` | ARCHITECTURE §11 |
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
