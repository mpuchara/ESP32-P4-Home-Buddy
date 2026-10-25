// SPDX-License-Identifier: Apache-2.0
#include "voice_diagnostic_platform.hpp"
#include <cstdio>
int main()
{
    hb_voice_capture_result result{};
    diag_fake::reset();
    assert(hb_voice_capture_start(2, 7) == ESP_ERR_INVALID_ARG);
    assert(hb_voice_capture_start(0, 0) == ESP_ERR_INVALID_ARG);
    assert(hb_voice_capture_start(0, 7) == ESP_OK);
    assert(hb_voice_capture_start(0, 7) == ESP_ERR_INVALID_STATE);
    diag_fake::run();
    assert(diag_fake::detachments == 1 && diag_fake::model_calls == 1);
    assert(hb_voice_capture_start(0, 7) ==
           ESP_ERR_INVALID_STATE); // Pending result cannot be overwritten.
    assert(hb_voice_capture_take(&result) && result.epoch == 7 && result.embedding.v[0] == 1);
    assert(!hb_voice_capture_take(&result));
    for (unsigned i = 0; i < 10; ++i)
    {
        diag_fake::reset();
        assert(hb_voice_diagnostic_start(0) == ESP_OK);
        diag_fake::run();
        assert(!hb_voice_capture_take(&result));
    }
    diag_fake::reset();
    assert(hb_voice_capture_start(0, 7) == ESP_OK);
    hb_voice_diagnostic_stop();
    diag_fake::run();
    assert(!hb_voice_capture_take(&result));
    assert(!diag_fake::model_calls && !diag_fake::consumer);
    diag_fake::reset();
    diag_fake::cancel_inference = true;
    assert(hb_voice_capture_start(0, 7) == ESP_OK);
    diag_fake::run();
    assert(!hb_voice_capture_take(&result));
    diag_fake::reset();
    diag_fake::alloc_fail = true;
    assert(hb_voice_capture_start(0, 7) == ESP_OK);
    diag_fake::run();
    assert(hb_voice_capture_take(&result) && result.error == ESP_ERR_NO_MEM &&
           result.embedding.v[0] == 0);
    diag_fake::reset();
    diag_fake::start_fail = true;
    assert(hb_voice_capture_start(0, 7) == ESP_OK);
    diag_fake::run();
    assert(hb_voice_capture_take(&result) && result.error == ESP_ERR_INVALID_STATE);
    diag_fake::reset();
    diag_fake::silence = true;
    assert(hb_voice_capture_start(0, 7) == ESP_OK);
    diag_fake::run();
    assert(hb_voice_capture_take(&result) && result.error != ESP_OK && !diag_fake::model_calls);
    diag_fake::reset();
    diag_fake::model_error = ESP_ERR_NO_MEM;
    assert(hb_voice_capture_start(0, 7) == ESP_OK);
    diag_fake::run();
    assert(hb_voice_capture_take(&result) && result.error == ESP_ERR_NO_MEM &&
           result.embedding.v[0] == 0);
    diag_fake::reset();
    diag_fake::task_fail = true;
    assert(hb_voice_capture_start(0, 7) == ESP_ERR_NO_MEM);
    diag_fake::reset();
    assert(hb_voice_capture_start(0, 7) == ESP_OK);
    diag_fake::run();
    hb_voice_diagnostic_stop();
    assert(!hb_voice_capture_take(&result));
    diag_fake::reset();
    puts("Real voice worker: explicit capture, one result, no overwrite, cancel, timeout, "
         "allocation/task/model errors, detach-before-scrub/free passed");
}
