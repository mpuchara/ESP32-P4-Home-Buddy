# Experimental recognition thresholds

Initial configurable menuconfig values, **not validated production thresholds**:

| Setting | Initial value | Meaning |
|---|---:|---|
| HB_IDENTITY_THRESHOLD_MILLI | 700 | top-three person mean cosine >=0.70 |
| HB_IDENTITY_AGREEMENT_MILLI | 600 | at least 3 templates with cosine >=0.60 |
| HB_IDENTITY_MARGIN_MILLI | 120 | best mean minus different-person runner-up >=0.12 |
| temporal confirmations | 3 | same accepted person on distinct chronological frames |

At least 8 committed templates are required. A fixed top-three mean prevents
linear template-count summation bias; more templates can still change order
statistics, so balanced enrollment counts and real validation are required.
Runner-up is always another person. Below threshold means UNKNOWN, insufficient
margin means AMBIGUOUS, invalid features/model/policy means UNAVAILABLE.
Raw cosine/aggregate score is never a percentage probability.

Duplicates/out-of-order/future/cancelled/stale (>5 seconds) results cannot confirm.
A >6 second frame gap breaks the streak. Unknown/ambiguous/inference failures reset
identity rather than indefinitely retaining the previous known person. The existing
face-presence tracker remains independent. Background greetings additionally need
fresh confirmed identity and a 5-minute cooldown per stable ID.
[Calibration protocol](ACCURACY_CALIBRATION.md).
