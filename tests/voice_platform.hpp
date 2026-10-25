// SPDX-License-Identifier: Apache-2.0
// Fault injection for the actual embedding adapter; not a numerical FFT/model oracle.
#pragma once
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <map>
#include <new>
#include <string>
#include <vector>
#define CONFIG_HB_IDENTITY_EXPERIMENTAL 1
#define MALLOC_CAP_INTERNAL 1
#define MALLOC_CAP_8BIT 2
#define MALLOC_CAP_SPIRAM 4
#define ESP_PARTITION_TYPE_DATA 1
#define ESP_PARTITION_MMAP_DATA 1
#define SPI_FLASH_MMAP_DATA 1
#define ESP_ERR_INVALID_ARG 0x102
#define ESP_ERR_INVALID_STATE 0x103
#define ESP_ERR_INVALID_SIZE 0x104
#define ESP_ERR_NOT_FOUND 0x105
#define ESP_ERR_NOT_SUPPORTED 0x106
#define ESP_ERR_TIMEOUT 0x107
#define ESP_ERR_INVALID_VERSION 0x108
#define ESP_ERR_INVALID_CRC 0x109
namespace fake
{
inline bool compute = true, small = false, partition = true, bad_hash = false, shape = false,
            zero = false, model_fail = false;
inline bool model_new_fail = false, alias = false;
inline unsigned feature_frames = 0;
inline int allocations = 0, fail_at = -1, live = 0, maps = 0, leases = 0, models = 0, stage = 0;
inline unsigned char flash[0x300000];
inline void reset()
{
    assert(live == 0 && maps == 0 && leases == 0 && models == 0);
    compute = true;
    small = false;
    partition = true;
    bad_hash = false;
    shape = false;
    zero = false;
    model_fail = false;
    model_new_fail = false;
    alias = false;
    feature_frames = 0;
    allocations = 0;
    fail_at = -1;
    stage = 0;
}
} // namespace fake
inline void *heap_caps_malloc(size_t n, unsigned)
{
    if (fake::allocations++ == fake::fail_at)
        return nullptr;
    void *p = malloc(n);
    if (p)
        ++fake::live;
    return p;
}
inline void *heap_caps_aligned_alloc(size_t, size_t n, unsigned caps)
{
    return heap_caps_malloc(n, caps);
}
// Adapter calls free() on capability allocations in firmware. Redirect only here.
inline void tracked_free(void *p)
{
    if (p)
    {
        --fake::live;
        std::free(p);
    }
}
#define free tracked_free
inline size_t heap_caps_get_free_size(unsigned)
{
    return fake::small ? 1 : 16 * 1024 * 1024;
}
inline size_t heap_caps_get_largest_free_block(unsigned)
{
    return fake::small ? 1 : 16 * 1024 * 1024;
}
inline bool hb_compute_enter(unsigned)
{
    if (fake::compute)
        ++fake::leases;
    return fake::compute;
}
inline void hb_compute_leave()
{
    --fake::leases;
}
inline unsigned uxTaskGetStackHighWaterMark(void *)
{
    return 1024;
}
inline void vTaskDelay(int)
{
}
inline int64_t esp_timer_get_time()
{
    static int64_t t = 0;
    return ++t;
}
typedef unsigned esp_partition_mmap_handle_t;
typedef int esp_partition_subtype_t;
struct esp_partition_t
{
    size_t size = 0x300000;
    size_t address = 0xc20000;
    bool encrypted = false;
};
inline const esp_partition_t *esp_partition_find_first(int, int, const char *)
{
    static esp_partition_t p;
    return fake::partition ? &p : nullptr;
}
inline size_t spi_flash_mmap_get_free_pages(int)
{
    return 256;
}
inline int esp_partition_mmap(const esp_partition_t *, int, size_t, int, const void **out,
                              unsigned *handle)
{
    *out = fake::flash;
    *handle = 1;
    ++fake::maps;
    return 0;
}
inline void esp_partition_munmap(unsigned)
{
    --fake::maps;
}
struct mbedtls_sha256_context
{
};
inline void mbedtls_sha256_init(mbedtls_sha256_context *)
{
}
inline int mbedtls_sha256_starts(mbedtls_sha256_context *, int)
{
    return 0;
}
inline int mbedtls_sha256_update(mbedtls_sha256_context *, const uint8_t *, size_t)
{
    return 0;
}
inline int mbedtls_sha256_finish(mbedtls_sha256_context *, uint8_t *out)
{
    const uint8_t hash[32] = {0xb5, 0x03, 0xff, 0x04, 0x9a, 0x0c, 0xd5, 0xc6, 0x2b, 0x4c, 0x1a,
                              0xc3, 0x37, 0xcf, 0x20, 0xb7, 0xde, 0x15, 0x49, 0x85, 0x7a, 0x4d,
                              0x53, 0x29, 0xa1, 0x82, 0x1c, 0xfb, 0x55, 0xe9, 0xb3, 0xa8};
    memcpy(out, hash, 32);
    if (fake::bad_hash)
        out[0] ^= 1;
    return 0;
}
inline void mbedtls_sha256_free(mbedtls_sha256_context *)
{
}
struct dl_fft_f32_t
{
    void *bitrev_table;
};
inline dl_fft_f32_t *dl_rfft_f32_init(int, unsigned caps)
{
    auto *p = (dl_fft_f32_t *)heap_caps_malloc(sizeof(dl_fft_f32_t), caps);
    if (p)
        p->bitrev_table = p;
    return p;
}
inline void dl_rfft_f32_deinit(dl_fft_f32_t *p)
{
    tracked_free(p);
}
inline int dl_rfft_f32_run(dl_fft_f32_t *, float *)
{
    return 0;
}
namespace fbs
{
enum
{
    MODEL_LOCATION_IN_FLASH_RODATA
};
}
namespace dl
{
enum
{
    MEMORY_MANAGER_GREEDY,
    DATA_TYPE_INT16,
    DATA_TYPE_INT8
};
struct Scale
{
    int value = -7;
    operator int() const
    {
        return value;
    }
    bool is_valid() const
    {
        return true;
    }
    bool is_per_channel() const
    {
        return false;
    }
};
struct Tensor
{
    void *data = nullptr;
    std::vector<int> shape;
    int dtype = 0, size = 0;
    Scale exponent;
};
class Model
{
    int16_t input_data[24000]{};
    int8_t output_data[512]{};
    Tensor in, out;
    std::map<std::string, Tensor *> inputs, outputs;

