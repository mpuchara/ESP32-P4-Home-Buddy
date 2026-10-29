// SPDX-License-Identifier: Apache-2.0
#include "hb_identity_lifecycle.h"
#include "identity_navigation.h"
#include <assert.h>
#include <stdio.h>

int main(void)
{
    assert(!hb_identity_create_timed_out(5000, false));
    assert(hb_identity_create_timed_out(5001, false));
    assert(!hb_identity_create_timed_out(5031, true));
    assert(!hb_identity_create_timed_out(6042, true));
    assert(!hb_identity_create_timed_out(30000, true));
    // Reproduce the worker/UI interleaving: the worker still reports TEST when
    // saver navigation follows an accepted BACKGROUND command immediately.
    hb_id_mode published = HB_ID_TEST;
    bool preserve = hb_identity_saver_intent(false, HB_ID_CMD_TEST, true);
    assert(!preserve);
    preserve = hb_identity_saver_intent(preserve, HB_ID_CMD_BACKGROUND, true);
    assert(published == HB_ID_TEST && preserve);
    // After worker acknowledgement, ordinary commands keep the same intent.
    published = HB_ID_BACKGROUND;
    preserve = hb_identity_saver_intent(preserve, HB_ID_CMD_EXPECTED, true);
    assert(preserve);
    // A rejected foreground request cannot change a live background session.
    preserve = hb_identity_saver_intent(preserve, HB_ID_CMD_TEST, false);
    assert(preserve);
    // Conversely, accepted TEST/STOP must cancel navigation ownership even
    // while the published snapshot still says BACKGROUND.
    preserve = hb_identity_saver_intent(preserve, HB_ID_CMD_TEST, true);
    assert(published == HB_ID_BACKGROUND && !preserve);
    preserve = hb_identity_saver_intent(preserve, HB_ID_CMD_BACKGROUND, false);
    assert(!preserve); // Queue-full BACKGROUND must not survive navigation.
    preserve = hb_identity_saver_intent(preserve, HB_ID_CMD_BACKGROUND, true);
    preserve = hb_identity_saver_intent(preserve, HB_ID_CMD_STOP, true);
    assert(!preserve);
    // Lease completion does not issue a new start or depend on mode polling.
    published = HB_ID_STOP;
    preserve = hb_identity_saver_intent(preserve, HB_ID_CMD_BACKGROUND, true);
    assert(published == HB_ID_STOP && preserve);
    const hb_id_command_type exits[] = {HB_ID_CMD_BEGIN, HB_ID_CMD_CANCEL, HB_ID_CMD_DELETE,
                                        HB_ID_CMD_SAVE};
    for (unsigned i = 0; i < sizeof(exits) / sizeof(exits[0]); ++i)
        assert(!hb_identity_saver_intent(true, exits[i], true));
    assert(hb_identity_saver_intent(false, HB_ID_CMD_AUTO, true));
    assert(!hb_identity_saver_intent(false, HB_ID_CMD_AUTO, false));
    assert(hb_id_background(HB_ID_AUTO) && hb_id_background(HB_ID_BACKGROUND));
    assert(!hb_id_background(HB_ID_TEST));
    assert(!hb_id_expired(HB_ID_AUTO, 0, UINT64_MAX));
    assert(!hb_id_expired(HB_ID_STOP, 0, UINT64_MAX));
    assert(!hb_id_expired(HB_ID_TEST, 600000, 599999));
    assert(hb_id_expired(HB_ID_TEST, 600000, 600000));
    assert(hb_id_expired(HB_ID_BACKGROUND, 600000, 600001));
    assert(!hb_id_auto_ready(true, 0)); // RAM-only reboot, no restored templates.
    assert(!hb_id_auto_ready(false, 8));
    assert(hb_id_auto_ready(true, 1));
    assert(hb_id_detection_failure(true, 2) == HB_ID_AMBIGUOUS);
    assert(hb_id_detection_failure(true, 0) == HB_ID_UNAVAILABLE);
    assert(hb_id_detection_failure(false, 2) == HB_ID_UNAVAILABLE);
    assert(hb_id_detection_failure(true, 1) == HB_ID_UNAVAILABLE);
    // The actual worker wait policy saves 1800 ms between three successful
    // background matches without changing the core's three-frame confirmation.
    unsigned pending_wait = 100 * (hb_id_wait_slices(HB_ID_AUTO, HB_ID_KNOWN, true, false, 1, 3) +
                                   hb_id_wait_slices(HB_ID_AUTO, HB_ID_KNOWN, true, false, 2, 3));
    assert(pending_wait == 200);
    assert(hb_id_wait_slices(HB_ID_BACKGROUND, HB_ID_KNOWN, true, false, 2, 3) == 1);
    assert(hb_id_wait_slices(HB_ID_AUTO, HB_ID_KNOWN, true, true, 3, 3) == 10);
    assert(hb_id_wait_slices(HB_ID_AUTO, HB_ID_UNKNOWN, true, false, 0, 3) == 10);
    assert(hb_id_wait_slices(HB_ID_AUTO, HB_ID_AMBIGUOUS, true, false, 2, 3) == 10);
    assert(hb_id_wait_slices(HB_ID_AUTO, HB_ID_UNAVAILABLE, true, false, 2, 3) == 10);
    assert(hb_id_wait_slices(HB_ID_AUTO, HB_ID_KNOWN, false, false, 1, 3) == 10);
    assert(hb_id_wait_slices(HB_ID_AUTO, HB_ID_KNOWN, true, false, 0, 3) == 10);
    assert(hb_id_wait_slices(HB_ID_AUTO, HB_ID_KNOWN, true, false, 3, 3) == 10);
    assert(hb_id_wait_slices(HB_ID_ENROLL, HB_ID_KNOWN, true, false, 2, 3) == 10);
    assert(hb_id_wait_slices(HB_ID_TEST, HB_ID_KNOWN, true, false, 2, 3) == 10);
    assert(hb_id_wait_slices(HB_ID_STOP, HB_ID_KNOWN, true, false, 2, 3) == 1);
    // Actual worker policy: a profile pause reuses one bounded model set even
    // when the allocator can no longer satisfy a second cold-start admission.
    unsigned loads = 1, releases = 0, inferences = 0;
    bool loaded = true;
    for (unsigned i = 0; i < 100; ++i)
    {
        hb_id_model_action action = hb_id_model_next(HB_ID_STOP, false, true, true, loaded);
        assert(action == HB_ID_MODELS_PARK);
        // PARK performs neither inference nor destruction/allocation.
        assert(loaded && loads == 1 && releases == 0);
        action = hb_id_model_next(HB_ID_AUTO, true, true, false, loaded);
        assert(action == HB_ID_MODELS_RUN);
        ++inferences;
    }
    assert(inferences == 100 && loads == 1 && releases == 0);
    // Explicit stop/cancel still releases; an absent cache never starts a model
    // for a disabled member. Foreground enrollment remains admitted separately.
    assert(hb_id_model_next(HB_ID_STOP, false, false, true, loaded) == HB_ID_MODELS_RELEASE);
    assert(hb_id_model_next(HB_ID_STOP, false, true, false, loaded) == HB_ID_MODELS_RELEASE);
    loaded = false;
    ++releases;
    assert(hb_id_model_next(HB_ID_STOP, false, true, true, loaded) == HB_ID_MODELS_RELEASE);
    assert(hb_id_model_next(HB_ID_ENROLL, false, false, false, loaded) == HB_ID_MODELS_RUN);
    assert(hb_id_model_next(HB_ID_TEST, false, false, false, loaded) == HB_ID_MODELS_RELEASE);
    assert(releases == 1);
    puts("identity navigation: pending background, rejected commands and stale worker modes");
    return 0;
}
