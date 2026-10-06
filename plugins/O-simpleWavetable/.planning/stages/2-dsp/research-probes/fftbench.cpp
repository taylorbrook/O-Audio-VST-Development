#include <Accelerate/Accelerate.h>
#include <chrono>
#include <cstdio>
#include <vector>
#include <cmath>
int main() {
    const int order = 11, N = 1 << order;
    FFTSetup s = vDSP_create_fftsetup(order, 2);
    std::vector<float> buf(2 * N), work(2 * N);
    std::vector<float> bank((size_t)5 * 32 * 11 * 2049);   // 14.4 MB like BuiltInBanks
    auto t0 = std::chrono::steady_clock::now();
    for (int b = 0; b < 5; ++b) for (int f = 0; f < 32; ++f) {
        for (int i = 0; i < 2 * N; ++i) buf[i] = 0.f;
        for (int n = 1; n < 1024; ++n) buf[2*n+1] = -0.5f * N / n;      // spectrum build
        for (int L = 0; L < 11; ++L) {
            const int kmax = std::min(1023, 1024 >> L);
            std::copy(buf.begin(), buf.end(), work.begin());
            for (int n = kmax + 1; n <= 1024; ++n) work[2*n] = work[2*n+1] = 0.f;
            work[1] = work[2*1024];                                   // JUCE-style pack (Nyquist into DC imag)
            DSPSplitComplex sc { work.data(), work.data() + 1 };
            vDSP_fft_zrip(s, &sc, 2, order, kFFTDirection_Inverse);
            float sc2 = 1.0f / N; vDSP_vsmul(work.data(), 1, &sc2, work.data(), 1, N);
            float* dst = &bank[(((size_t)b * 11 + L) * 32 + f) * 2049];
            std::copy(work.begin(), work.begin() + N, dst); dst[N] = dst[0];
        }
    }
    auto ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
    std::printf("5 banks x 32 frames x 11 levels (1760 IFFTs + copies): %.2f ms (checksum %g)\n", ms, bank[12345]);
}