  public:
    static void *operator new(size_t n, const std::nothrow_t &) noexcept
    {
        return fake::model_new_fail ? nullptr : ::operator new(n, std::nothrow);
    }
    static void operator delete(void *p) noexcept
    {
        ::operator delete(p);
    }
    static void operator delete(void *p, const std::nothrow_t &) noexcept
    {
        ::operator delete(p);
    }
    Model(const char *, int, int, int, const uint8_t *, bool)
    {
        ++fake::models;
        in.data = input_data;
        in.shape = {1, 300, 80};
        in.dtype = DATA_TYPE_INT16;
        in.size = 24000;
        out.data = fake::alias ? (void *)input_data : (void *)output_data;
        out.shape = {1, 512};
        out.dtype = DATA_TYPE_INT8;
        out.size = 512;
        if (fake::shape)
            in.shape = {1, 299, 80};
        if (!fake::model_fail)
        {
            inputs["input"] = &in;
            outputs["output"] = &out;
        }
    }
    ~Model()
    {
        --fake::models;
    }
    auto &get_inputs()
    {
        return inputs;
    }
    auto &get_outputs()
    {
        return outputs;
    }
    void run(int stages, int stage)
    {
        if (fake::alias && stage == 0)
            assert(input_data[0] != 0);
        fake::stage = stage + 1;
        if (stage == stages - 1 && !fake::zero)
        {
            ((int8_t *)out.data)[0] = 100;
            ((int8_t *)out.data)[1] = 20;
        }
    }
};
namespace audio
{
enum class WinType
{
    HAMMING
};
struct mel_filter_t
{
    float *coeff = nullptr;
    int *bank_pos = nullptr;
    int nfilter = 0;
};
inline float hz2mel(float x)
{
    return 1127 * logf(1 + x / 700);
}
inline float *win_func_init(WinType, int n, unsigned caps)
{
    auto *p = (float *)heap_caps_malloc(n * sizeof(float), caps);
    if (p)
        for (int i = 0; i < n; ++i)
            p[i] = 1;
    return p;
}
inline void remove_dc_offset(float *, int)
{
}
inline void apply_preemphasis(float *, int, float, float)
{
}
inline void apply_window(float *, int, float *)
{
}
inline void compute_spectrum(float *, int, bool)
{
}
inline void mel_dotprod(float *, mel_filter_t *, float *out)
{
    for (int i = 0; i < 80; ++i)
        out[i] = fake::feature_frames % 2 ? 2.0f : 1.0f;
    ++fake::feature_frames;
}
} // namespace audio
} // namespace dl
