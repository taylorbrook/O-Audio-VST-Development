/*
  ==============================================================================

    HyperbolicFriction.h
    O-Bowed - Memoryless Hyperbolic Friction Model
    Ouaricon Audio
    Developer: Taylor Brook

    STK-style memoryless friction curve. Computes waveguide reflection
    coefficient from differential velocity and bow force. O(1) per sample,
    no iteration required. Always stable (rho bounded in [0, ~0.5]).

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
    float mu_s = 0.8f;   // static friction coefficient
    float mu_d = 0.3f;   // dynamic friction coefficient
    float v_0  = 0.05f;  // characteristic velocity (from ROSIN)
    float R_s  = 0.5f;   // string wave impedance (fixed for Phase 3.1)
};
