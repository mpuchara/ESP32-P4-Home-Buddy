// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "hb_identity.h"

// UI-thread ownership: track accepted intent rather than a delayed worker snapshot.
// This only controls navigation cancellation; it never starts or extends a lease.
static inline bool hb_identity_saver_intent(bool current, hb_id_command_type command, bool accepted)
{
    if (!accepted)
        return current;
    switch (command)
    {
    case HB_ID_CMD_BACKGROUND:
        return true;
    case HB_ID_CMD_TEST:
    case HB_ID_CMD_BEGIN:
    case HB_ID_CMD_STOP:
    case HB_ID_CMD_CANCEL:
    case HB_ID_CMD_DELETE:
    case HB_ID_CMD_SAVE:
        return false;
    default:
        return current;
    }
}
