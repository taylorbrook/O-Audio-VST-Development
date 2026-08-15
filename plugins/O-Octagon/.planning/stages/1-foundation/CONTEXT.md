# Stage 1 — Foundation: CONTEXT

**Plugin:** O-Octagon
**Stage:** 1 of 4 — Foundation + Shell
**Phase:** discuss ✓
**Date:** 2026-08-11
**Mode:** manual (interactive)
**Branch:** `feat/o-octagon` (created from `docs/logic-multichannel-dbap-research` @ 12ae50dd)

---

## Goal

A loadable, validating 8-channel shell: correct bus declaration, all 17 APVTS parameters,
session-state round-trip. **No DBAP.** The shell exists to prove the transport before any
geometry depends on it.

## Inherited Contracts — NOT re-opened

These are settled by Stage 0 and by `research/logic-pro-multichannel-octaphonic-dbap.md`.
Stage 1 implements them verbatim.

| Contract | Source | Value |
|---|---|---|
| Build target | ARCHITECTURE §12 | `OuariconOctagon` (folder `plugins/O-Octagon`) |
| Product name | ARCHITECTURE §12 | `O-Octagon${OUARICON_DEV_SUFFIX}` |
| Plugin code | ARCHITECTURE §12 | `OuOc` — verified unused across all 39 plugins |
| Version keyword | ARCHITECTURE §12 | **`VERSION 1.0.0`**, never `PLUGIN_VERSION` |
| Input bus | ARCHITECTURE §4.2 | `mono()`, declared in the ctor member-initialiser list |
| Output bus | ARCHITECTURE §4.2 | `create7point1()` — an 8-channel **carrier**, semantics ignored |
| Accepted outputs | ARCHITECTURE §4.2 | 7.1, 7.1-SDDS, 5.1.2 (real) + mono, stereo (SAFE) |
| Accepted inputs | ARCHITECTURE §4.2 | mono, stereo only |
| Parameters | ARCHITECTURE §6.1 | 17 × `AudioParameterFloat`, **all skews linear** |
| Venue store | ARCHITECTURE §4.1 | separate `VENUE` child of `apvts.state` — **added at Phase 2.1** |
| State round-trip | ARCHITECTURE §4.1 | `copyState()` / `replaceState()` — written once, here |
| Latency | ARCHITECTURE §12 | **zero**; never call `setLatencySamples()` |

**Forbidden in CMakeLists.txt:** `PLUGIN_CHANNEL_CONFIGURATIONS` (counts only, not types —
breaks surround detection). **Forbidden as a dependency:** SAF (unlike sibling O-Orbit).

## Decisions Taken This Phase

### D1 — Shell audio behaviour: dry input to all 8 outputs at unity

`processBlock` clears the buffer, then writes the mono-summed dry input to **every one of the
8 output channels** at unity.

**Why:** a silent shell is indistinguishable from a failed bus negotiation. Writing all 8
proves both that Logic negotiated an 8-channel container *and* that every channel index is
writable — Logic's surround meters show 8 lanes moving. This is the strongest available
Stage 1 evidence for FUNC-01 / COMPAT-01 without any DSP existing.

**Constraint:** this is a **placeholder**, replaced wholesale by the GainStage inner loop at
Phase 2.2. It must not acquire dependants. Mark it with a comment naming Phase 2.2.

**Note:** the write here is by **raw buffer index 0..7**, not through a speaker map. That is
correct only because `ChannelMap` does not exist yet (Phase 2.1). The Phase 2.1 test criterion
*"`grep -rn` confirms zero hardcoded output channel indices outside `ChannelMap`"* is what
retires this loop — it is expected to fail against this placeholder and must not be
grandfathered.

### D2 — Editor: `juce::GenericAudioProcessorEditor`

`hasEditor()` returns true; `createEditor()` returns a `GenericAudioProcessorEditor`.

**Why:** it renders all 17 parameters with their names, ranges, defaults and units, which is
directly the Stage 1 test criterion *"all 17 parameters appear with correct names, ranges,
defaults and units."* It also gives the Standalone build a non-empty window, which is how the
COMPAT-04 stereo-interface (SAFE mode) check gets eyeballed. Five lines, deleted at Phase 3.1.

**Interaction with the render harness (Phase 2.2):** `createEditor` will be guarded with
`#if JUCE_WEB_BROWSER` when the WebView lands. Write the guard's *shape* now is **not**
required — but do not let the generic editor become load-bearing for anything
(`pattern_render_harness_breaks_on_webview_editor`).

### D3 — Branch: `feat/o-octagon`, cut from the docs branch

