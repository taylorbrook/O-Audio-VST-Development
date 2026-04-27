# bow-friction

Shared friction-junction + bow-envelope module for Ouaricon digital-waveguide
bowed-string plugins (O-Bowed, O-Contrabass).

Extracted from O-Bowed in Phase 2.1b. Pure value-class C++; no JUCE patch
required, no per-format routing, no allocations on the audio thread.

## Public API

### `HyperbolicFriction` (header-only)

STK-style memoryless friction curve. Computes waveguide reflection coefficient
from differential velocity and bow force. O(1) per sample, always stable
(rho bounded in [0, ~0.5]).

```cpp
class HyperbolicFriction
{
public:
    float computeReflectionCoefficient (float v_delta, float F_bow) const noexcept;

    void setRosin (float rosinParam) noexcept;
    void setStringImpedance (float impedance) noexcept;

    // Phase 2.1b — bass-string consumer hook (RESEARCH §13.3-Q5).
    void setStaticFrictionCoefficient  (float mu) noexcept;
    void setDynamicFrictionCoefficient (float mu) noexcept;
};
```

### `BowModel` (header + cpp)

Generates bow velocity (`v_bow`) and bow force (`F_bow`) signals from MIDI
input and parameter values. One-pole envelope smoothing for click-free
attack/release. Velocity-dependent attack time (5–50 ms).

```cpp
class BowModel
{
public:
    void prepare (double sampleRate);
    void startBow (float velocity);
    void stopBow();
    void reset();
    void updateEnvelope();
    void setBowSpeed (float speed);
    void setBowPressure (float pressure);

    float getBowVelocity() const noexcept;
    float getBowForce()    const noexcept;
    bool  isActive()       const noexcept;
};
```

## Default-coefficient ladder

| Coefficient | Treble (O-Bowed) | Bass (O-Contrabass) |
|-------------|------------------|---------------------|
| `mu_s`      | 0.8 (init)       | 0.85 (setter)       |
| `mu_d`      | 0.3 (init)       | 0.25 (setter)       |
| `v_0`       | 0.05 (init)      | 0.05 (init, unchanged) |
| `R_s`       | 0.5 (init)       | 0.5 (init, unchanged)  |

Treble consumers (O-Bowed) use the module's init defaults verbatim — no
setter calls in `prepareToPlay`. Bass consumers (O-Contrabass) call
`setStaticFrictionCoefficient(0.85f)` and `setDynamicFrictionCoefficient(0.25f)`
in `prepareToPlay`, after `bowModel.prepare(...)` and `waveguideString.prepare(...)`
but before any audio activity.

## Setter contract

All setters are `noexcept` and write a single member float. Safe to call from:

- `prepareToPlay` (preferred for one-time configuration)
- Per-block APVTS-driven update paths (e.g. `setRosin` reads APVTS each block)

Do NOT call from per-sample inner loops — none of the setters allocate, but
they are not optimised for sample-rate invocation.

## Consumer call-pattern example

```cpp
// In OContrabassAudioProcessor::prepareToPlay (after voice prep):
voice->frictionModel.setStaticFrictionCoefficient (0.85f);
voice->frictionModel.setDynamicFrictionCoefficient (0.25f);
```

## Design rationale

See `plugins/O-Contrabass/.planning/stages/2-dsp/RESEARCH.md` §13.3 for the
rationale behind the setter API choice (vs. constructor template parameter,
vs. per-plugin coefficient header). The setter approach preserves the
treble-default contract bit-exactly for O-Bowed while allowing bass-string
plugins to opt into different coefficients without forking the module.

## Status

- v1.0.0 (2026-04-27) — initial extraction, 2 consumers (O-Bowed, O-Contrabass).
- v1.1.0 (planned, Phase 2.3) — `SchellengGuard` class for bow-pressure clamping.
- v1.x (deferred) — elasto-plastic friction, thermal-coupled friction, alternative
  coefficient ladders.
