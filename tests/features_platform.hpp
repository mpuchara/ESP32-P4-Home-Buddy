// SPDX-License-Identifier: Apache-2.0
// Host FFT shim: shared by reference and candidate. Does not validate P4 FFT assembly.
#pragma once
#include "hb_voice.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <complex>
#include <cstdlib>
#include <cstring>
#include <vector>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#define MALLOC_CAP_SPIRAM 0
#define MALLOC_CAP_8BIT 0
#define MALLOC_CAP_INTERNAL 0
#define DL_MIN(a, b) std::min(a, b)
#define DL_MAX(a, b) std::max(a, b)
inline void *heap_caps_malloc(size_t n, unsigned)
{
    return malloc(n);
}
inline void *heap_caps_aligned_alloc(size_t, size_t n, unsigned)
{
    return malloc(n);
}
inline void vTaskDelay(int)
{
}
struct dl_fft_f32_t
{
    void *bitrev_table;
};
inline dl_fft_f32_t *dl_rfft_f32_init(int, unsigned)
{
    auto *p = new dl_fft_f32_t;
    p->bitrev_table = p;
    return p;
}
inline void dl_rfft_f32_deinit(dl_fft_f32_t *p)
{
    delete p;
}
inline int dl_rfft_f32_run(dl_fft_f32_t *, float *x)
{
    std::complex<double> a[512];
    for (unsigned i = 0; i < 512; ++i)
        a[i] = x[i];
    for (unsigned i = 1, j = 0; i < 512; ++i)
    {
        unsigned bit = 256;
        for (; j & bit; bit >>= 1)
            j ^= bit;
        j ^= bit;
        if (i < j)
            std::swap(a[i], a[j]);
    }
    for (unsigned len = 2; len <= 512; len *= 2)
    {
        auto wlen = std::polar(1.0, -2 * 3.14159265358979323846 / len);
        for (unsigned i = 0; i < 512; i += len)
        {
            std::complex<double> w = 1;
            for (unsigned j = 0; j < len / 2; ++j)
            {
                auto u = a[i + j], v = a[i + j + len / 2] * w;
                a[i + j] = u + v;
                a[i + j + len / 2] = u - v;
                w *= wlen;
            }
        }
    }
    x[0] = (float)a[0].real();
    x[1] = (float)a[256].real();
    for (unsigned i = 1; i < 256; ++i)
    {
        x[2 * i] = (float)a[i].real();
        x[2 * i + 1] = (float)a[i].imag();
    }
    return ESP_OK;
}
namespace dl
{
namespace base
{
inline void dotprod(float *a, float *b, float *out, int n, int)
{
    *out = 0;
    for (int i = 0; i < n; ++i)
        *out += a[i] * b[i];
}
} // namespace base
namespace audio
{
enum class WinType
{
    HANNING,
    SINE,
    HAMMING,
    HANN,
    POVEY,
    RECTANGULAR,
    BLACKMAN,
    NONE
};
struct mel_filter_t
{
    float *coeff;
    int *bank_pos;
    int nfilter;
};
float *win_func_init(WinType, int, uint32_t = 0, float = 0.42f);
struct Config
{
    bool remove_dc_offset = true, raw_energy = true, use_energy = false, use_power = true;
    float log_epsilon = 1.1920928955078125e-7f, preemphasis = 0.97f;
    int use_log_fbank = 1, num_mel_bins = 80;
};
struct Fbank
{
    Config m_config;
    float m_cache[512], *m_win_func;
    int m_fft_size = 512;
    dl_fft_f32_t *m_fft_config;
    mel_filter_t *m_mel_filter;
    esp_err_t process_frame(const float *, int, float *, float = 0);
};
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif
#include "voice_reference.inc"
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
} // namespace audio
} // namespace dl
