# O-simpleWavetable — Stage 0 Context (Discuss Findings)

**Date:** 2026-10-05
**Stage:** 0 (Research & Planning) — complete

This is the narrative companion to `research/ARCHITECTURE.md` and `ROADMAP.md`. It records the decisions, constraints and evidence behind them.

## What this plugin is

The multi-frame successor to O-simpleAdditive's two-frame A→B morph. It is a single wavetable oscillator whose five formula banks each isolate one lesson, plus an Imported bank. The design rule throughout: **the panels must show exactly what is heard**, and each "Off" switch must teach its own concept, not introduce an unrelated artifact.

## Constraints carried in

- Contracts: BRIEF.md (authoritative), parameter-spec-draft.md (22 params), REQUIREMENTS.md (30 reqs). No mockup yet.
- Suite conventions: juce::Synthesiser + 16 custom voices, dual ADSR, block param push, lead-voice display snapshot, WebView + WebView2 static, en/fr/zh-Hans tooltips, factory presets never touch output level.
- Shared checkout: Stage 0 writes only under `plugins/O-simpleWavetable/.planning/`. The orchestrator commits.

## Evidence gathered

- **Oscillator model (numpy):** with strict per-octave mips and a linear read, the worst alias is −74 dB (A0) and −121 dB at C8. A Prism-style blend toward the richer level gives −15.6 dB at C8, so it fails QUAL-02. Cubic interpolation helps only below C3. Decision: linear + strict selection.
- **Drive sweep:** `g = 0.25·100^((k−1)/31)` gives a near-sine frame 1 (h3 −46 dB) and strictly monotonic h3/h5/h7/THD.
- **In-repo precedents:** O-Prism already has the 2048 × 256 × 10-level layout, the FFT mip builder, a hardened importer and the REG-01 generation reaper. They are reused, with two fixes (Nyquist bin; no trilinear level blend). O-simpleGrain supplies the import UX (FileChooser + webview-drop-streaming). Its state stores a path; this plugin must embed the frames instead.
- **JUCE 8.0.15** APIs verified in local source / Context7 (FFT inverse scaling, FlacAudioFormat writer, PositionInfo optionals, ThreadPool, base64, GZIP).

## Key decisions (see ARCHITECTURE.md for the full table)

1. Linear in-cycle read; 11 mip levels; strict `ceil(log2(f·2048/fs))` level; level changes are crossfaded, not blended.
2. Interpolation Off latches the frame at each cycle wrap, so it teaches stepping rather than clicks.
3. One per-voice **frozen-cycle 5 ms crossfade** handles bank switch, import swap, the band-limit and interpolation toggles, and mip-level changes. Voices never hold retired banks.
4. Built-in banks are shared process-wide and immutable. The imported bank uses atomic publish + the Prism reaper.
5. Import: channel mean → 2048-sample slices (≤ 256, tail dropped, < 2048 samples rejected) → DC removal → per-frame normalize (+24 dB cap) → int16 canonical → FLAC16 base64 in an `IMPORTED_BANK` state child. Reload is bit-identical.
6. Sine→Square reaches h31 with a fractional frontier harmonic. Formant uses A-E-I-O-U (Peterson & Barney) at f_ref 110 Hz, so the formants sit inside the 1–32 graph. Pulse duty goes from 50% to 3.125%.
7. Mid-rise quantizer (3 bits = 8 levels), Full = bypass, 15-step choice.
8. Separate `lfo_div` (16 divisions, default 1/1); PPQ-locked tempo sync; deterministic S&H.
9. The harmonics and cycle panels are rendered on the message thread from the immutable bank plus lead-voice atomics, with an exact 2048-point FFT and no window.

## Open questions for the user (non-blocking; defaults chosen)

1. Imported selected with nothing imported: **silence + UI prompt** (default) or fall back to a built-in bank?
2. Keep the added ±2 st pitch bend?
3. Accept mid-rise (no zero level) over the sibling's mid-tread?
4. `lfo_div` default 1/1 (instead of 1/4)?
5. Per-frame (capped) normalization of imports, or bank-wide normalization?
6. Reject imports shorter than 2048 samples (AKWF-style single cycles won't load)?

## Strategy

Complexity 5.0 (capped) → phased. Stage 2: 2.1 banks (offline gates) → 2.2 voice/oscillator (QUAL-02 gate) → 2.3 modulation → 2.4 swap/import/persistence (HIGH risk; fallback is a preallocated single slot with fade-to-silence). Stage 3: layout → visual panels → import UX/i18n.

## Verification notes

Web search worked once this session (KVR, Electric Druid, JUCE forum). The vowel formant table and the EarLevel and J.O. Smith references come from established literature and were not re-verified online.
