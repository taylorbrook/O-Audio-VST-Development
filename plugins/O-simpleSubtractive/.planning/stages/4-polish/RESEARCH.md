# Stage 4 (Polish) — RESEARCH

**Source:** Express mode, non-interactive. Synthesized from the validated Stage 2 DSP, the Stage 3 bridge, parameter ranges (`createParameterLayout`), and the `O-simpleGrain` preset pattern.

## 1. Preset write API (reuse, don't reinvent)

`O-simpleGrain::applyFactoryPreset` is the template. Pattern:

```cpp
void applyFactoryPreset (const juce::String& name) {
    using namespace OSimpleSubtractive::ParamIDs;
    for (auto* p : getParameters())                    // 1. clean slate
        p->setValueNotifyingHost (p->getDefaultValue());
    auto setReal   = [this](const char* id, float v){ if (auto* p = parameters.getParameter(id)) p->setValueNotifyingHost(p->convertTo0to1(v)); };
    auto setChoice = [this](const char* id, int i)  { if (auto* p = parameters.getParameter(id)) p->setValueNotifyingHost(p->convertTo0to1((float)i)); };
    if (name == "Saw Sweep") { ... } else if (...) { ... }
}
```

- APVTS member is `parameters` (not `apvts`).
- `convertTo0to1(real)` handles the skewed ranges (cutoff log skew 0.25, ADSR skew 0.35, glide skew 0.5) — always pass **real** values, never normalised.
- `setValueNotifyingHost` triggers the relays → WebView attachments → UI updates automatically.

## 2. Parameter ranges (from `createParameterLayout`)

| ID | Type | Range / choices | Default |
|----|------|-----------------|---------|
| `oscWave` | choice | 0 Saw · 1 Square · 2 Triangle · 3 Sine | Saw |
| `subLevel` | float | 0–1 | 0.0 |
| `noiseLevel` | float | 0–1 | 0.0 |
| `filterType` | choice | 0 LP · 1 HP · 2 BP · 3 Notch | LP |
| `filterSlope` | choice | 0 = 6 · 1 = 12 · 2 = 24 dB/oct | 24 dB |
| `cutoff` | float | 20–20000 Hz (skew 0.25) | 2000 |
| `resonance` | float | 0–1 | 0.10 |
| `filterEnvAmount` | float | −1…+1 (octaves, bipolar) | +0.5 |
| `keyTrack` | float | 0–1 | 0.0 |
| `filterAttack/Decay/Release` | float | 0–5 s (skew 0.35) | 0.005 / 0.30 / 0.20 |
| `filterSustain` | float | 0–1 | 0.40 |
| `ampAttack/Decay/Release` | float | 0–5 s (skew 0.35) | 0.005 / 0.30 / 0.10 |
| `ampSustain` | float | 0–1 | 0.80 |
| `voiceMode` | choice | 0 Poly · 1 Mono · 2 Legato | Poly |
| `glide` | float | 0–1 s (skew 0.5) | 0.0 |
| `outputLevel` | float | −60…0 dB | 0.0 |

## 3. The 8 concept snapshots (each isolates ONE idea)

Reset-to-default first; only the listed params are overridden. Reference note 60 (C4 ≈ 261.6 Hz).

