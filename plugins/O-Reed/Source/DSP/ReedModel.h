/*
  ==============================================================================

    ReedModel.h
    O-Reed - Physical Modeling Reed Wind Synthesizer
    Mass-spring-damper reed ODE with Bernoulli flow junction
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include <cmath>
#include <algorithm>

struct ReedParams
{
    float mu_r       = 0.018f;    // Reed mass per unit area (kg/m^2)
    float g_r        = 3000.0f;   // Reed damping (s^-1)
    float k_r        = 8e6f;      // Reed stiffness (N/m^3)
    float H          = 0.0004f;   // Rest opening (m)
    float embouchure = 0.4f;      // Embouchure parameter (0-1)
    float w_reed     = 0.01f;     // Reed width (m) -- clarinet default
    float rho_air    = 1.2f;      // Air density (kg/m^3)
    float psi        = 0.0f;      // Guillemain Psi confinement (0-0.8), 0=single reed, 0.8=double reed
    float S_reed     = 1.54e-4f;  // Bore cross-section at throat (m^2), for Psi normalization
};

struct ReedState
{
    float x     = 0.0f;  // Reed displacement (m), negative = closing
    float x_dot = 0.0f;  // Reed velocity (m/s)
};

class ReedModel
{
public:
    ReedModel() = default;

    void prepare(double sampleRate)
    {
        dt = 1.0f / static_cast<float>(sampleRate);
        reset();
    }

    void updateParams(float breathPressure, float embouchure,
                      float reedHardness, float reedOpening,
                      float reedMass, float reedDamping,
                      float boreDiameter, float doubleReed = 0.0f)
    {
        // Map APVTS [0-1] to physical units
        // REED_MASS 0-1 -> mu_r 1e-4 to 0.06 kg/m^2
        params.mu_r = 1e-4f + reedMass * (0.06f - 1e-4f);

        // REED_DAMPING 0-1 -> g_r 500 to 6000 s^-1
        params.g_r = 500.0f + reedDamping * 5500.0f;

        // REED_HARDNESS 0-1 -> k_r 2e6 to 20e6 N/m^3
        params.k_r = 2e6f + reedHardness * 18e6f;

        // REED_OPENING 0-1 -> H 0.1mm to 1.5mm (in meters)
        params.H = 0.0001f + reedOpening * 0.0014f;

        // Store embouchure (0-1)
        params.embouchure = embouchure;

        // BORE_DIAMETER 0-1 -> throat_radius 2mm to 20mm (in meters)
        float throatRadius = 0.002f + boreDiameter * 0.018f;

        // Reed area and impedance
        params.w_reed = 0.01f;  // fixed width
        float A_bore = 3.14159265f * throatRadius * throatRadius;

        // Characteristic impedance: Z_c = rho * c / A_bore
        // c_sound = 343 m/s
        cachedZc = params.rho_air * 343.0f / A_bore;

        // Breath pressure: 0-1 -> 0 to 12000 Pa (max ~120 cmH2O)
        cachedBreathPressure = breathPressure * 12000.0f;

        // Guillemain Psi confinement: DOUBLE_REED 0-1 -> psi 0-0.8
        params.psi = doubleReed * 0.8f;

        // S_reed = bore cross-section at throat (for Psi normalization)
        params.S_reed = A_bore;
    }

    // Returns u_reed (volume flow through reed channel, m^3/s)
    // p_mouth: mouth pressure (Pa)
    // p_bore_minus: wave arriving at reed from bore (Pa)
    // Flow-to-wave conversion (Z_c * u_reed + p_bore_minus) done by caller
    float processSample(float p_mouth, float p_bore_minus)
    {
        // Effective parameters with embouchure modifiers
        float emb = params.embouchure;
        float g_eff = params.g_r + emb * 4000.0f;
        float k_eff = params.k_r + emb * 5e6f;
        float H_eff = std::max(params.H - emb * 0.0003f, 1e-5f);

        // Pressure difference across reed
        float dp = p_mouth - p_bore_minus;

        float x_min = -(H_eff + 1e-4f);

        if (params.mu_r < 1e-4f)
        {
            // Static reed fallback: memoryless spring
            // Negative dp: positive pressure closes reed (standard reed physics)
            state.x = -dp / k_eff;
            state.x_dot = 0.0f;
            state.x = std::max(state.x, x_min);
        }
        else
        {
            // Semi-implicit Euler: implicit damping prevents instability
            // when damping rate (g/mu ~270kHz) >> sample rate (~88kHz)
            // All terms per-unit-area: dp (Pa), g_eff (s^-1), k_eff (N/m^3), mu_r (kg/m^2)
            float mu_safe = std::max(params.mu_r, 1e-6f);
            float accel = (-dp - k_eff * state.x) / mu_safe;
            // Implicit damping: unconditionally stable for any g_eff/mu ratio
            state.x_dot = (state.x_dot + accel * dt) / (1.0f + (g_eff / mu_safe) * dt);

            // Clamp velocity to prevent runaway under extreme params
            state.x_dot = std::max(-1000.0f, std::min(state.x_dot, 1000.0f));

            state.x += state.x_dot * dt;

            // Reed closure clamp with velocity zeroing
            if (state.x < x_min)
            {
                state.x = x_min;
                if (state.x_dot < 0.0f)
                    state.x_dot = 0.0f;
            }
        }

        // Reed channel opening area
        float opening = state.x + H_eff;

        // Physical behavior: without mouth pressure, reed rests closed against
        // mouthpiece. Gates the opening by mouth pressure to prevent Bernoulli
        // self-excitation through the rest opening at zero breath.
        float gateThreshold = std::max(0.01f * k_eff * H_eff, 1.0f);
        float mouthGate = std::min(std::abs(p_mouth) / gateThreshold, 1.0f);
        opening *= mouthGate;

        // No soft minimum: the old 0.5% leakage continuously pumped flow into the
        // bore even during reed closure, driving bore pressure to 40-60 kPa (vs
        // intended ~3 kPa). Turbulence noise in the voice loop seeds startup instead.
        float S_opening = params.w_reed * std::max(opening, 0.0f);

        // Guillemain Psi confinement for double-reed morphing
        // psi_denom = 1 + psi * (S_opening / S_reed)^2
        // At psi=0 (single reed): psi_denom=1, identical to Phase 3.1
        float psi_denom = 1.0f;
        if (params.psi > 0.0f && S_opening > 0.0f)
        {
            float ratio = S_opening / params.S_reed;
            psi_denom = 1.0f + params.psi * ratio * ratio;
        }

        // Bernoulli flow with Psi confinement
        float abs_dp = std::max(std::abs(dp), 1e-10f);
        float u_reed = std::copysign(1.0f, dp) * S_opening
                     * std::sqrt(2.0f * abs_dp / (params.rho_air * psi_denom));

        // Clamp flow to prevent blowup at extreme parameter combos
        u_reed = std::max(-0.01f, std::min(u_reed, 0.01f));

        return u_reed;
    }

    // Lightweight per-sample embouchure update for expression modifiers
    void setEmbouchure(float emb) { params.embouchure = emb; }

    float getZc() const { return cachedZc; }
    float getBreathPressure() const { return cachedBreathPressure; }

    // Closure pressure: dp at which reed just closes (oscillation regime is 0 to ~p_closure)
    float getClosurePressure() const
    {
        float emb = params.embouchure;
        float k_eff = params.k_r + emb * 5e6f;
        float H_eff = std::max(params.H - emb * 0.0003f, 1e-5f);
        return k_eff * H_eff;
    }

    void reset()
    {
        state.x = 0.0f;
        state.x_dot = 0.0f;
    }

    bool isActive() const
    {
        return std::abs(state.x_dot) > 1e-10f;
    }

private:
    ReedParams params;
    ReedState state;
    float dt = 1.0f / 44100.0f;
    float cachedZc = 2.3e6f;
    float cachedBreathPressure = 6000.0f;
};
