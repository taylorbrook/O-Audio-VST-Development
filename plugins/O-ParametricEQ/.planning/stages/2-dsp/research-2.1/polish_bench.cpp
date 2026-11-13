// Research port (not plugin code): MLR seed + anchored Gauss-Newton polish, double, no allocation.
// Build: clang++ -O3 -std=c++17 polish_bench.cpp -o polish_bench
#include <cmath>
#include <cstdio>
#include <chrono>
#include <random>
static const double PI = 3.14159265358979323846, CDB = 10.0 / std::log(10.0);
struct MP { double P0, P1, Om, g, k, hN; };
static MP matchedPair (double w, double Q) {
    MP m; double q = 1.0 / (2.0 * Q);
    if (q < 1.0) { double r = std::exp (-q * w), th = std::fmin (w * std::sqrt (1 - q * q), PI), s = std::sin (th / 2), s2 = s * s, e = std::expm1 (-q * w);
        m.P0 = e * e + 4 * r * s2; m.P1 = (1 + r) * (1 + r) - 4 * r * s2; }
    else { double d = w * std::sqrt (q * q - 1), e1 = -std::expm1 (-q * w + d), e2 = -std::expm1 (-q * w - d); m.P0 = e1 * e2; m.P1 = (2 - e1) * (2 - e2); }
    if (m.P1 < 1e-12) m.P1 = 1e-12;
    m.Om = -std::expm1 (-2 * q * w); m.g = std::sqrt (m.P0 / m.P1); m.k = 2 * m.Om / std::sqrt (m.P0 * m.P1);
    double f = w / PI; m.hN = f * f / std::sqrt ((1 - f * f) * (1 - f * f) + f * f / (Q * Q)); return m; }
// p = g,k,mL,mB,mH,G,cL,cH
static void mlr (double wz, double Qz, double wp, double Qp, double c, double* p) {
    MP N = matchedPair (wz, Qz), D = matchedPair (wp, Qp);
    p[0] = D.g; p[1] = D.k; p[2] = N.P0 / D.P0; p[3] = 2 * N.Om / std::sqrt (D.P0 * D.P1); p[4] = N.P1 / D.P1;
    p[5] = N.g * N.g / N.hN; p[6] = c * D.P0 / N.P0; p[7] = c * (D.hN * D.P1) / (N.hN * N.P1); }
enum { M = 16, NT = 10, NP = M + NT, NF = 6 };
struct Grid { double w[M], t[M]; double flo, fhi, fs; };
static void prepare (Grid& g, double fs) { g.fs = fs; g.flo = 20; g.fhi = std::fmin (20000.0, 0.499 * fs);
    for (int i = 0; i < M; ++i) { double f = g.flo * std::pow (g.fhi / g.flo, i / double (M - 1)); g.w[i] = 2 * PI * f / fs; double tn = std::tan (g.w[i] / 2); g.t[i] = tn * tn; } }
static inline double sstep (double x) { x = x < 0 ? 0 : (x > 1 ? 1 : x); return x * x * (3 - 2 * x); }
struct Work { double t[NP], tl[NP]; };   // tan^2 and ln(target power)
static inline void evalRow (const double* e, double t, double& L, double* J) {
    double g = e[0], k = e[1], mL = e[2], mB = e[3], mH = e[4], G = e[5], cL = e[6], cH = e[7];
    double X = t / (g * g), Y = t / (G * G), u = mL - mH * X, Nn = u * u + mB * mB * X, Dn = (1 - X) * (1 - X) + k * k * X, N1 = cL * cL + cH * cH * Y, D1 = 1 + Y;
    L = std::log ((Nn * N1) / (Dn * D1));
    if (J) { double iN = 1 / Nn, iD = 1 / Dn, i1 = 1 / N1;
        J[0] = -2 * X * ((-2 * mH * u + mB * mB) * iN - (-2 * (1 - X) + k * k) * iD); J[1] = -2 * k * k * X * iD; J[2] = 2 * mB * mB * X * iN;
        J[3] = -2 * mH * X * u * iN; J[4] = -2 * Y * (cH * cH * i1 - 1 / D1); J[5] = 2 * cH * cH * Y * i1; } }
