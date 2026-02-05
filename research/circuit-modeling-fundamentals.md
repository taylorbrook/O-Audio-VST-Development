---
title: "Circuit Modeling Fundamentals for Analog Audio Effects"
summary: "Research report covering primary techniques for digitally recreating analog audio circuits, including Wave Digital Filters, nodal analysis, distortion/saturation modeling, tape saturation, transformer modeling, and compressor/limiter emulation with C++ implementations."
domain: dsp
type: algorithm
keywords:
  - circuit-modeling
  - wave-digital-filters
  - analog-emulation
  - distortion
  - saturation
  - tape-modeling
  - transformer
  - compressor
  - waveshaping
  - newton-raphson
stages: [0, 2]
agents: [dsp, research]
---

# Circuit Modeling Fundamentals for Analog Audio Effects

## Research Report: Physical Modeling Techniques for Hardware Emulation

**Author:** Research Agent 1
**Date:** January 2026
**Version:** 1.0

---

## Table of Contents

1. [Introduction](#1-introduction)
2. [Wave Digital Filters (WDF)](#2-wave-digital-filters-wdf)
3. [Nodal Analysis / SPICE-style Modeling](#3-nodal-analysis--spice-style-modeling)
4. [Distortion and Saturation Modeling](#4-distortion-and-saturation-modeling)
5. [Tape Saturation Modeling](#5-tape-saturation-modeling)
6. [Transformer Modeling](#6-transformer-modeling)
7. [Compressor/Limiter Modeling](#7-compressorlimiter-modeling)
8. [Complexity Analysis and Performance](#8-complexity-analysis-and-performance)
9. [References and Further Reading](#9-references-and-further-reading)

---

## 1. Introduction

Circuit modeling for audio effects aims to digitally recreate the behavior of analog electronic circuits with sufficient accuracy to capture their sonic characteristics. This report covers the primary techniques used in professional audio software development, ranging from mathematically rigorous approaches (Wave Digital Filters, State-space modeling) to pragmatic approximations (waveshaping, lookup tables).

### Key Trade-offs in Circuit Modeling

| Approach | Accuracy | CPU Cost | Flexibility | Implementation Complexity |
|----------|----------|----------|-------------|--------------------------|
| Wave Digital Filters | High | Medium-High | Medium | High |
| State-Space/MNA | Very High | High | High | Very High |
| Waveshaping | Medium | Low | Low | Low |
| Lookup Tables | Medium | Very Low | Low | Medium |
| Neural Networks | Variable | Medium | Low | High |

---

## 2. Wave Digital Filters (WDF)

### 2.1 Historical Background and Theory

Wave Digital Filters were introduced by Alfred Fettweis in 1971 as a method for digitizing classical analog filter structures while preserving their desirable properties. The key insight is to represent circuit variables as traveling waves rather than voltages and currents directly.

### 2.2 Mathematical Foundation

#### Wave Variables

For a one-port element with port resistance R_p:

```
Incident wave:     a = v + R_p * i
Reflected wave:    b = v - R_p * i

Where:
  v = voltage across the port
  i = current into the port
  R_p = port resistance (free parameter)
```

From these definitions:
```
v = (a + b) / 2
i = (a - b) / (2 * R_p)
```

#### Scattering Parameters

Each circuit element is characterized by its scattering behavior - how it reflects incident waves:

**Resistor (resistance R):**
```
b = a * (R - R_p) / (R + R_p)

// When R = R_p (matched), b = 0 (no reflection)
Scattering coefficient: S = (R - R_p) / (R + R_p)
```

**Capacitor (capacitance C):**
```
// State variable: voltage across capacitor
// Using bilinear transform at sample rate fs:

b[n] = a[n-1]  // When R_p = 1/(2*C*fs)

// The capacitor acts as a unit delay at matched port resistance
// This is a key insight - matching eliminates reflection artifacts
```

**Inductor (inductance L):**
```
// State variable: current through inductor

b[n] = -a[n-1]  // When R_p = 2*L*fs

// Negative sign indicates phase inversion
```

**Ideal Voltage Source (voltage E):**
```
b = 2*E - a
```

**Ideal Current Source (current J):**
```
b = 2*R_p*J + a
```

### 2.3 Port Adaptors

Adaptors connect multiple one-port elements, enforcing Kirchhoff's laws.

#### Series Adaptor (3-port)

Three ports in series (voltages add, currents equal):

```cpp
class SeriesAdaptor {
    float R1, R2, R3;  // Port resistances
    float gamma1, gamma2, gamma3;  // Scattering coefficients

    void setPortResistances(float r1, float r2, float r3) {
        R1 = r1; R2 = r2; R3 = r3;
        float Rsum = R1 + R2 + R3;
        gamma1 = 2.0f * R1 / Rsum;
        gamma2 = 2.0f * R2 / Rsum;
        gamma3 = 2.0f * R3 / Rsum;
    }

    // Port 3 is the "adapted" port (reflection-free)
    float getAdaptedPortResistance() {
        return R1 + R2;
    }

    void process(float a1, float a2, float a3,
                 float& b1, float& b2, float& b3) {
        // Compute junction wave (sum of weighted incident waves)
        float junction = -(a1 * gamma1 + a2 * gamma2 + a3 * gamma3);

        // Reflected waves
        b1 = junction + a1;
        b2 = junction + a2;
        b3 = junction + a3;
    }
};
```

#### Parallel Adaptor (3-port)

Three ports in parallel (currents add, voltages equal):

```cpp
class ParallelAdaptor {
    float G1, G2, G3;  // Port conductances (1/R)
    float gamma1, gamma2, gamma3;

    void setPortResistances(float r1, float r2, float r3) {
        G1 = 1.0f/r1; G2 = 1.0f/r2; G3 = 1.0f/r3;
        float Gsum = G1 + G2 + G3;
        gamma1 = 2.0f * G1 / Gsum;
        gamma2 = 2.0f * G2 / Gsum;
        gamma3 = 2.0f * G3 / Gsum;
    }

    float getAdaptedPortResistance() {
        return 1.0f / (G1 + G2);
    }

    void process(float a1, float a2, float a3,
                 float& b1, float& b2, float& b3) {
        float junction = a1 * gamma1 + a2 * gamma2 + a3 * gamma3;

        b1 = junction - a1;
        b2 = junction - a2;
        b3 = junction - a3;
    }
};
```

### 2.4 Non-linear Elements in WDF

Non-linear elements (diodes, transistors) require special handling because their scattering behavior depends on instantaneous voltage/current.

#### Diode Model

```cpp
class WDFDiode {
    float Is = 1e-15f;   // Saturation current
    float Vt = 0.026f;   // Thermal voltage (kT/q at room temp)
    float n = 1.0f;      // Ideality factor
    float Rp;            // Port resistance

    // Shockley diode equation: i = Is * (exp(v/(n*Vt)) - 1)

    float process(float a) {
        // Newton-Raphson iteration to find reflected wave
        // We need to solve: v = f(v) where v = (a + b)/2
        // and b = a - 2*Rp*Is*(exp(v/(n*Vt)) - 1)

        float v = 0.0f;  // Initial guess
        for (int iter = 0; iter < 8; iter++) {
            float expTerm = std::exp(v / (n * Vt));
            float i = Is * (expTerm - 1.0f);
            float di_dv = Is * expTerm / (n * Vt);

            // f(v) = (a + a - 2*Rp*i) / 2 = a - Rp*i
            float fv = a - Rp * i;
            float dfv = -Rp * di_dv;

            // Newton step: v_new = v - (v - f(v)) / (1 - f'(v))
            float error = v - fv;
            float deriv = 1.0f - dfv;
            v = v - error / deriv;
        }

        // Compute reflected wave from converged voltage
        float i = Is * (std::exp(v / (n * Vt)) - 1.0f);
        return a - 2.0f * Rp * i;
    }
};
```

### 2.5 Complete WDF RC Lowpass Filter Example

```cpp
class WDFRCLowpass {
    // Components
    float R, C;
    float sampleRate;

    // Port resistances
    float Rp_R, Rp_C;

    // State (capacitor wave)
    float cap_a = 0.0f;

    // Scattering coefficient
    float S_R;  // Resistor scattering

public:
    void prepare(float resistance, float capacitance, float fs) {
        R = resistance;
        C = capacitance;
        sampleRate = fs;

        // Set capacitor port resistance for bilinear transform
        Rp_C = 1.0f / (2.0f * C * fs);

        // Resistor port resistance matches the capacitor
        Rp_R = Rp_C;  // This gives reflection-free adaption

        // Calculate resistor scattering coefficient
        S_R = (R - Rp_R) / (R + Rp_R);
    }

    float process(float input) {
        // Input voltage source
        float vs_b = 2.0f * input - cap_a;  // Reflected from voltage source

        // Resistor scatters
        float r_a = vs_b;
        float r_b = S_R * r_a;

        // Capacitor receives
        float cap_b = r_b;

        // Output is capacitor voltage
        float output = (cap_a + cap_b) * 0.5f;

        // Update state (capacitor is unit delay at matched Rp)
        cap_a = cap_b;

        return output;
    }
};
```

### 2.6 Practical WDF Tone Stack (Fender-style)

```cpp
class FenderToneStack {
    // Component values (typical Fender blackface)
    float R1 = 250e3f;   // Treble pot
    float R2 = 1e6f;     // Bass pot
    float R3 = 25e3f;    // Mid pot
    float R4 = 56e3f;    // Fixed resistor
    float C1 = 250e-12f; // Treble cap
    float C2 = 100e-9f;  // Bass cap
    float C3 = 47e-9f;   // Mid cap

    // This requires a more complex adaptor tree
    // Typically implemented as nested series/parallel adaptors

    // Simplified: Use state-space for complex topologies
};
```

### 2.7 WDF Pros and Cons

**Advantages:**
- Guaranteed stability (passive circuits remain passive)
- Modularity - components can be swapped easily
- Natural handling of reactive elements
- No matrix inversions at runtime
- Energy-conserving

**Disadvantages:**
- Complex topology handling (multiple non-linear elements)
- Delay-free loops require special techniques
- Some topologies cannot be directly adapted
- Higher complexity than direct form filters

---

## 3. Nodal Analysis / SPICE-style Modeling

### 3.1 Modified Nodal Analysis (MNA)

MNA extends standard nodal analysis to handle voltage sources and current-controlled elements. It forms the basis of SPICE circuit simulation.

#### System Formulation

```
[G  B] [v]   [i]
[C  D] [j] = [e]

Where:
  G = conductance matrix (n x n)
  B = voltage source incidence matrix
  C = B^T for passive elements
  D = typically zero
  v = node voltages
  j = voltage source currents
  i = current source values
  e = voltage source values
```

#### Example: RC Circuit MNA

For a simple RC lowpass (voltage source -> R -> C -> ground):

```cpp
class MNARCLowpass {
    // Component values
    float R, C;
    float dt;  // 1/sampleRate

    // State
    float v_c = 0.0f;  // Capacitor voltage

public:
    void prepare(float resistance, float capacitance, float sampleRate) {
        R = resistance;
        C = capacitance;
        dt = 1.0f / sampleRate;
    }

    float process(float input) {
        // Using backward Euler discretization
        // C * dv/dt = i = (Vin - v) / R
        // C * (v[n] - v[n-1]) / dt = (Vin - v[n]) / R
        // Solving for v[n]:
        // v[n] * (C/dt + 1/R) = C*v[n-1]/dt + Vin/R

        float alpha = C / dt;
        float beta = 1.0f / R;

        v_c = (alpha * v_c + beta * input) / (alpha + beta);

        return v_c;
    }
};
```

### 3.2 State-Space Representation

For a linear circuit with state vector x (capacitor voltages, inductor currents):

```
dx/dt = A*x + B*u
y = C*x + D*u

Where:
  x = state vector
  u = input vector
  y = output vector
  A, B, C, D = system matrices
```

#### Discretization Methods

**Forward Euler (explicit):**
```
x[n+1] = x[n] + dt * A * x[n] + dt * B * u[n]
       = (I + dt*A) * x[n] + dt * B * u[n]

// Simple but can be unstable for stiff systems
```

**Backward Euler (implicit):**
```
x[n+1] = x[n] + dt * A * x[n+1] + dt * B * u[n+1]
x[n+1] = (I - dt*A)^(-1) * (x[n] + dt * B * u[n+1])

// More stable, requires matrix inverse (computed once at init)
```

**Trapezoidal (Tustin/Bilinear):**
```
x[n+1] = x[n] + (dt/2) * A * (x[n] + x[n+1]) + (dt/2) * B * (u[n] + u[n+1])

// Best frequency response, most common choice
```

### 3.3 Non-linear Solver: Newton-Raphson

For circuits with non-linear elements, we solve iteratively:

```cpp
// f(x) = 0, where f encapsulates circuit equations

void newtonRaphson(float* x, int n, int maxIter = 10, float tol = 1e-6f) {
    float f[MAX_VARS];
    float J[MAX_VARS][MAX_VARS];  // Jacobian
    float dx[MAX_VARS];

    for (int iter = 0; iter < maxIter; iter++) {
        // Evaluate f(x) and Jacobian J = df/dx
        evaluateCircuit(x, f, J);

        // Solve J * dx = -f
        solveLinearSystem(J, f, dx, n);

        // Update: x = x + dx
        float maxChange = 0.0f;
        for (int i = 0; i < n; i++) {
            x[i] += dx[i];
            maxChange = std::max(maxChange, std::abs(dx[i]));
        }

        // Convergence check
        if (maxChange < tol) break;
    }
}

// For 2x2 systems (common in audio):
void solve2x2(float J[2][2], float f[2], float dx[2]) {
    float det = J[0][0]*J[1][1] - J[0][1]*J[1][0];
    dx[0] = (-f[0]*J[1][1] + f[1]*J[0][1]) / det;
    dx[1] = (-f[1]*J[0][0] + f[0]*J[1][0]) / det;
}
```

### 3.4 Handling Stiff Systems

Audio circuits often have vastly different time constants (e.g., fast diode switching + slow filter). This "stiffness" causes stability issues.

**Solutions:**

1. **Implicit integration** - Backward Euler, Trapezoidal
2. **Subcycling** - Run non-linear solver at higher rate
3. **Limiting** - Clamp Newton-Raphson steps
4. **Damping** - Reduce step size: `x = x + alpha*dx` where alpha < 1

```cpp
// K-method (common in guitar amp modeling)
// Separates linear and non-linear parts

class KMethodSolver {
    // Linear system: y = H * x + K * f(y)
    // where f(y) is the non-linear function

    float H, K;  // Precomputed from circuit

    float solve(float x, float y_prev) {
        float y = y_prev;  // Initial guess from previous sample

        for (int i = 0; i < 4; i++) {
            float f_y = nonlinearFunction(y);
            float df_y = nonlinearDerivative(y);

            // Newton step
            float error = y - H*x - K*f_y;
            float deriv = 1.0f - K*df_y;
            y = y - error / deriv;
        }
        return y;
    }
};
```

### 3.5 Real-time Considerations

| Technique | Typical CPU Cost | Best For |
|-----------|-----------------|----------|
| Direct state-space | Very low | Linear circuits |
| Newton-Raphson (2-4 iter) | Low-Medium | Single non-linearity |
| Full MNA + NR | High | Complex topologies |
| Oversampling + NR | Very High | Extreme accuracy |

---

## 4. Distortion and Saturation Modeling

### 4.1 Diode Clipping Models

#### Shockley Diode Equation

```cpp
// Ideal diode: i = Is * (exp(v / (n*Vt)) - 1)
//
// Parameters:
//   Is = saturation current (1e-12 to 1e-15 A typical)
//   n = ideality factor (1.0 to 2.0)
//   Vt = thermal voltage = kT/q = 0.026V at room temp

float diodeCurrent(float v, float Is, float n, float Vt) {
    return Is * (std::exp(v / (n * Vt)) - 1.0f);
}

// For numerical stability, use:
float diodeCurrentSafe(float v, float Is, float n, float Vt) {
    float x = v / (n * Vt);
    if (x > 30.0f) return Is * std::exp(30.0f) * (1.0f + x - 30.0f);  // Linear extrapolation
    if (x < -30.0f) return -Is;
    return Is * (std::exp(x) - 1.0f);
}
```

#### Symmetric Diode Clipper (e.g., Tube Screamer)

```cpp
class SymmetricDiodeClipper {
    float Is = 2.52e-9f;   // 1N914 diode
    float n = 1.752f;
    float Vt = 0.026f;

    float R = 4700.0f;     // Series resistor
    float C = 47e-9f;      // Feedback cap

    float state = 0.0f;    // Capacitor voltage
    float dt;

public:
    void prepare(float sampleRate) {
        dt = 1.0f / sampleRate;
    }

    float process(float input) {
        // Simplified model: output clipped by anti-parallel diodes
        // Full model requires Newton-Raphson in feedback loop

        float v = input;

        // Newton-Raphson for diode pair
        for (int i = 0; i < 8; i++) {
            float i_d1 = Is * (std::exp(v / (n*Vt)) - 1.0f);
            float i_d2 = Is * (std::exp(-v / (n*Vt)) - 1.0f);
            float i_total = i_d1 - i_d2;

            float di_d1 = Is * std::exp(v / (n*Vt)) / (n*Vt);
            float di_d2 = -Is * std::exp(-v / (n*Vt)) / (n*Vt);
            float di_total = di_d1 - di_d2;

            float error = v + R * i_total - input;
            float deriv = 1.0f + R * di_total;

            v = v - error / deriv;
        }

        return v;
    }
};
```

#### Asymmetric Clipping (Germanium + Silicon)

```cpp
class AsymmetricClipper {
    // Two different diodes for positive/negative
    float Is_ge = 200e-6f;   // Germanium (higher Is, lower Vf)
    float n_ge = 2.0f;
    float Is_si = 1e-12f;    // Silicon (lower Is, higher Vf)
    float n_si = 1.0f;
    float Vt = 0.026f;

    float process(float input) {
        float v = input;

        for (int i = 0; i < 8; i++) {
            // Germanium clips positive half
            float i_ge = Is_ge * (std::exp(v / (n_ge*Vt)) - 1.0f);
            // Silicon clips negative half
            float i_si = Is_si * (std::exp(-v / (n_si*Vt)) - 1.0f);

            // ... Newton-Raphson iteration
        }

        return v;
    }
};
```

### 4.2 Transistor Saturation

#### BJT Ebers-Moll Model (Simplified)

```cpp
class BJTSaturation {
    float Is = 1e-14f;
    float Vt = 0.026f;
    float beta_f = 100.0f;  // Forward current gain
    float beta_r = 1.0f;    // Reverse current gain

    void computeCurrents(float Vbe, float Vbc,
                         float& Ic, float& Ib, float& Ie) {
        float If = Is * (std::exp(Vbe / Vt) - 1.0f);
        float Ir = Is * (std::exp(Vbc / Vt) - 1.0f);

        Ic = If - Ir / beta_r;
        Ib = If / beta_f + Ir / beta_r;
        Ie = If * (1.0f + 1.0f/beta_f) - Ir * (1.0f + 1.0f/beta_r);
    }

    // Simplified transfer characteristic for common emitter
    float transferFunction(float Vin, float Vcc, float Rc, float Re) {
        // Approximation for audio-range signals
        float Vbe = 0.6f + Vin * 0.1f;  // Simplified
        float Ic = Is * std::exp(Vbe / Vt);
        float Vout = Vcc - Ic * Rc;

        // Clamp to saturation/cutoff
        Vout = std::max(0.2f, std::min(Vcc, Vout));
        return Vout;
    }
};
```

### 4.3 Tube/Valve Saturation

#### Triode Model (Koren equations)

```cpp
class TriodeModel {
    // 12AX7 typical parameters
    float mu = 100.0f;      // Amplification factor
    float Kp = 600.0f;      // Plate coefficient
    float Kvb = 300.0f;     // Knee voltage
    float Ex = 1.4f;        // Plate current exponent
    float Kg1 = 1060.0f;    // Grid coefficient
    float Vct = 0.0f;       // Contact potential

    float plateCurrent(float Vp, float Vg) {
        // Koren model
        float E1 = Vp / Kp * std::log(1.0f + std::exp(Kp * (1.0f/mu + Vg/Vp)));

        if (E1 > 0.0f) {
            return std::pow(E1, Ex) / Kg1;
        }
        return 0.0f;
    }

    float process(float Vg_in, float Vp_supply, float Rp) {
        // Newton-Raphson to find operating point
        float Vp = Vp_supply * 0.5f;  // Initial guess

        for (int i = 0; i < 8; i++) {
            float Ip = plateCurrent(Vp, Vg_in);
            float Vp_calc = Vp_supply - Ip * Rp;
            float error = Vp - Vp_calc;

            // Numerical derivative
            float Ip_delta = plateCurrent(Vp + 0.1f, Vg_in);
            float dIp_dVp = (Ip_delta - Ip) / 0.1f;
            float deriv = 1.0f + Rp * dIp_dVp;

            Vp = Vp - error / deriv;
        }

        return Vp;
    }
};
```

#### Simplified Tube Saturation (Waveshaping)

```cpp
// Fast approximation using tanh-based waveshaping
float tubeSaturation(float x, float drive, float bias) {
    // Asymmetric soft clipping
    float biased = x + bias;
    float driven = biased * drive;

    // Different saturation for positive/negative
    if (driven >= 0.0f) {
        return std::tanh(driven);
    } else {
        return std::tanh(driven * 1.2f) / 1.2f;  // Softer negative clip
    }
}

// More realistic: piece-wise function
float tubeTransfer(float x) {
    if (x < -1.0f) {
        return -1.0f;  // Hard cutoff (tube cuts off)
    } else if (x < 0.0f) {
        // Smooth transition region
        return x - x*x*x/3.0f;
    } else if (x < 1.0f) {
        // Near-linear region with soft saturation
        return x - x*x*x/3.0f + x*x*x*x*x/5.0f;
    } else {
        // Soft saturation (tube saturation)
        return 2.0f/3.0f + (x - 1.0f) * 0.1f;  // Gradual limit
    }
}
```

### 4.4 Waveshaping Functions

```cpp
// Common saturation curves

// Soft clipping - tanh
float tanhSat(float x, float drive) {
    return std::tanh(x * drive) / std::tanh(drive);
}

// Arctan - even softer
float atanSat(float x, float drive) {
    return std::atan(x * drive) / std::atan(drive);
}

// Polynomial (cheap, no transcendentals)
float polySat(float x) {
    // Attempt to match tanh shape
    if (x > 1.0f) return 1.0f;
    if (x < -1.0f) return -1.0f;
    return 1.5f * x - 0.5f * x * x * x;
}

// Hard clip
float hardClip(float x, float threshold) {
    return std::max(-threshold, std::min(threshold, x));
}

// Exponential soft clip (asymmetric friendly)
float expSat(float x, float drive) {
    if (x >= 0.0f) {
        return 1.0f - std::exp(-x * drive);
    } else {
        return -1.0f + std::exp(x * drive);
    }
}

// Foldback distortion
float foldback(float x, float threshold) {
    while (std::abs(x) > threshold) {
        if (x > threshold) x = 2.0f * threshold - x;
        else if (x < -threshold) x = -2.0f * threshold - x;
    }
    return x;
}
```

### 4.5 Anti-aliasing for Waveshaping

Waveshaping introduces harmonics that can alias. Solutions:

```cpp
// 1. Oversampling
class OversampledWaveshaper {
    // Upsample -> waveshape -> downsample
    // Typically 4x-8x for heavy distortion

    float upsampleBuffer[8];
    float downsampleBuffer[8];

    float process(float input) {
        // Upsample (polyphase FIR)
        upsample4x(input, upsampleBuffer);

        // Apply waveshaping at higher rate
        for (int i = 0; i < 4; i++) {
            upsampleBuffer[i] = waveshape(upsampleBuffer[i]);
        }

        // Downsample (polyphase FIR with anti-alias)
        return downsample4x(upsampleBuffer);
    }
};

// 2. ADAA (Antiderivative Anti-Aliasing)
// For y = f(x), use first antiderivative F(x)
// Output: (F(x[n]) - F(x[n-1])) / (x[n] - x[n-1])

class ADAATanh {
    float x_prev = 0.0f;
    float F_prev = 0.0f;

    // Antiderivative of tanh(x) = log(cosh(x))
    float antiderivative(float x) {
        return std::log(std::cosh(x));
    }

    float process(float x) {
        float F = antiderivative(x);
        float output;

        float dx = x - x_prev;
        if (std::abs(dx) < 1e-6f) {
            // Avoid division by zero - use direct evaluation
            output = std::tanh(x);
        } else {
            output = (F - F_prev) / dx;
        }

        x_prev = x;
        F_prev = F;
        return output;
    }
};
```

---

## 5. Tape Saturation Modeling

### 5.1 Jiles-Atherton Hysteresis Model

The Jiles-Atherton model describes magnetic hysteresis in tape recording.

#### Core Equations

```
M = total magnetization
Man = anhysteretic magnetization (what M would be without hysteresis)
H = magnetic field strength
a = domain wall density parameter
Ms = saturation magnetization
alpha = mean field parameter (domain coupling)
c = reversibility coefficient
k = pinning coefficient

Anhysteretic magnetization (Langevin function):
Man = Ms * (coth(He/a) - a/He)
where He = H + alpha*M (effective field)

Differential equation:
dM/dH = (Man - M) / (k*delta - alpha*(Man - M)) + c * dMan/dH
where delta = sign(dH/dt)
```

#### Implementation

```cpp
class JilesAthertonTape {
    // Tape parameters (adjust for different tape types)
    float Ms = 350000.0f;   // Saturation magnetization
    float a = 25.0f;        // Domain wall density
    float alpha = 1.6e-3f;  // Mean field parameter
    float k = 20.0f;        // Pinning coefficient
    float c = 0.2f;         // Reversibility

    float M = 0.0f;         // Current magnetization state
    float H_prev = 0.0f;    // Previous field value

    float langevin(float x) {
        if (std::abs(x) < 1e-6f) return x / 3.0f;
        return 1.0f / std::tanh(x) - 1.0f / x;  // coth(x) - 1/x
    }

    float langevinDeriv(float x) {
        if (std::abs(x) < 1e-6f) return 1.0f / 3.0f;
        float cothx = 1.0f / std::tanh(x);
        return -cothx * cothx + 1.0f / (x * x) + 1.0f;
    }

public:
    float process(float H) {
        float dH = H - H_prev;
        float delta = (dH >= 0.0f) ? 1.0f : -1.0f;

        // Effective field
        float He = H + alpha * M;

        // Anhysteretic magnetization
        float Man = Ms * langevin(He / a);
        float dMan_dH = Ms * langevinDeriv(He / a) * (1.0f + alpha) / a;

        // Irreversible component
        float dM_irr = (Man - M) / (k * delta - alpha * (Man - M));

        // Total magnetization change
        float dM_dH = dM_irr + c * dMan_dH;

        // Prevent infinite derivatives near reversal
        dM_dH = std::max(-1e6f, std::min(1e6f, dM_dH));

        // Update magnetization
        M += dM_dH * dH;
        M = std::max(-Ms, std::min(Ms, M));

        H_prev = H;
        return M / Ms;  // Normalized output
    }
};
```

### 5.2 Simplified Tape Saturation

```cpp
class SimpleTapeSaturation {
    // State for hysteresis
    float prev_in = 0.0f;
    float prev_out = 0.0f;
    float bias = 0.0f;

    // Parameters
    float saturation = 0.8f;    // Saturation threshold
    float hysteresis = 0.1f;    // Hysteresis amount
    float softness = 0.3f;      // Knee softness

public:
    float process(float input) {
        // Add tape bias (high-frequency)
        float biased = input + bias;

        // Soft saturation with asymmetry
        float sat;
        if (biased >= 0.0f) {
            sat = std::tanh(biased / saturation) * saturation;
        } else {
            sat = std::tanh(biased / saturation * 1.1f) * saturation / 1.1f;
        }

        // Simple hysteresis (slew-rate dependent)
        float delta = sat - prev_out;
        float slewLimited = delta * (1.0f - hysteresis * std::abs(delta));
        float output = prev_out + slewLimited;

        prev_in = input;
        prev_out = output;

        return output;
    }
};
```

### 5.3 Tape Head Characteristics

```cpp
class TapeHeadModel {
    // Head bump (low-frequency resonance)
    float headBumpFreq = 80.0f;
    float headBumpQ = 0.7f;
    float headBumpGain = 3.0f;  // dB

    // High-frequency roll-off
    float hfCorner = 15000.0f;
    float hfSlope = -3.0f;  // dB/octave

    // State for filters
    float lp_state[2] = {0};
    float bump_state[2] = {0};

public:
    void prepare(float sampleRate) {
        // Compute filter coefficients
        // Head bump: peaking EQ
        // HF rolloff: first-order lowpass
    }

    float process(float input) {
        // Apply head bump (low-mid boost)
        float bumped = applyHeadBump(input);

        // Apply HF rolloff
        float output = applyHFRolloff(bumped);

        return output;
    }
};
```

### 5.4 Wow and Flutter

```cpp
class WowAndFlutter {
    // LFO states
    float wowPhase = 0.0f;
    float flutterPhase = 0.0f;

    // Parameters
    float wowRate = 0.5f;       // Hz
    float wowDepth = 0.002f;    // Pitch deviation
    float flutterRate = 6.0f;   // Hz
    float flutterDepth = 0.0005f;

    // Delay line for pitch modulation
    static constexpr int MAX_DELAY = 4096;
    float delayLine[MAX_DELAY];
    float writePos = 0.0f;
    int writeIndex = 0;

    float sampleRate;

public:
    void prepare(float fs) {
        sampleRate = fs;
        std::fill(delayLine, delayLine + MAX_DELAY, 0.0f);
    }

    float process(float input) {
        // Write to delay line
        delayLine[writeIndex] = input;
        writeIndex = (writeIndex + 1) % MAX_DELAY;

        // Compute wow/flutter modulation
        float wow = std::sin(wowPhase) * wowDepth;
        float flutter = std::sin(flutterPhase) * flutterDepth;
        float totalMod = wow + flutter;

        // Update LFO phases
        wowPhase += 2.0f * M_PI * wowRate / sampleRate;
        flutterPhase += 2.0f * M_PI * flutterRate / sampleRate;
        if (wowPhase > 2.0f * M_PI) wowPhase -= 2.0f * M_PI;
        if (flutterPhase > 2.0f * M_PI) flutterPhase -= 2.0f * M_PI;

        // Variable delay read
        float nominalDelay = 1000.0f;  // samples (about 20ms at 48kHz)
        float modulatedDelay = nominalDelay * (1.0f + totalMod);

        float readPos = writeIndex - modulatedDelay;
        while (readPos < 0) readPos += MAX_DELAY;

        // Cubic interpolation for smooth pitch variation
        int idx0 = (int)readPos;
        float frac = readPos - idx0;

        int idx_m1 = (idx0 - 1 + MAX_DELAY) % MAX_DELAY;
        int idx_p1 = (idx0 + 1) % MAX_DELAY;
        int idx_p2 = (idx0 + 2) % MAX_DELAY;

        float y_m1 = delayLine[idx_m1];
        float y_0 = delayLine[idx0];
        float y_p1 = delayLine[idx_p1];
        float y_p2 = delayLine[idx_p2];

        // Cubic Hermite interpolation
        float c0 = y_0;
        float c1 = 0.5f * (y_p1 - y_m1);
        float c2 = y_m1 - 2.5f * y_0 + 2.0f * y_p1 - 0.5f * y_p2;
        float c3 = 0.5f * (y_p2 - y_m1) + 1.5f * (y_0 - y_p1);

        return ((c3 * frac + c2) * frac + c1) * frac + c0;
    }
};
```

### 5.5 Complete Tape Processor

```cpp
class TapeProcessor {
    JilesAthertonTape saturation;
    TapeHeadModel head;
    WowAndFlutter wobble;

    // Noise
    float hissLevel = -60.0f;  // dB

    float inputGain = 1.0f;    // "Record level"
    float biasAmount = 0.5f;

public:
    void prepare(float sampleRate) {
        head.prepare(sampleRate);
        wobble.prepare(sampleRate);
    }

    float process(float input) {
        // Input gain (record level)
        float signal = input * inputGain;

        // Tape head frequency response
        signal = head.process(signal);

        // Magnetic saturation
        signal = saturation.process(signal);

        // Wow and flutter
        signal = wobble.process(signal);

        // Tape hiss
        float hiss = (rand() / (float)RAND_MAX * 2.0f - 1.0f);
        hiss *= std::pow(10.0f, hissLevel / 20.0f);
        signal += hiss;

        return signal;
    }
};
```

---

## 6. Transformer Modeling

### 6.1 Transformer Equivalent Circuit

```
Primary side:
  Rp = primary winding resistance
  Lp = primary leakage inductance
  Lm = magnetizing inductance (core)
  Rc = core loss resistance

Secondary side (referred to primary):
  Rs' = secondary resistance * n^2
  Ls' = secondary leakage inductance * n^2

n = turns ratio (Np/Ns)
```

### 6.2 Linear Transformer Model

```cpp
class LinearTransformer {
    // Transformer parameters
    float turnsRatio = 1.0f;
    float primaryR = 100.0f;     // Ohms
    float primaryL = 0.1f;       // Henry (leakage)
    float magnetizingL = 10.0f;  // Henry
    float secondaryR = 100.0f;
    float secondaryL = 0.1f;

    // State
    float magCurrent = 0.0f;     // Magnetizing current
    float leakageState = 0.0f;   // Leakage inductance state

    float dt;

public:
    void prepare(float sampleRate) {
        dt = 1.0f / sampleRate;
    }

    float process(float input, float loadImpedance) {
        // Simplified: treats transformer as lowpass with resonance
        // Real model would use full state-space

        // Low-frequency rolloff from magnetizing inductance
        float omega_lf = primaryR / magnetizingL;
        float hpCoeff = 1.0f - std::exp(-omega_lf * dt);

        // High-frequency rolloff from leakage
        float omega_hf = (primaryR + loadImpedance/turnsRatio/turnsRatio) / primaryL;
        float lpCoeff = 1.0f - std::exp(-omega_hf * dt);

        // Apply filtering
        magCurrent += hpCoeff * (input - magCurrent);
        float hp = input - magCurrent;
        leakageState += lpCoeff * (hp - leakageState);

        return leakageState * turnsRatio;
    }
};
```

### 6.3 Non-linear Core Saturation

```cpp
class TransformerWithSaturation {
    // Saturation parameters
    float coreSaturation = 1.0f;  // Flux density at saturation
    float coreHysteresis = 0.1f;

    // State
    float flux = 0.0f;
    float flux_prev = 0.0f;

    float dt;

    float saturate(float B) {
        // Simple sigmoid saturation
        return coreSaturation * std::tanh(B / coreSaturation);
    }

public:
    void prepare(float sampleRate) {
        dt = 1.0f / sampleRate;
    }

    float process(float input) {
        // V = N * dFlux/dt
        // Flux = integral(V/N) dt

        // Integrate to get flux
        flux += input * dt;

        // Apply saturation
        float saturatedFlux = saturate(flux);

        // Output is rate of change of saturated flux
        float output = (saturatedFlux - flux_prev) / dt;
        flux_prev = saturatedFlux;

        // Slow flux decay to prevent DC buildup
        flux *= 0.9999f;

        return output;
    }
};
```

### 6.4 API/Neve Style Transformer Color

```cpp
class ConsoleTransformer {
    // Modeled on API 2520 / Neve 1073 transformers

    // Low-frequency bump
    float lfBumpFreq = 60.0f;
    float lfBumpQ = 0.7f;
    float lfBumpGain = 2.0f;  // dB

    // High-frequency sheen
    float hfSheenFreq = 8000.0f;
    float hfSheenQ = 0.5f;
    float hfSheenGain = 1.0f;  // dB

    // Saturation
    float satThreshold = 0.8f;
    float satKnee = 0.2f;

    // Biquad filter states
    struct BiquadState {
        float x1 = 0, x2 = 0;
        float y1 = 0, y2 = 0;
    } lfState, hfState;

    struct BiquadCoeffs {
        float b0, b1, b2, a1, a2;
    } lfCoeffs, hfCoeffs;

public:
    void prepare(float sampleRate) {
        // Calculate LF bump (peak EQ)
        calculatePeakEQ(lfBumpFreq, lfBumpQ, lfBumpGain, sampleRate, lfCoeffs);
        // Calculate HF sheen (high shelf)
        calculateHighShelf(hfSheenFreq, hfSheenGain, sampleRate, hfCoeffs);
    }

    float process(float input) {
        // Apply LF bump
        float lf = processBiquad(input, lfState, lfCoeffs);

        // Apply HF sheen
        float hf = processBiquad(lf, hfState, hfCoeffs);

        // Soft saturation
        float output;
        if (std::abs(hf) < satThreshold - satKnee) {
            output = hf;  // Linear region
        } else {
            // Soft knee saturation
            float x = std::abs(hf) - (satThreshold - satKnee);
            float compressed = satThreshold - satKnee + satKnee * std::tanh(x / satKnee);
            output = (hf >= 0) ? compressed : -compressed;
        }

        return output;
    }

private:
    void calculatePeakEQ(float freq, float Q, float dBGain, float fs, BiquadCoeffs& c) {
        float A = std::pow(10.0f, dBGain / 40.0f);
        float omega = 2.0f * M_PI * freq / fs;
        float sinOmega = std::sin(omega);
        float cosOmega = std::cos(omega);
        float alpha = sinOmega / (2.0f * Q);

        float a0 = 1.0f + alpha / A;
        c.b0 = (1.0f + alpha * A) / a0;
        c.b1 = (-2.0f * cosOmega) / a0;
        c.b2 = (1.0f - alpha * A) / a0;
        c.a1 = c.b1;
        c.a2 = (1.0f - alpha / A) / a0;
    }

    void calculateHighShelf(float freq, float dBGain, float fs, BiquadCoeffs& c) {
        float A = std::pow(10.0f, dBGain / 40.0f);
        float omega = 2.0f * M_PI * freq / fs;
        float sinOmega = std::sin(omega);
        float cosOmega = std::cos(omega);
        float alpha = sinOmega / 2.0f * std::sqrt(2.0f);

        float a0 = (A+1) - (A-1)*cosOmega + 2*std::sqrt(A)*alpha;
        c.b0 = A*((A+1) + (A-1)*cosOmega + 2*std::sqrt(A)*alpha) / a0;
        c.b1 = -2*A*((A-1) + (A+1)*cosOmega) / a0;
        c.b2 = A*((A+1) + (A-1)*cosOmega - 2*std::sqrt(A)*alpha) / a0;
        c.a1 = 2*((A-1) - (A+1)*cosOmega) / a0;
        c.a2 = ((A+1) - (A-1)*cosOmega - 2*std::sqrt(A)*alpha) / a0;
    }

    float processBiquad(float input, BiquadState& s, const BiquadCoeffs& c) {
        float output = c.b0*input + c.b1*s.x1 + c.b2*s.x2 - c.a1*s.y1 - c.a2*s.y2;
        s.x2 = s.x1; s.x1 = input;
        s.y2 = s.y1; s.y1 = output;
        return output;
    }
};
```

---

## 7. Compressor/Limiter Modeling

### 7.1 Basic Compressor Architecture

```cpp
class BasicCompressor {
    // Parameters
    float threshold = -10.0f;   // dB
    float ratio = 4.0f;         // :1
    float attackTime = 10.0f;   // ms
    float releaseTime = 100.0f; // ms
    float kneeWidth = 6.0f;     // dB (soft knee)
    float makeupGain = 0.0f;    // dB

    // Envelope follower state
    float envelope = 0.0f;

    // Coefficients
    float attackCoeff, releaseCoeff;

public:
    void prepare(float sampleRate) {
        attackCoeff = std::exp(-1.0f / (attackTime * 0.001f * sampleRate));
        releaseCoeff = std::exp(-1.0f / (releaseTime * 0.001f * sampleRate));
    }

    float process(float input) {
        // Level detection (peak or RMS)
        float inputLevel = std::abs(input);
        float inputdB = 20.0f * std::log10(inputLevel + 1e-10f);

        // Envelope follower
        float targetdB = inputdB;
        if (targetdB > envelope) {
            envelope = attackCoeff * envelope + (1.0f - attackCoeff) * targetdB;
        } else {
            envelope = releaseCoeff * envelope + (1.0f - releaseCoeff) * targetdB;
        }

        // Gain computation with soft knee
        float gaindB = computeGain(envelope);

        // Apply gain
        float gain = std::pow(10.0f, gaindB / 20.0f);
        return input * gain;
    }

private:
    float computeGain(float inputdB) {
        float overdB = inputdB - threshold;

        if (kneeWidth > 0.0f && overdB > -kneeWidth/2.0f && overdB < kneeWidth/2.0f) {
            // Soft knee region
            float kneeInput = overdB + kneeWidth/2.0f;
            overdB = kneeInput * kneeInput / (2.0f * kneeWidth);
        } else if (overdB < 0.0f) {
            overdB = 0.0f;  // Below threshold
        }

        float reduction = overdB * (1.0f - 1.0f/ratio);
        return -reduction + makeupGain;
    }
};
```

### 7.2 VCA Compressor (SSL, API style)

```cpp
class VCACompressor {
    // VCA characteristics
    float vcaLinearityError = 0.01f;  // Slight nonlinearity

    // Fast attack, program-dependent release
    float attackTime = 0.1f;   // ms - very fast
    float releaseTime = 100.0f;
    float autoRelease = 0.5f;  // Program-dependent factor

    // Sidechain filter
    float scHPF = 80.0f;  // Hz - common in SSL

    float envelope = 0.0f;
    float slowEnvelope = 0.0f;  // For auto-release

    // Filter state
    float hpState = 0.0f;
    float hpCoeff;

public:
    void prepare(float sampleRate) {
        hpCoeff = std::exp(-2.0f * M_PI * scHPF / sampleRate);
    }

    float process(float input, float sidechain) {
        // Sidechain HPF (reduce pumping from bass)
        float filtered = sidechain - hpState;
        hpState = sidechain * (1.0f - hpCoeff) + hpState * hpCoeff;

        float level = std::abs(filtered);

        // Dual envelope for auto-release
        float fastTarget = level;
        float slowTarget = level;

        // Fast envelope (attack/release)
        float attackCoeff = std::exp(-1.0f / (attackTime * 0.001f * 48000.0f));
        float releaseCoeff = std::exp(-1.0f / (releaseTime * 0.001f * 48000.0f));

        if (fastTarget > envelope) {
            envelope = attackCoeff * envelope + (1.0f - attackCoeff) * fastTarget;
        } else {
            envelope = releaseCoeff * envelope + (1.0f - releaseCoeff) * fastTarget;
        }

        // Slow envelope for auto-release
        float slowCoeff = std::exp(-1.0f / (500.0f * 0.001f * 48000.0f));
        slowEnvelope = slowCoeff * slowEnvelope + (1.0f - slowCoeff) * level;

        // Blend envelopes for program-dependent release
        float envdB = 20.0f * std::log10(envelope + 1e-10f);
        float slowdB = 20.0f * std::log10(slowEnvelope + 1e-10f);
        float blendedEnv = envdB * (1.0f - autoRelease) + slowdB * autoRelease;

        // Gain computation (hard knee typical for VCA)
        float gaindB = computeGainHardKnee(blendedEnv);

        // VCA nonlinearity (subtle)
        float gain = std::pow(10.0f, gaindB / 20.0f);
        gain += vcaLinearityError * (gain - 1.0f) * (gain - 1.0f);  // Quadratic error

        return input * gain;
    }

private:
    float threshold = -10.0f;
    float ratio = 4.0f;

    float computeGainHardKnee(float inputdB) {
        if (inputdB < threshold) return 0.0f;
        return -(inputdB - threshold) * (1.0f - 1.0f/ratio);
    }
};
```

### 7.3 Optical Compressor (LA-2A style)

```cpp
class OpticalCompressor {
    // Opto characteristics
    // Slow, program-dependent attack and release
    // Attack: 10ms to 100ms depending on level
    // Release: 60ms to several seconds (two-stage)

    // T4 cell modeling (electro-luminescent panel + photoresistor)
    float cellAttack = 0.0f;
    float cellRelease = 0.0f;
    float cellResistance = 1.0f;  // 1.0 = no compression

    // Two-stage release
    float fastReleaseTime = 60.0f;   // ms
    float slowReleaseTime = 2000.0f; // ms
    float releaseBlend = 0.0f;       // 0 = fast, 1 = slow

    float peakReduction = 0.0f;      // Track peak GR for release blend

public:
    void prepare(float sampleRate) {
        // Pre-compute coefficients
    }

    float process(float input) {
        float level = std::abs(input);
        float leveldB = 20.0f * std::log10(level + 1e-10f);

        // Compute target compression from opto response
        float targetResistance = computeOptoResponse(leveldB);

        // Level-dependent attack time (faster attack for louder signals)
        float attackMs = 100.0f - std::min(90.0f, std::max(0.0f, leveldB + 20.0f) * 3.0f);
        float attackCoeff = std::exp(-1.0f / (attackMs * 0.001f * 48000.0f));

        // Two-stage release
        float releaseMs = fastReleaseTime * (1.0f - releaseBlend) +
                          slowReleaseTime * releaseBlend;
        float releaseCoeff = std::exp(-1.0f / (releaseMs * 0.001f * 48000.0f));

        // Update cell resistance
        if (targetResistance < cellResistance) {
            cellResistance = attackCoeff * cellResistance +
                            (1.0f - attackCoeff) * targetResistance;
        } else {
            cellResistance = releaseCoeff * cellResistance +
                            (1.0f - releaseCoeff) * targetResistance;
        }

        // Track peak reduction for release blend
        float currentReduction = 1.0f - cellResistance;
        if (currentReduction > peakReduction) {
            peakReduction = currentReduction;
            releaseBlend = 0.0f;  // Reset to fast release
        } else {
            // Gradually shift to slow release
            releaseBlend = std::min(1.0f, releaseBlend + 0.0001f);
        }

        // Apply compression (with characteristic LA-2A softness)
        float output = input * cellResistance;

        // Subtle harmonic enhancement (tube makeup amp)
        output = softClipTube(output);

        return output;
    }

private:
    float threshold = -20.0f;
    float ratio = 3.0f;  // LA-2A is roughly 3:1 to infinity

    float computeOptoResponse(float leveldB) {
        // LA-2A has a very gradual knee
        float over = leveldB - threshold;
        if (over < 0.0f) return 1.0f;

        // Non-linear opto response
        // More compression at higher levels (approaching limiting)
        float compressiondB = over * (1.0f - 1.0f/ratio);
        compressiondB *= (1.0f + over * 0.01f);  // Increasing ratio

        return std::pow(10.0f, -compressiondB / 20.0f);
    }

    float softClipTube(float x) {
        // Subtle tube warmth in makeup stage
        return std::tanh(x * 1.2f) / 1.2f * 1.05f;
    }
};
```

### 7.4 FET Compressor (1176 style)

```cpp
class FETCompressor {
    // 1176 characteristics
    // Very fast attack (20us to 800us)
    // Program-dependent release
    // All-buttons mode for extreme compression

    float attackTime = 0.02f;   // ms (20us)
    float releaseTime = 50.0f;  // ms
    float ratio = 4.0f;         // 4:1, 8:1, 12:1, 20:1, or "all"

    // Input/output transformers add color
    float inputDrive = 0.0f;    // dB
    float outputDrive = 0.0f;   // dB

    float envelope = 0.0f;

    // All-buttons mode parameters
    bool allButtonsMode = false;

public:
    void process(float& left, float& right) {
        // Input transformer saturation
        float inputGain = std::pow(10.0f, inputDrive / 20.0f);
        left = transformerSaturate(left * inputGain);
        right = transformerSaturate(right * inputGain);

        // FET detector (full-wave rectified)
        float level = std::max(std::abs(left), std::abs(right));

        // Super fast attack
        float attackCoeff = std::exp(-1.0f / (attackTime * 0.001f * 48000.0f));
        float releaseCoeff = std::exp(-1.0f / (releaseTime * 0.001f * 48000.0f));

        if (level > envelope) {
            envelope = attackCoeff * envelope + (1.0f - attackCoeff) * level;
        } else {
            envelope = releaseCoeff * envelope + (1.0f - releaseCoeff) * level;
        }

        // Gain computation
        float envdB = 20.0f * std::log10(envelope + 1e-10f);
        float gaindB;

        if (allButtonsMode) {
            gaindB = computeGainAllButtons(envdB);
        } else {
            gaindB = computeGainNormal(envdB);
        }

        // FET gain element (slight distortion at high GR)
        float gain = std::pow(10.0f, gaindB / 20.0f);
        gain = fetCharacteristic(gain);

        left *= gain;
        right *= gain;

        // Output transformer
        float outputGain = std::pow(10.0f, outputDrive / 20.0f);
        left = transformerSaturate(left * outputGain);
        right = transformerSaturate(right * outputGain);
    }

private:
    float threshold = -10.0f;

    float computeGainNormal(float inputdB) {
        if (inputdB < threshold) return 0.0f;
        float over = inputdB - threshold;
        return -over * (1.0f - 1.0f/ratio);
    }

    float computeGainAllButtons(float inputdB) {
        // All buttons creates a very aggressive curve
        // Effectively ratio changes with level
        if (inputdB < threshold) return 0.0f;
        float over = inputdB - threshold;

        // Starts around 20:1, approaches infinity
        float dynamicRatio = 20.0f + over * 0.5f;
        return -over * (1.0f - 1.0f/dynamicRatio);
    }

    float fetCharacteristic(float gain) {
        // FET has slight nonlinearity at extremes
        if (gain < 0.1f) {
            // More distortion at high compression
            return gain * (1.0f + (0.1f - gain) * 0.3f);
        }
        return gain;
    }

    float transformerSaturate(float x) {
        // Subtle transformer saturation
        return std::tanh(x * 0.8f) / 0.8f;
    }
};
```

### 7.5 Tube Compressor (Fairchild 670 style)

```cpp
class TubeCompressor {
    // Fairchild characteristics
    // Variable-mu gain reduction using tubes
    // Very smooth, musical compression
    // Complex attack/release with 6 time constants

    // Time constant selector (1-6)
    int timeConstant = 3;

    // Variable-mu characteristics
    float muCurve = 0.5f;  // How "variable" the mu is

    // Multiple envelope followers for smooth response
    float envelopeFast = 0.0f;
    float envelopeMed = 0.0f;
    float envelopeSlow = 0.0f;

    // Tube stages
    float tubeWarmth = 0.3f;

    float getAttackTime() {
        const float attacks[] = {0.2f, 0.4f, 0.8f, 2.0f, 4.0f, 8.0f};
        return attacks[timeConstant - 1];
    }

    float getReleaseTime() {
        const float releases[] = {100.0f, 200.0f, 400.0f, 800.0f, 2000.0f, 5000.0f};
        return releases[timeConstant - 1];
    }

public:
    float process(float input) {
        float level = std::abs(input);

        // Multi-stage envelope (creates smooth response)
        updateEnvelopes(level);

        // Blend envelopes for Fairchild-style response
        float envelope = envelopeFast * 0.3f + envelopeMed * 0.5f + envelopeSlow * 0.2f;

        // Variable-mu gain reduction
        float gain = computeVariableMuGain(envelope);

        // Apply compression
        float output = input * gain;

        // Tube makeup amplifier adds warmth
        output = tubeStage(output);

        return output;
    }

private:
    float threshold = 0.3f;  // Linear threshold

    void updateEnvelopes(float level) {
        float attackMs = getAttackTime();
        float releaseMs = getReleaseTime();

        float attackFast = std::exp(-1.0f / (attackMs * 0.001f * 48000.0f));
        float attackMed = std::exp(-1.0f / (attackMs * 3.0f * 0.001f * 48000.0f));
        float attackSlow = std::exp(-1.0f / (attackMs * 10.0f * 0.001f * 48000.0f));

        float releaseFast = std::exp(-1.0f / (releaseMs * 0.001f * 48000.0f));
        float releaseMed = std::exp(-1.0f / (releaseMs * 2.0f * 0.001f * 48000.0f));
        float releaseSlow = std::exp(-1.0f / (releaseMs * 5.0f * 0.001f * 48000.0f));

        // Update each envelope
        if (level > envelopeFast) {
            envelopeFast = attackFast * envelopeFast + (1.0f - attackFast) * level;
        } else {
            envelopeFast = releaseFast * envelopeFast + (1.0f - releaseFast) * level;
        }

        if (level > envelopeMed) {
            envelopeMed = attackMed * envelopeMed + (1.0f - attackMed) * level;
        } else {
            envelopeMed = releaseMed * envelopeMed + (1.0f - releaseMed) * level;
        }

        if (level > envelopeSlow) {
            envelopeSlow = attackSlow * envelopeSlow + (1.0f - attackSlow) * level;
        } else {
            envelopeSlow = releaseSlow * envelopeSlow + (1.0f - releaseSlow) * level;
        }
    }

    float computeVariableMuGain(float envelope) {
        if (envelope < threshold) return 1.0f;

        // Variable-mu: gain reduction increases smoothly
        float over = (envelope - threshold) / threshold;

        // Soft, musical compression curve
        float reduction = 1.0f / (1.0f + over * muCurve * 3.0f);

        return reduction;
    }

    float tubeStage(float x) {
        // Fairchild tube warmth
        float driven = x * (1.0f + tubeWarmth);

        // Asymmetric soft clipping
        if (driven >= 0.0f) {
            return std::tanh(driven);
        } else {
            return std::tanh(driven * 1.1f) / 1.1f;
        }
    }
};
```

---

## 8. Complexity Analysis and Performance

### 8.1 CPU Cost Comparison

| Technique | Operations/Sample | Memory | Real-time @ 48kHz |
|-----------|-------------------|--------|-------------------|
| Simple waveshaping | 2-5 | Minimal | Trivial |
| Polynomial waveshaping | 5-10 | Minimal | Easy |
| tanh/atan saturation | 15-30 | Minimal | Easy |
| ADAA waveshaping | 30-50 | 16-32 bytes | Easy |
| Biquad filter | 5-8 | 32 bytes | Trivial |
| WDF RC filter | 10-15 | 16-32 bytes | Easy |
| WDF tone stack | 50-100 | 128+ bytes | Moderate |
| Newton-Raphson (4 iter) | 50-100 | 32-64 bytes | Moderate |
| Full state-space (8x8) | 100-200 | 512+ bytes | Moderate |
| Oversampled 4x | 4x base | 4x buffers | 4x cost |
| Jiles-Atherton | 100-150 | 64 bytes | Moderate |
| Full tube amp model | 500-2000 | 2KB+ | Challenging |

### 8.2 Optimization Strategies

```cpp
// 1. Lookup Tables for transcendentals
class FastTanh {
    static constexpr int TABLE_SIZE = 4096;
    static constexpr float RANGE = 4.0f;
    float table[TABLE_SIZE];

public:
    FastTanh() {
        for (int i = 0; i < TABLE_SIZE; i++) {
            float x = (i / (float)TABLE_SIZE * 2.0f - 1.0f) * RANGE;
            table[i] = std::tanh(x);
        }
    }

    float operator()(float x) {
        x = std::max(-RANGE, std::min(RANGE, x));
        float idx = (x / RANGE * 0.5f + 0.5f) * (TABLE_SIZE - 1);
        int i = (int)idx;
        float frac = idx - i;
        return table[i] * (1.0f - frac) + table[i+1] * frac;
    }
};

// 2. SIMD for parallel processing
// Process 4 samples at once using SSE/NEON

// 3. Reduced iteration counts
// Many Newton-Raphson loops can use 2-4 iterations
// with good initial guesses from previous sample

// 4. Coefficient caching
// Precompute all filter coefficients on parameter change

// 5. Block processing
// Process in blocks of 32-128 samples to amortize overhead
```

### 8.3 Quality vs Performance Trade-offs

| Quality Level | Approach | CPU Cost | Use Case |
|---------------|----------|----------|----------|
| Basic | Waveshaping + filters | Low | Live/real-time |
| Good | WDF + simple NL | Medium | DAW plugins |
| High | Full state-space | High | Premium plugins |
| Reference | Oversampled + full model | Very High | Offline rendering |

---

## 9. References and Further Reading

### 9.1 Foundational Papers

1. **Fettweis, A.** (1971) - "Digital Filter Structures Related to Classical Filter Networks"
   - Original Wave Digital Filter paper

2. **Karjalainen, M., et al.** (2006) - "Wave Digital Modeling of the Output Chain of a Vacuum-Tube Amplifier"
   - Practical WDF application to tube amps

3. **Yeh, D.** (2008) - "Digital Implementation of Musical Distortion Circuits by Analysis and Simulation"
   - PhD thesis covering multiple modeling approaches

4. **Zavalishin, V.** - "The Art of VA Filter Design"
   - Comprehensive virtual analog filter resource (Native Instruments)

5. **Jiles, D. & Atherton, D.** (1986) - "Theory of Ferromagnetic Hysteresis"
   - Foundation for tape saturation modeling

### 9.2 Books

1. **Smith, J.O.** - "Physical Audio Signal Processing"
   - Stanford CCRMA, free online at ccrma.stanford.edu

2. **Zolzer, U.** (ed.) - "DAFX: Digital Audio Effects"
   - Comprehensive audio DSP textbook

3. **Pirkle, W.** - "Designing Audio Effect Plugins in C++"
   - Practical implementation guide

### 9.3 Software Resources

1. **SPICE simulators** - LTspice, ngspice for circuit verification
2. **Faust** - Functional audio DSP language
3. **RT-WDF** - Open-source Wave Digital Filter library
4. **JUCE** - Industry-standard audio plugin framework

### 9.4 Key Implementation Notes

1. **Always oversample** distortion by at least 2x, preferably 4x
2. **Use ADAA** for waveshaping to reduce aliasing without full oversampling
3. **Limit Newton-Raphson iterations** - 4-8 is usually sufficient
4. **Cache previous values** for better initial guesses in iterative solvers
5. **Test with sine sweeps** to verify frequency response
6. **Use lookahead** in compressors to avoid click artifacts
7. **Dither** when reducing bit depth

---

## Appendix A: Complete Example - Tube Screamer Diode Clipper

```cpp
// Full Tube Screamer-style overdrive with WDF diode clipper
class TubeScreamer {
    // Input buffer
    float inputBuffer[4] = {0};
    int bufferIndex = 0;

    // Component values (typical TS808)
    const float R1 = 4700.0f;
    const float C1 = 0.047e-6f;
    const float R2 = 51000.0f;
    const float R3 = 10000.0f;  // Drive pot max
    const float C2 = 0.047e-6f;
    const float Is = 2.52e-9f;  // 1N914 diode
    const float Vt = 0.026f;
    const float n = 1.752f;

    // State
    float opampState = 0.0f;
    float outputFilterState = 0.0f;

    // Parameters
    float driveAmount = 0.5f;  // 0-1
    float toneAmount = 0.5f;   // 0-1
    float levelAmount = 0.5f;  // 0-1

    float sampleRate = 48000.0f;

public:
    void prepare(float fs) {
        sampleRate = fs;
    }

    void setDrive(float d) { driveAmount = d; }
    void setTone(float t) { toneAmount = t; }
    void setLevel(float l) { levelAmount = l; }

    float process(float input) {
        // Input HPF (removes DC, shapes low end)
        float inputFiltered = input - inputBuffer[bufferIndex];
        inputBuffer[bufferIndex] = input * 0.01f + inputBuffer[bufferIndex] * 0.99f;
        bufferIndex = (bufferIndex + 1) & 3;

        // Op-amp gain stage with diode feedback
        float driveR = R3 * (1.0f - driveAmount) + 500.0f;  // 500R to 10.5kR
        float gain = 1.0f + driveR / R2;

        float driven = inputFiltered * gain;

        // Diode clipper (Newton-Raphson)
        float v = driven;
        for (int i = 0; i < 8; i++) {
            float i_d1 = Is * (std::exp(v / (n * Vt)) - 1.0f);
            float i_d2 = Is * (std::exp(-v / (n * Vt)) - 1.0f);
            float i_total = i_d1 - i_d2;

            float di_d1 = Is * std::exp(v / (n * Vt)) / (n * Vt);
            float di_d2 = -Is * std::exp(-v / (n * Vt)) / (n * Vt);
            float di_total = di_d1 - di_d2;

            float error = v + driveR * i_total - driven;
            float deriv = 1.0f + driveR * di_total;

            if (std::abs(deriv) < 1e-10f) break;
            v = v - error / deriv;
        }

        float clipped = v;

        // Tone control (simple one-pole LPF blend)
        float toneFreq = 500.0f + toneAmount * 4000.0f;  // 500Hz to 4.5kHz
        float toneCoeff = std::exp(-2.0f * M_PI * toneFreq / sampleRate);
        outputFilterState = toneCoeff * outputFilterState + (1.0f - toneCoeff) * clipped;

        float toned = clipped * toneAmount + outputFilterState * (1.0f - toneAmount);

        // Output level
        float output = toned * levelAmount * 2.0f;

        return output;
    }
};
```

---

## Appendix B: Typical Parameter Ranges

### Diode Saturation
| Parameter | Typical Range | Unit |
|-----------|---------------|------|
| Is (saturation current) | 1e-15 to 1e-6 | A |
| n (ideality factor) | 1.0 to 2.0 | - |
| Vf (forward voltage) | 0.2 to 0.7 | V |

### Tube Stages
| Parameter | Typical Range | Unit |
|-----------|---------------|------|
| mu (amplification) | 20 to 100 | - |
| Rp (plate resistance) | 10k to 100k | Ohm |
| Vp (plate voltage) | 100 to 400 | V |

### Compressors
| Parameter | Typical Range | Unit |
|-----------|---------------|------|
| Attack | 0.02 to 100 | ms |
| Release | 10 to 5000 | ms |
| Ratio | 1.5:1 to inf:1 | - |
| Threshold | -60 to 0 | dB |
| Knee | 0 to 20 | dB |

### Tape
| Parameter | Typical Range | Unit |
|-----------|---------------|------|
| Bias | 50 to 150 | kHz |
| Record level | -6 to +6 | dB |
| Flutter rate | 0.1 to 10 | Hz |
| Flutter depth | 0.01 to 0.5 | % |
| Wow rate | 0.1 to 2 | Hz |
| Wow depth | 0.05 to 1.0 | % |

---

*End of Research Report*
