# O-Octagon Notes

## Status
- **Current Status:** 🚧 Stage 4 — phase 4.1 (machine gates) complete; 4.2 (host-and-ear) open
- **Version:** 1.0.0 (dev build installed; not released)
- **Type:** Audio Effect (8-Channel DBAP Spatializer)
- **Build target:** `OuariconOctagon` (folder `plugins/O-Octagon`) — `PLUGIN_CODE OuOc`
- **Complexity:** 5.0 (capped; raw 13.0) — staged implementation

## Lifecycle Timeline

- **2026-08-10:** Prior research completed — `research/logic-pro-multichannel-octaphonic-dbap.md`
  establishes the locked architecture (Logic's 10 named surround formats, `aufx` single-bus
  constraint, the `AudioChannelSet` bitset trap, DBAP 2011-04-14 revised equations)
- **2026-08-10 (Ideation):** Creative brief and 30 requirements documented
- **2026-08-11 (Stage 0):** Research & Planning complete — ARCHITECTURE.md and ROADMAP.md
  documented (Complexity 5.0, staged). All 5 open questions resolved; parameter-count
  discrepancy resolved to 17.
- **2026-08-11 (Stage 1):** Foundation shell — CMake, APVTS (17 parameters), 8-channel transport
  validated.
- **2026-08-12 (Stage 2, phases 2.1–2.3):** Geometry core (channel map, convex hull, audience
  plane), DBAP solve, source shaping and outside-hull processing.
- **2026-08-12 (Stage 3, phases 3.1–3.3):** WebView UI — room plan and puck, venue editor and
  `.venue` I/O, verify ping, preset rail, scenes, field visualisation, eight meters.
- **2026-08-12 (Stage 4, phase 4.1):** Machine gates. Per-commit CI added; first MSVC compile; six
  factory presets; `COMPAT-04` closed 3 of 3; `COMPAT-01` re-confirmed on the final binary.
  95 probes, 0 failures.

## Known Issues

**Registered risks** — see
`.planning/stages/0-ideation/CONTEXT.md` for the full register:

- **R1 (CRITICAL):** the speaker→buffer channel map fails *silently*. A wrong map does not crash,
  does not produce NaN, and passes `auval`, `pluginval` strictness 10 and every test that does not
  specifically look for it — it is audible only in the hall. Aggravated by the fact that for
  `create7point1()` the JUCE enum-bit order coincidentally equals the initializer-list order, so a
  hardcoded 0..7 map *appears correct today*. Mitigated by a three-layer test strategy.
- **R2 (HIGH):** Logic may negotiate 7.1 (SDDS) rather than plain 7.1 —
  `kAudioChannelLayoutTag_Emagic_Default_7_1` (`juce_CoreAudioLayouts_mac.h:117`) maps to the SDDS
  channel-type membership. Mitigated by accepting all three 8-channel containers. Settled at Stage 4.

## Additional Notes

**Concept.** A Logic Pro-native 8-channel spatializer rendering a mono/stereo source to eight
discrete speaker feeds using Distance-Based Amplitude Panning over an irregular, non-flat,
user-measured speaker array. Target venue: Roy Barnett Recital Hall, UBC — 255 seats, deep
rectangular plan, steeply raked seating, speakers mounted high on the side walls.

**Why DBAP and not VBAP.** VBAP normalises speaker positions to unit direction vectors and discards
distance entirely, serving one sweet spot. The Barnett array is three pairs down the side walls plus
an inboard rear pair — non-equidistant and mounted above a rake. DBAP weights every speaker by its
actual distance and shows lower variance across listener positions, which is the metric that matters
for an audience distributed through a hall.

**Transport.** mono/stereo in → `AudioChannelSet::create7point1()` out. 7.1 is used purely as an
8-channel carrier; its L/C/R/LFE semantics are meaningless here and all real geometry lives in the
DSP. Logic exposes only 10 named surround formats and no arbitrary discrete N-channel bus, and
effects (`aufx`) get exactly one output bus.

**Parameters.** 17 musical (automatable, APVTS) + 42 venue values (separate `ValueTree`, never
written by a musical preset). The headline gesture is *spatial orchestration* in the Acousmonium
sense — automating the 8 per-speaker weights to move a sound between speaker subsets at constant
perceived level, since `Σ v_i² = 1` means dropping weights redistributes rather than reduces.

**Relationship to O-Orbit.** Complementary, not redundant. O-Orbit is the general-purpose VBAP
*orbiter* with a motion engine across many surround formats. O-Octagon has no motion engine, one
locked 8-channel transport, and DBAP distance weighting for a specific irregular non-flat rig.
O-Octagon does **not** fork O-Orbit and must **not** link SAF.

**Presets store `blur` (0–1), not metres — and that is deliberate.** The blur radius is resolved
against the venue's `rigScale`, so the same `blur` gives a proportionally different radius on a
differently-sized rig. Factory presets are therefore venue-portable *by construction*: a patch means
the same musical thing in a different hall. On the default venue `rigScale` is 7.9317 m, so
`blur = 0.55` resolves to `r_s = 2.18 m` **there and only there**. If a preset's audible diffusion
changes after a venue edit, that is the design working, not drift.

**A preset never moves the source or the scene.** The six values a factory preset carries are room
character (`width`, `rolloff`, `blur`, `hullAtten`, `airAmount`, `outputGain`). The other eleven —
`srcX`, `srcY`, `srcZ` and the eight weights — are snapshotted and restored around the load
(`oo::presets::loadPreserving`). This is not the same as omitting them from the preset: the shared
preset manager resets *every* parameter to its default before applying anything, so omission alone
would re-centre the source and clear the scene mid-cue.

**Deferred to v1.1+:** VBAP A/B mode; binaural/stereo fold-down; quadraphonic variant; internal
diffuse reverb; motion engine; multiple simultaneous sources.

**Planning artifacts:**
- `.planning/BRIEF.md` — creative brief
- `.planning/REQUIREMENTS.md` — 30 requirements with acceptance criteria and stage traceability
- `.planning/parameter-spec-draft.md` — parameter draft
- `.planning/research/ARCHITECTURE.md` — binding DSP architecture contract
- `.planning/ROADMAP.md` — phased implementation plan
- `.planning/stages/0-ideation/CONTEXT.md` — Stage 0 findings, decisions, risk register

**Related research:**
- `research/logic-pro-multichannel-octaphonic-dbap.md` — the locked architecture
- `research/juce8-multichannel-spatial-audio.md` — `AudioChannelSet` reference, bus negotiation
- `research/spatial-audio-per-grain-spatialization.md` §1 — VBAP math, for the deferred v1.1 mode