static const int FREE[NF] = { 0, 1, 3, 4, 5, 7 };
static double gnStep (const Work& wk, const double* phi0, double* phi, double mu, double lam) {   // returns S (dB^2) before the step
    double e[8]; for (int i = 0; i < 8; ++i) e[i] = std::exp (phi[i]);
    double A[NF][NF] = {}, b[NF] = {}, S = 0;
    for (int j = 0; j < NP; ++j) { double L, J[NF]; evalRow (e, wk.t[j], L, J); double r = CDB * (L - wk.tl[j]); S += r * r;
        for (int a = 0; a < NF; ++a) { double ja = CDB * J[a]; b[a] += ja * r; for (int c = 0; c <= a; ++c) A[a][c] += ja * CDB * J[c]; } }
    for (int a = 0; a < NF; ++a) { A[a][a] += lam * A[a][a] + mu; b[a] += mu * (phi[FREE[a]] - phi0[FREE[a]]); }
    for (int a = 0; a < NF; ++a) { for (int c = 0; c <= a; ++c) { double s = A[a][c]; for (int q = 0; q < c; ++q) s -= A[a][q] * A[c][q]; A[a][c] = (a == c) ? std::sqrt (s) : s / A[c][c]; } }
    for (int a = 0; a < NF; ++a) { double s = b[a]; for (int q = 0; q < a; ++q) s -= A[a][q] * b[q]; b[a] = s / A[a][a]; }
    for (int a = NF - 1; a >= 0; --a) { double s = b[a]; for (int q = a + 1; q < NF; ++q) s -= A[q][a] * b[q]; b[a] = s / A[a][a]; }
    for (int a = 0; a < NF; ++a) phi[FREE[a]] -= b[a];
    return S; }
static double objective (const Work& wk, const double* phi) { double e[8]; for (int i = 0; i < 8; ++i) e[i] = std::exp (phi[i]);
    double S = 0; for (int j = 0; j < NP; ++j) { double L; evalRow (e, wk.t[j], L, nullptr); double r = CDB * (L - wk.tl[j]); S += r * r; } return S; }
// returns blend weight; out = polished log-params
static double polish (const Grid& g, double wz, double Qz, double wp, double Qp, double c, const double* seed, double* out, int K, int Kmax, int* extra) {
    Work wk; double phi0[8], phi[8]; for (int i = 0; i < 8; ++i) phi0[i] = phi[i] = std::log (seed[i]);
    double w[NP]; for (int i = 0; i < M; ++i) { w[i] = g.w[i]; wk.t[i] = g.t[i]; }
    const double sc[2] = { 0.5, 1.5 }, wlo = 2 * PI * g.flo / g.fs, whi = 2 * PI * g.fhi / g.fs; int n = M;
    for (int s = 0; s < 2; ++s) { double w0 = s ? wp : wz, q = s ? Qp : Qz; w[n++] = w0;
        for (int i = 0; i < 2; ++i) { double h = sc[i] / (2 * q), rt = std::sqrt (1 + h * h); w[n++] = w0 * (rt + h); w[n++] = w0 * (rt - h); } }
    for (int j = M; j < NP; ++j) { w[j] = std::fmin (std::fmax (w[j], wlo), whi); double cs = std::cos (w[j]); wk.t[j] = (1 - cs) / (1 + cs); }
    double wz2 = wz * wz, wp2 = wp * wp, kz = wz2 / (Qz * Qz), kp = wp2 / (Qp * Qp), sc2 = c * c * (wp2 * wp2) / (wz2 * wz2);
    for (int j = 0; j < NP; ++j) { double x = w[j] * w[j]; wk.tl[j] = std::log (sc2 * ((wz2 - x) * (wz2 - x) + kz * x) / ((wp2 - x) * (wp2 - x) + kp * x)); }
    double S0 = 0; for (int it = 0; it < K; ++it) { double S = gnStep (wk, phi0, phi, 0.1, 1e-3); if (it == 0) S0 = S; }
    const double Sa = 0.002, Sb = 0.006, Sx = 0.03;
    if (S0 / NP <= Sa * Sa) { for (int i = 0; i < 8; ++i) out[i] = phi0[i]; return 0; }       // (in the plugin this exit comes after ONE objective pass, before any step)
    double S1 = objective (wk, phi);
    if (Kmax > K) { double wx = sstep ((std::sqrt (S1 / NP) - Sx) / Sx);
        if (wx > 0) { if (extra) ++*extra; double p2[8]; for (int i = 0; i < 8; ++i) p2[i] = phi[i]; for (int it = K; it < Kmax; ++it) gnStep (wk, phi0, p2, 0.1, 1e-3);
            bool ok = true; for (int i = 0; i < 8; ++i) ok = ok && std::isfinite (p2[i]); if (ok) { for (int i = 0; i < 8; ++i) phi[i] += wx * (p2[i] - phi[i]); S1 = objective (wk, phi); } } }
    bool ok = std::isfinite (S1) && std::isfinite (S0); for (int i = 0; i < 8; ++i) ok = ok && std::isfinite (phi[i]);
    double l = ok ? sstep ((S0 / NP - Sa * Sa) / (Sb * Sb - Sa * Sa)) * sstep ((1 - S1 / S0) / 0.1) : 0;
    for (int i = 0; i < 8; ++i) out[i] = phi0[i] + l * ((ok ? phi[i] : phi0[i]) - phi0[i]);
    return l; }
