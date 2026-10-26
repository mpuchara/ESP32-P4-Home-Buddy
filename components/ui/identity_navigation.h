// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "hb_identity.h"

// An in-progress durable save can exceed the command-admission timeout.
// The caller handles an actual creation receipt before evaluating this timeout.
static inline bool hb_identity_create_timed_out(uint32_t elapsed_ms, bool storage_saving)
{
    return elapsed_ms > 5000 && !storage_saving;
}

// UI-thread ownership: track accepted intent rather than a delayed worker snapshot.
// This only controls navigation cancellation; it never starts or extends a lease.
static inline bool hb_identity_saver_intent(bool current, hb_id_command_type command, bool accepted)
{
    if (!accepted)
        return current;
    switch (command)
    {
    case HB_ID_CMD_AUTO:
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
