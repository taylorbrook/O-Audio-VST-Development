# O-Strata — Stage 0 Context (Discuss phase findings)

**Date:** 2026-09-07
**Agent:** research-planning-agent
**Outputs:** `research/ARCHITECTURE.md`, `ROADMAP.md`, `STATUS.md` (this directory's parent)

## What Stage 0 did, and deliberately did not do

Research was already at Level 3 with prototypes (`research/wavetable-synthesis-3d-geometry.md` §7, three prototype directories). Stage 0 therefore **consumed** it and spent its effort on what the research left open: the integration design around O-Prism's existing swap/reaper, the threading contract, persistence, the two open decisions from ideation, and requirement traceability.

**Not re-researched, and why:**
- Geometry maths (unwrap modes, loop policy, alignment, normalisation, C++ carry-overs) — §7.1 is prototype-verified on five meshes with numbers; ARCHITECTURE cites it verbatim.
- Competitive landscape — §5 is a September 2026 survey; no WebSearch performed.
- 3D view transport and WebView gotchas — §7.3 has a working prototype and sourced bug references.
- The inherited engine — O-Prism ARCHITECTURE.md is referenced by section name; nothing re-documented.
- JUCE APIs — verified against the local 8.0.14 module headers (Context7 `get-library-docs` is not in this toolset); all classes named in ARCHITECTURE exist; `juce_cryptography` is a new module link.

## Decisions taken

1. **Embedded-import cap: 2 MB per source after compression** (gzip for OBJ/STL, raw for PNG). Rationale: 2 MB gzip ≈ 65–80 k triangles, which is exactly the class of mesh that bakes in ≤ ~150 ms — everything that embeds also re-bakes fast on load; PNGs up to ~1024² embed; 1 M-triangle scans link by path + SHA-256. Worst case ~5.4 MB of base64 XML in state, in line with sampler precedent. (ARCHITECTURE Decision 1; FUNC-08, QUAL-03.)
2. **Bake parameters are APVTS `AudioParameter*`** — automatable, persisted, relay-generated from `allSliderIds()`, excluded from mod-matrix destinations. Re-bake storms are handled by the scheduler (message-thread 50 ms poll of a BakeKey hash, 150 ms debounce, one in-flight job per oscillator with cancellation, **no APVTS listener** so audio-thread automation callbacks cannot run bake code). (ARCHITECTURE Decision 2; FUNC-01, PERF-01.)
3. **Baked pipeline only; zero new audio-thread code.** Oscillator read path byte-identical to O-Prism (DSP-01). Live terrain oscillator = v1.1.
4. **Built-in libraries generated procedurally in C++**, not shipped as OBJ binary data — the golden test needs the Python parametrisation, and it sidesteps the hyphen-stripping and dual-`BinaryData`-namespace traps. Terrains are formulas.
5. **Delete the wavetable library code paths** (`WavetableFactory`, `UserWavetableManager`, `WavetableImporter`, `WavetableEditor`, 14 native functions, `oscATable/oscBTable`, their i18n rows). Keep `WavetableGenerator` for mipmaps and the sine placeholder that plays before the first bake completes (never silence).
6. **Geometry parsing runs in the bake job** (background pool thread "O-Strata Bake"), asserted off the audio thread; a 1 M-triangle OBJ parse would otherwise freeze the UI for ~1 s.
7. **Event push, not `evaluateJavascript`** for playhead/view/progress; O-Prism's held-notes push is converted too (JUCE #1415).
8. **Third open item from STATUS ("WKWebView/WebView2 frame-time gate")** stays a Stage 3.2 measurement with a defined method (in-page `performance.now()`, 300 frames, DPR 2, both hosts) and a defined fallback (Canvas 2D default if > 2 ms).

## Constraints carried into implementation

- APVTS layout is static: all 46 geometry params always exist; only the active family is hashed into the BakeKey, so inactive-family knobs never bake.
- `prepareToPlay` never publishes → the O-simpleSampler lock pattern is not needed; publish is message-thread only (`JUCE_ASSERT_MESSAGE_THREAD`). Retired tables use O-Prism's two-generation reaper.
- Bake determinism (single thread per table, canonical frame-0 start) is what makes FUNC-08's SHA-256 check meaningful.
- No unit-test framework, CI runs no tests: Stage 2 gates are the offline bake/render harnesses; `createEditor` is `JUCE_WEB_BROWSER`-guarded from Stage 1 so the harness survives Stage 3.
- fr strings go through the glossary + lint gate; zh-Hans through the rollout rules.
- Licence hygiene: Terrain (GPL-3.0) formulas re-derived; no code copied.

## Wording notes for REQUIREMENTS.md (not edited)

- FUNC-05 "embedded as binary data" → satisfied by procedural generation compiled into the binary.
- FUNC-06 "parse on the message thread" → implemented as "parse off the audio thread, in the bake job"; the thread-name assert and the ≤ 2 s budget are honoured.
- PERF-02's 100 ms is the generator + conditioning figure; the inherited mipmap stage (~90 ms at 256 frames) is reported alongside in the harness.

## Complexity

Raw 23.0 (params 2.0 cap + 16 algorithms + 5 features) → capped 5.0. Staged: Stage 1 (1), Stage 2 (5 phases), Stage 3 (3 phases), Stage 4 (2 phases).

## Before Stage 1 can run

- UI mockup (`/start O-Strata` → option 3) and the full `parameter-spec.md` (the draft has the IDs; the mockup finalises names/order). The 0-ideation → 1-foundation gate needs `--force` (memory: `pattern_gate_0_to_1_always_needs_force`).
