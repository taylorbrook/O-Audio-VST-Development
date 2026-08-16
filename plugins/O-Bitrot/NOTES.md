# O-Bitrot Notes

## Status
- **Current Status:** 💡 Ideated
- **Version:** N/A
- **Type:** Audio Effect (Broken-Media Degradation)

## Lifecycle Timeline

- **2026-08-14:** Ideation complete — creative brief and requirements created from `research/glitch-effects/` (concept 2). Six degradation families (tape/CD/vinyl/packet/codec/crush), per-module controls, tempo-synced stochastic clock, seeded randomness.

## Third-Party Code

### libgsm 1.0.22 (GSM 06.10 full-rate codec)

- **Vendored:** 2026-08-15 into `third_party/libgsm/{src,inc}` + `COPYRIGHT`,
  from the canonical upstream tarball at <https://www.quut.com/gsm/>
  (gsm-1.0-pl22, Jutta Degener & Carsten Bormann, TU Berlin). 18 library
  `.c` files + 5 headers; the `toast` utilities are not vendored.
- **License:** TU-Berlin permissive (ISC-style; the 2009 addendum grants
  "Permission to use, copy, modify, and distribute this software for any
  purpose with or without fee ... provided that this notice is not removed"
  with no warranty). Full text verbatim in `third_party/libgsm/COPYRIGHT`.
  Compatible with this repository's AGPL-3.0 license (permissive → AGPL).
- **Provenance / GPL firewall:** vendored directly from upstream, NOT from
  RSBrokenMedia (GPL-3.0), which remains a patterns-only reference.
- **Build:** compiled as the `OBitrot_gsm` STATIC library with definitions
  `SASR NDEBUG NeedFunctionPrototypes=1` (never `WAV49`/`FAST`), warnings
  suppressed PRIVATE, PIC on; linked PRIVATE into `OBitrot`.

## Latency Scheme (Stage 2)

Constant reported latency of 20 ms in ALL modes:
`setLatencySamples(ceil(0.020 * fs))` once in `prepareToPlay`. CodecStage
owns the alignment: a hand-rolled integer delay of exactly that figure when
the codec is disabled or in Mu-law mode; the GSM 160-frame chain replaces it
structurally in GSM mode (one 8 kHz frame = 0.020 s). `DryWetMixer::
setWetLatency` aligns the dry path. The first 20 ms of GSM output after
engagement is silence (output frame primed with zeros; the 10 ms
CODEC_ENABLE fade covers it). At non-integer `0.020*fs` host rates the GSM
path misaligns by < 1 sample versus the plain delay — accepted.

## Known Issues

None