GSD `branching_strategy` is `none`, which would have kept Stages 1-4 on
`docs/logic-multichannel-dbap-research`. Overridden for this plugin: the branch is cut from
that point so the DBAP research doc and the Stage 0 planning commits ride along, and O-Octagon
gets its own PR rather than turning a docs branch into a four-stage implementation branch.

### D4 — `parameter-spec.md` must be promoted from the draft

`plugins/O-Octagon/.planning/parameter-spec-draft.md` still carries **unresolved** Open
Questions 3, 4 and 5 and the 17-vs-18 discrepancy as *open*. All four were resolved at Stage 0.
The draft is therefore stale in exactly the places Stage 1 reads from.

**Action for the plan phase:** write `plugins/O-Octagon/.planning/parameter-spec.md` as the
final spec, reconciled against **ARCHITECTURE §6.1 (authoritative)** — OQ5 resolved to a
separate `ValueTree`, count resolved to 17, all skews linear. The draft stays as a historical
artifact. The foundation agent reads `parameter-spec.md`, not the draft.

## Non-Goals for Stage 1 — do not build these

Listed because each is cheap to start early and each would be wrong to.

- `ChannelMap` / `rebuildChannelMap()` — Phase 2.1. **One construction site** is a load-bearing
  property; a Stage 1 "temporary" map creates a second.
- The `VENUE` ValueTree and its 42 values — Phase 2.1. The *serialisation code* is written now
  and does not change when the child is added.
- `ConvexHull2D`, `DbapSolver`, `SourceShaper`, `HullProcessor`, `GainStage`, `VerifyPing`.
- The 64-sample control grid and the 17 `SmoothedValue` instances — Phase 2.2.
- The render harness — Phase 2.2, per ARCHITECTURE §14.4.
- Any WebView asset — Phase 3.1.

## Open Risks Carried Into Stage 1

| Risk | Handling at this stage |
|---|---|
| **R2** — Logic may negotiate 7.1-SDDS, not 7.1 | Already mitigated by accepting all three 8-channel containers. Settled empirically at Stage 4. |
| **R1** — channel map (CRITICAL, silent) | Not yet in scope. D1's raw-index placeholder is the one deliberate exception and is scheduled for removal. |
| `juce_add_plugin` `VERSION` keyword | `PLUGIN_VERSION` is silently ignored by JUCE and ships 1.0.0 regardless — latent in O-Marimba / O-MicrotonalSampler / O-Reed. Use `VERSION`. |

## Stage 1 Exit Criteria (from ROADMAP)

- [ ] Builds clean on macOS — VST3, AU, Standalone — zero warnings from
      `juce_recommended_warning_flags`
- [ ] `auval -a | grep -i octagon` lists the AU
- [ ] pluginval strictness 10 passes, VST3 **and** AU, run 2-3× (`pattern_ci_pluginval10_catches_latent_nan`)
- [ ] Standalone opens on a 2-channel interface (SAFE mode) without error — the COMPAT-04 gate
- [ ] All 17 parameters appear in the host automation list with correct names, ranges, defaults, units
- [ ] A parameter change round-trips through save/reload of session state
- [ ] **FUNC-01, COMPAT-01, COMPAT-04 verified**

## Verified Before Planning

- `create7point1()` — `juce_AudioChannelSet.h:199`
- `create7point1SDDS()` — `juce_AudioChannelSet.h:209`
- `create5point1point2()` — `juce_AudioChannelSet.h:221`
- `getTypeOfChannel()` — `juce_AudioChannelSet.h:577`
- `getChannelIndexForType()` — `juce_AudioChannelSet.h:581`
- Root `CMakeLists.txt:48` auto-discovers `plugins/*` via `file(GLOB ...)` — a new
  `plugins/O-Octagon/CMakeLists.txt` is picked up on re-configure, no root edit needed.

## References

- `plugins/O-Octagon/.planning/research/ARCHITECTURE.md` §4.1, §4.2, §6.1, §12, §14
- `plugins/O-Octagon/.planning/ROADMAP.md` — Stage 1
- `plugins/O-Octagon/.planning/BRIEF.md` — Locked Architecture 1-6
- `plugins/O-Octagon/.planning/REQUIREMENTS.md` — FUNC-01, COMPAT-01, COMPAT-04
- `troubleshooting/patterns/juce8-critical-patterns.md` §4
- Reference plugin: `plugins/O-Orbit/` — `Source/{Data,DSP}/` layout, permissive
  `isBusesLayoutSupported()`. **Do not** take the motion engine, VBAP, or SAF.
