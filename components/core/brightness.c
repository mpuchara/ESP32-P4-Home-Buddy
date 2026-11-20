// SPDX-License-Identifier: Apache-2.0
#include "hb_brightness.h"
#include <math.h>
#include <string.h>
static float clamp(float value, float lo, float hi)
{
    return fminf(hi, fmaxf(lo, value));
}
hb_brightness_config hb_brightness_defaults(void)
{
    return (hb_brightness_config){.schema = 1,
                                  .mode = HB_BRIGHT_MANUAL,
                                  .manual_pwm = 128,
                                  .fallback_pwm = 128,
                                  .sample_seconds = 5,
                                  .transition_seconds = 30,
                                  .override_seconds = 300};
}
static bool valid_anchor(const hb_light_anchor *a)
{
    return a->valid <= 1 && (!a->valid || (isfinite(a->metric) && fabsf(a->metric) <= 64 &&
                                           a->pwm >= 26 && a->pwm <= 255));
}
bool hb_brightness_valid(const hb_brightness_config *c)
{
    return c && c->schema == 1 && c->mode <= HB_BRIGHT_OVERRIDE && c->manual_pwm >= 26 &&
           c->manual_pwm <= 255 && c->fallback_pwm >= 26 && c->fallback_pwm <= 255 &&
           c->sample_seconds >= 1 && c->sample_seconds <= 60 && c->transition_seconds >= 20 &&
           c->transition_seconds <= 60 && c->override_seconds >= 10 &&
           c->override_seconds <= 3600 && valid_anchor(&c->day) && valid_anchor(&c->night);
}
bool hb_brightness_calibrated(const hb_brightness_config *c)
{
    return hb_brightness_valid(c) && c->day.valid && c->night.valid &&
           fabsf(c->day.metric - c->night.metric) >= 0.5f;
}
uint8_t hb_brightness_target(const hb_brightness_config *c, float metric)
{
    if (!hb_brightness_valid(c))
        return 128;
    if (!hb_brightness_calibrated(c) || !isfinite(metric))
        return (uint8_t)c->fallback_pwm;
    float t = clamp((metric - c->night.metric) / (c->day.metric - c->night.metric), 0, 1);
    return (uint8_t)lroundf(clamp(c->night.pwm + t * ((float)c->day.pwm - c->night.pwm), 26, 255));
}
void hb_brightness_init(hb_brightness_state *s, uint8_t pwm, uint64_t now)
{
    memset(s, 0, sizeof(*s));
    s->output = clamp(pwm, 26, 255);
    s->updated_ms = now;
}
uint8_t hb_brightness_step(hb_brightness_state *s, const hb_brightness_config *c, bool valid,
                           float metric, uint64_t sample, uint64_t now)
{
    if (!hb_brightness_valid(c) || now < s->updated_ms)
        return (uint8_t)lroundf(s->output);
    float dt = fminf((float)(now - s->updated_ms) / 1000, 1);
    s->updated_ms = now;
    bool manual = c->mode == HB_BRIGHT_MANUAL ||
                  (c->mode == HB_BRIGHT_OVERRIDE && now < s->override_until_ms);
    if (manual)
        s->output = (float)c->manual_pwm;
    else if (!hb_brightness_calibrated(c))
        s->output = (float)c->fallback_pwm;
    else if (valid && isfinite(metric) && fabsf(metric) <= 64 && sample <= now &&
             now - sample <= (uint64_t)c->sample_seconds * 3000)
    {
        if (!s->initialized)
        {
            s->filtered = metric;
            s->initialized = true;
            s->sample_ms = sample;
        }
        else if (sample > s->sample_ms)
        {
            float elapsed = (float)(sample - s->sample_ms) / 1000;
            s->sample_ms = sample;
            // Abrupt changes >2 stops need three mutually consistent samples.
            if (fabsf(metric - s->filtered) > 2)
            {
                if (!s->confirmations || fabsf(metric - s->candidate) > 0.5f)
                {
                    s->candidate = metric;
                    s->confirmations = 1;
                }
                else
                    s->confirmations++;
                if (s->confirmations < 3)
                    return (uint8_t)lroundf(s->output);
            }
            s->confirmations = 0;
            float alpha = elapsed / (c->transition_seconds + elapsed);
            s->filtered += alpha * (metric - s->filtered);
        }
        float target = hb_brightness_target(c, s->filtered);
        float delta = target - s->output;
        if (fabsf(delta) >= 3)
        {
            float limit = 229 * dt / c->transition_seconds;
            s->output += clamp(delta, -limit, limit);
        }
    }
    return (uint8_t)lroundf(clamp(s->output, 26, 255));
}