| Preset (data-key) | Concept | Key overrides |
|-------------------|---------|---------------|
| **Saw Sweep** | the literal subtraction: saw → LP, watch harmonics fall | osc Saw; LP 24 dB; cutoff 600; res 0.25; filterEnvAmt +0.7; filterA 0.4 / D 1.5 / S 0.5 / R 0.6; ampA 0.02 / S 0.9 / R 0.4 |
| **Pluck** | fast filter env → percussive bright-then-dark | osc Saw; LP 24 dB; cutoff 400; res 0.35; filterEnvAmt +0.85; filterA 0.002 / D 0.18 / S 0.0 / R 0.15; ampA 0.002 / D 0.35 / S 0.0 / R 0.18 |
| **Brass Stab** | positive env opens cutoff with attack & holds; key-track | osc Saw; LP 24 dB; cutoff 700; res 0.2; filterEnvAmt +0.6; keyTrack 0.3; filterA 0.04 / D 0.25 / S 0.7 / R 0.2; ampA 0.02 / D 0.2 / S 0.75 / R 0.15 |
| **Sweep Pad** | slow amp swell + long deep filter sweep | osc Saw; LP 24 dB; cutoff 350; res 0.18; filterEnvAmt +0.8; filterA 1.8 / D 2.5 / S 0.6 / R 1.5; ampA 1.2 / D 1.0 / S 0.85 / R 1.4 |
| **Acid Bass** | high res + snappy env on saw → 24 dB LP; mono + glide | osc Saw; LP 24 dB; cutoff 300; res 0.78; filterEnvAmt +0.85; keyTrack 0.15; filterA 0.003 / D 0.22 / S 0.1 / R 0.15; ampA 0.003 / D 0.4 / S 0.7 / R 0.12; voiceMode Mono; glide 0.06 |
| **Square Bass** | hollow odd-harmonic square + sub for weight; mono | osc Square; subLevel 0.5; LP 24 dB; cutoff 500; res 0.2; filterEnvAmt +0.4; filterA 0.005 / D 0.25 / S 0.3 / R 0.15; ampA 0.004 / D 0.3 / S 0.85 / R 0.12; voiceMode Mono |
| **Noise Wind** | open filter on white noise → wind | osc Sine; noiseLevel 1.0; BP; cutoff 1500; res 0.45; filterEnvAmt +0.2; keyTrack 0.5; filterA 0.8 / D 1.2 / S 0.7 / R 1.0; ampA 0.6 / D 0.8 / S 0.8 / R 0.9 |
| **Self-Oscillation** | filter rings into a pure sine; plays in tune | osc Sine; LP 24 dB; cutoff 261.6; res 1.0; filterEnvAmt 0.0; keyTrack 1.0; filterS 1.0; ampA 0.05 / D 0.2 / S 0.9 / R 0.4 |

**Why these values:**
- **Self-Oscillation** in tune: with `keyTrack=1.0` and `cutoff=261.6` (= C4 = reference note 60), `fcEff` at note N = `261.6·2^((N−60)/12)` = the note's own pitch → the resonant tone tracks the keyboard chromatically. `filterEnvAmount=0` keeps it a steady tone. Validated Stage-2 path (soft-knee limiter + negative-k bias bounds the self-osc).
- **Noise Wind**: the un-silenceable Sine fundamental (≤ C5) sits well below the 1500 Hz band-pass centre and is rejected, so the audible band is filtered noise. `keyTrack 0.5` lets the wind band move as you sweep notes.
- **Acid Bass / Square Bass** use Mono so the legato/glide and single-voice bass behaviour (FUNC-05) is shown; Acid adds a short `glide` for the 303 slide.
- All LP presets use **24 dB/oct** (the workhorse, default, and most-validated slope).

## 4. FUNC-07 playability

The factory default state (cutoff 2 kHz, res 0.10, filterEnvAmt +0.5, amp S 0.8) is already a usable bright poly patch (Stage-2 verified, no clicks/denormals). FUNC-07 is satisfied by construction once the preset tour gives ready-made bass/lead/pluck/pad starting points; no parameter-default changes are needed. Confirm by ear in UAT.

## 5. Pitfalls (from memory + sibling lessons)
- **Pass real values to `convertTo0to1`**, never pre-normalised — skewed ranges will be wrong otherwise.
- Keep `data-preset` keys **identical** between `index.html`, the `LESSONS` map, and the C++ `if (name == …)` chain — a mismatch silently no-ops the button (WebView→C++ bridge gap fails silently; cf. memory `pattern_webview_native_fn_bridge_gap`).
- Tour row already uses `flex-wrap` — 8 buttons reflow cleanly, no CSS change required.
- No new native function and no new binary-data target → no auval/pluginval surface change beyond the snapshot bodies.
