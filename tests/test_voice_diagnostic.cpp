// SPDX-License-Identifier: Apache-2.0
#include "voice_diagnostic_platform.hpp"
#include <cstdio>
int main()
{
    hb_voice_capture_result result{};
    hb_voice_readiness report{};
    assert(!hb_voice_diagnostic_report(&report));
    assert(!hb_voice_diagnostic_standalone_report(&report));
    diag_fake::reset();
    assert(hb_voice_capture_start(2, 7) == ESP_ERR_INVALID_ARG);
    assert(hb_voice_capture_start(0, 0) == ESP_ERR_INVALID_ARG);
    assert(hb_voice_capture_start(0, 7) == ESP_OK);
    assert(hb_voice_capture_start(0, 7) == ESP_ERR_INVALID_STATE);
    diag_fake::run();
    assert(diag_fake::detachments == 1 && diag_fake::model_calls == 1);
    assert(hb_voice_diagnostic_report(&report) && report.error == ESP_OK);
    assert(report.slot == 0 && report.sample_rate == 16000 && report.samples == HB_VOICE_SAMPLES);
    assert(report.dimensions == 512 && report.rms == 1700 && report.peak == 3000);
    assert(report.active_samples == HB_VOICE_SAMPLES);
    assert(report.callback_detached && report.pcm_released);
    assert(hb_voice_capture_start(0, 7) ==
           ESP_ERR_INVALID_STATE); // Pending result cannot be overwritten.
    assert(hb_voice_capture_take(&result) && result.epoch == 7 && result.embedding.v[0] == 1);
    assert(result.quality == 1.0f);
    assert(!hb_voice_capture_take(&result));
    for (unsigned i = 0; i < 10; ++i)
    {
        diag_fake::reset();
        assert(hb_voice_diagnostic_start(0) == ESP_OK);
        diag_fake::run();
        assert(!hb_voice_capture_take(&result));
    }
    assert(hb_voice_diagnostic_standalone_report(&report) && report.standalone);
    diag_fake::reset();
    assert(hb_voice_capture_start(0, 7) == ESP_OK);
    diag_fake::run();
    assert(hb_voice_capture_take(&result));
    assert(hb_voice_diagnostic_standalone_report(&report) && report.standalone);
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
    assert(result.no_window);
    diag_fake::reset();
    assert(hb_voice_capture_start_auto(0, 7) == ESP_OK);
    diag_fake::run();
    assert(hb_voice_capture_take(&result) && result.no_window && !diag_fake::model_calls);
    diag_fake::reset();
    diag_fake::speech_like = diag_fake::auto_drain = true;
    assert(hb_voice_capture_start_auto(0, 7) == ESP_OK);
    diag_fake::run();
    assert(diag_fake::drained == 1 && diag_fake::first.more);
    assert(diag_fake::first.error == ESP_OK && diag_fake::first.quality == 1);
    assert(hb_voice_capture_take(&result) && result.error == ESP_OK && result.quality == 1);
    assert(!result.more && result.end_ms > diag_fake::first.end_ms);
    assert(diag_fake::model_calls == 2 && diag_fake::during_inference == 2);
    assert(!hb_voice_capture_busy() && !hb_voice_capture_result_pending());
    diag_fake::reset();
    diag_fake::model_error = ESP_ERR_NO_MEM;
    assert(hb_voice_capture_start(0, 7) == ESP_OK);
    diag_fake::run();
    assert(hb_voice_capture_take(&result) && result.error == ESP_ERR_NO_MEM &&
           result.embedding.v[0] == 0);
    diag_fake::reset();
    diag_fake::invalid_embedding = true;
    assert(hb_voice_capture_start(1, 7) == ESP_OK);
    assert(!hb_voice_diagnostic_report(&report));
    diag_fake::run();
    assert(hb_voice_capture_take(&result) && result.error == ESP_ERR_INVALID_STATE);
    assert(hb_voice_diagnostic_report(&report) && report.dimensions == 0 && report.slot == 1);
    assert(report.callback_detached && report.pcm_released);
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
