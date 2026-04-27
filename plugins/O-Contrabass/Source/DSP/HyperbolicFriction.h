/*
  ==============================================================================

    HyperbolicFriction.h
    O-Contrabass - Memoryless Hyperbolic Friction Model (bass-tuned)
    Ouaricon Audio
    Developer: Taylor Brook

    Inline copy of O-Bowed/Source/DSP/HyperbolicFriction.h with bass-tuned
    member-init defaults (mu_s = 0.85, mu_d = 0.25, v_0 = 0.05). All other
    behaviour — including setRosin's v_0 = 0.1 * exp(-4.6 * rosinParam)
    formula — is preserved verbatim. Promotion to the shared
    modules/synthesis/bow-friction module is Phase 2.1b's job.

  ==============================================================================
*/

#pragma once
#include <cmath>

class HyperbolicFriction
{
public:
    // Compute reflection coefficient from differential velocity and bow force.
    // v_delta = v_bow - v_string_incoming (already computed by caller)
    // F_bow   = current bow force from BowModel envelope
    // Returns rho in [0, ~0.5] -- bounded, always stable.
    float computeReflectionCoefficient (float v_delta, float F_bow) const noexcept
    {
        float absV = std::abs (v_delta);
        float mu = mu_d + (mu_s - mu_d) * v_0 / (v_0 + absV);

        // Convert friction coefficient to waveguide reflection coefficient
        // r = (1/4) * mu * F_bow / R_s
        float r = 0.25f * mu * F_bow / R_s;
        float rho = r / (1.0f + r);

        return rho;
    }

    // ROSIN 0.0 = smooth (v_0 = 0.1), ROSIN 1.0 = aggressive (v_0 = 0.01)
    void setRosin (float rosinParam) noexcept
    {
        v_0 = 0.1f * std::exp (-4.6f * rosinParam);
    }

    void setStringImpedance (float impedance) noexcept
    {
        R_s = impedance;
    }

private:
    // Bass-tuned defaults (RESEARCH.md §1.2 — three init-list edits vs O-Bowed)
    float mu_s = 0.85f;  // static friction coefficient (was 0.8 on treble)
    float mu_d = 0.25f;  // dynamic friction coefficient (was 0.3 on treble)
    float v_0  = 0.05f;  // characteristic velocity (modulated by ROSIN)
    float R_s  = 0.5f;   // string wave impedance (fixed for Phase 2.1)
};
