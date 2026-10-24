// SPDX-License-Identifier: Apache-2.0
#include "identity_navigation.h"
#include <assert.h>
#include <stdio.h>

int main(void)
{
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
    puts("identity navigation: pending background, rejected commands and stale worker modes");
    return 0;
}