int main (int argc, char** argv) {
    Grid g; prepare (g, 44100.0);
    if (argc > 1) {   // dump mode for the Python cross-check: bells, fs 44.1k
        for (double fc : { 30.0, 1000.0, 9000.0, 15000.0, 19500.0 }) for (double Q : { 0.025, 0.7, 4.0, 40.0 }) for (double gd : { 3.0, 12.0, 30.0 }) {
            double A = std::pow (10, gd / 40), wc = 2 * PI * fc / 44100.0, s[8], o[8]; mlr (wc, Q / A, wc, Q * A, 1, s); double l = polish (g, wc, Q / A, wc, Q * A, 1, s, o, 2, 4, nullptr);
            std::printf ("%.17g %.17g %.17g %.17g", fc, Q, gd, l); for (int i = 0; i < 8; ++i) std::printf (" %.17g", std::log (s[i])); for (int i = 0; i < 8; ++i) std::printf (" %.17g", o[i]); std::printf ("\n"); }
        return 0; }
    std::mt19937_64 rng (3); std::uniform_real_distribution<double> U (0, 1); const int N = 400000;
    static double fc[N], Q[N], gd[N];
    for (double fs : { 44100.0, 48000.0, 96000.0 }) for (int region = 0; region < 3; ++region) {
        prepare (g, fs);
        for (int i = 0; i < N; ++i) { double lo = region == 0 ? 20 : (region == 1 ? 20 : 5000), hi = region == 0 ? 20000 : (region == 1 ? 2000 : 20000);
            fc[i] = lo * std::pow (hi / lo, U (rng)); Q[i] = 0.025 * std::pow (1600.0, U (rng)); gd[i] = 1 + 29 * U (rng); }
        double acc = 0; auto t0 = std::chrono::steady_clock::now ();
        for (int i = 0; i < N; ++i) { double A = std::pow (10, gd[i] / 40), wc = 2 * PI * fc[i] / fs, s[8]; mlr (wc, Q[i] / A, wc, Q[i] * A, 1, s); acc += s[3]; }
        auto t1 = std::chrono::steady_clock::now (); int ex = 0, pol = 0;
        for (int i = 0; i < N; ++i) { double A = std::pow (10, gd[i] / 40), wc = 2 * PI * fc[i] / fs, s[8], o[8]; mlr (wc, Q[i] / A, wc, Q[i] * A, 1, s); pol += polish (g, wc, Q[i] / A, wc, Q[i] * A, 1, s, o, 2, 4, &ex) > 0; acc += o[3]; }
        auto t2 = std::chrono::steady_clock::now ();
        double ns1 = std::chrono::duration<double, std::nano> (t1 - t0).count () / N, ns2 = std::chrono::duration<double, std::nano> (t2 - t1).count () / N;
        std::printf ("fs %.0f bells fc %s: seed %.0f ns, seed+polish %.0f ns (polish %.0f ns); polished share %.2f, extra-iteration share %.3f  [%g]\n", fs,
                     region == 0 ? "20-20k" : (region == 1 ? "20-2k " : "5k-20k"), ns1, ns2, ns2 - ns1, pol / double (N), ex / double (N), acc); }
    return 0; }
