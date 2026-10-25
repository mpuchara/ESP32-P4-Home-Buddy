// SPDX-License-Identifier: Apache-2.0
#include "hb_voice.h"
#include "voice_platform.hpp"
#include <cstdio>
static int16_t pcm[HB_VOICE_SAMPLES];
static hb_voice_embedding output;
static hb_voice_metrics metrics;
static bool cancelled(void *)
{
    return true;
}
static bool cancel_stage(void *)
{
    return fake::stage >= 3;
}
static esp_err_t run(hb_voice_cancel_fn cancel = nullptr)
{
    return hb_voice_embedding_run(pcm, HB_VOICE_SAMPLES, 16000, cancel, nullptr, &output, &metrics);
}
int main()
{
    for (unsigned i = 0; i < HB_VOICE_SAMPLES; ++i)
        pcm[i] = (int16_t)((i * 971) % 6000 - 3000);
    fake::reset();
    assert(run() == ESP_OK);
    assert(output.v[0] > 0.9f);
    fake::reset();
    assert(run(cancelled) == ESP_ERR_INVALID_STATE);
    fake::reset();
    assert(run(cancel_stage) == ESP_ERR_INVALID_STATE);
    assert(output.v[0] == 0);
    fake::reset();
    fake::alias = true;
    assert(run() == ESP_OK);
    fake::reset();
    fake::compute = false;
    assert(run() == ESP_ERR_TIMEOUT);
    fake::reset();
    fake::small = true;
    assert(run() == ESP_ERR_NO_MEM);
    fake::reset();
    fake::partition = false;
    assert(run() == ESP_ERR_NOT_FOUND);
    fake::reset();
    fake::bad_hash = true;
    assert(run() == ESP_ERR_INVALID_CRC);
    fake::reset();
    fake::shape = true;
    assert(run() == ESP_ERR_INVALID_VERSION);
    fake::reset();
    fake::model_new_fail = true;
    assert(run() == ESP_ERR_NO_MEM);
    fake::reset();
    fake::model_fail = true;
    assert(run() == ESP_ERR_INVALID_VERSION);
    fake::reset();
    fake::zero = true;
    assert(run() == ESP_FAIL);
    assert(output.v[0] == 0);
    fake::reset();
    for (int i = 0; i < 6; ++i)
    {
        fake::fail_at = i;
        assert(run() == ESP_ERR_NO_MEM);
        fake::reset();
    }
    for (int i = 0; i < 20; ++i)
    {
        assert(run() == ESP_OK);
        fake::reset();
    }
    assert(hb_voice_embedding_run(pcm, 48000, 16000, nullptr, nullptr, &output, &metrics) ==
           ESP_ERR_INVALID_ARG);
    memset(pcm, 0, sizeof(pcm));
    assert(run() == ESP_ERR_INVALID_ARG);
    fake::reset();
    puts("Real adapter with mocked platform: allocation/load/hash/tensor/output/cancel failures "
         "release resources");
}
