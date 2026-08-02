# Built-in Source Sample Provenance — O-simpleSampler

Source: **procedurally synthesized in-house** by `plugins/O-simpleGrain/tools/generate_samples.py`
Generator seed: `SEED = 20260624` (deterministic)
Dependencies to re-run: Python 3, `numpy`, `scipy`
License: **AGPL-3.0-or-later**, together with the rest of this repository
Copyright: © 2026 Ouaricon Audio

Files (with the MD5 recorded 2026-08-01):

| File | MD5 | Bytes |
|---|---|---|
| `piano.wav` | `68efbe27e85979e723e1d777c2209e46` | 396944 |

Format: WAVE PCM, 24-bit, **mono**, 44 100 Hz, peak-normalised to −1 dBFS.
Recorded-pitch root: MIDI **48** (probed f0 ≈ 131.6 Hz via YIN → nearest MIDI note).

Use: this is the plugin's single embedded built-in source, decoded from `BinaryData` and
resampled to the engine rate off the audio thread. It is what a fresh instance plays with
no user action. `Load…` and drag-and-drop bring in any other sound at runtime; nothing
loaded that way is stored in the repository or in the binary.

**Provenance is by identity, not by re-derivation.** `piano.wav` here is **byte-identical**
to `plugins/O-simpleGrain/Source/samples/piano.wav` — identical MD5
(`68efbe27e85979e723e1d777c2209e46`) and identical byte count. It is therefore the same
procedurally generated asset, carrying the same generator, the same seed, and the same
self-authored origin. See `plugins/O-simpleGrain/Source/samples/LICENSE.md` for the full
generator record.

**Reproducibility.** Re-running the O-simpleGrain generator reproduces this file
bit-identically (verified 2026-08-01, MD5 match):

```bash
python3 plugins/O-simpleGrain/tools/generate_samples.py
md5 -q plugins/O-simpleGrain/Source/samples/piano.wav   # == the MD5 above
md5 -q plugins/O-simpleSampler/Source/samples/piano.wav # == the MD5 above
```

**No third-party content.** Nothing in this folder was sampled, recorded, or derived from
any commercial sample library or third-party recording. This document is provenance
evidence, not a third-party notice.

---

## Removed in v1.1.0 (2026-08-01)

Three further files lived in this folder through v1.0.0 and have been **withdrawn**:
`cello.aif`, `pizz.aif`, and `hit.wav`. They originated in a **commercial sample library**
and their redistribution rights were never established, so they could not remain embedded
in a plugin binary from a repository that is about to be published.

They were **removed rather than replaced** — substituting generated or CC0 audio was
considered and deliberately declined — and the source-selector parameter was dropped with
them. **No release of O-simpleSampler ever shipped them**: the plugin has zero published
releases, so no signed or notarised artefact ever contained them. They existed only in
this repository.

Recording this here is deliberate. A provenance document that silently omits what used to
sit beside these files would be a worse record than one that says so plainly.

**They do remain in git history**, at commit `4ca27977` (2026-07-02). A `git rm` in a later
commit does not remove them from earlier ones. Whether to rewrite history to expunge them
is an open decision — see `PUBLIC-RELEASE-READINESS.md` §2.2 and §4.6.

Full detail: `.planning/quick/260801-u3o-remove-commercial-library-samples-from-o/`.
