#ifndef RT_BIQUAD_H
#define RT_BIQUAD_H

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <exceptions.h>

// Combined biquad coefficients and delay line state
struct rt_biquad_state
{
    float b0, b1, b2, a1, a2;
    float x1, x2, y1, y2;
};

struct rt_band
{
    float hz;
    float end_hz;
    float q;
    float db_gain;
    struct rt_biquad_state *state_right;
    struct rt_biquad_state *state_left;
};

static inline int
update_state(struct rt_biquad_state *state, float b0, float b1, float b2, float a1, float a2)
{
    if (state == NULL)
        return STATE_IS_NULL;

    state->b0 = b0;
    state->b1 = b1;
    state->b2 = b2;
    state->a1 = a1;
    state->a2 = a2;

    return SUCCESS;
}

static inline struct rt_biquad_state *create_state()
{
    struct rt_biquad_state *state = (struct rt_biquad_state *)calloc(1, sizeof(struct rt_biquad_state));

    return state;
}

static inline void destroy_state(struct rt_biquad_state *state)
{
    free(state);
}

static inline struct rt_band *create_band(float start_hz, float end_hz, float db_gain, float sample_rate)
{
    struct rt_biquad_state *state_r = create_state();
    struct rt_biquad_state *state_l = create_state();
    if (state_r == NULL || state_l == NULL)
    {
        printf("Error: Memory allocation failed.\n");
        if (state_r)
            destroy_state(state_r);
        if (state_l)
            destroy_state(state_l);
        return (struct rt_band *)(intptr_t)MEMORY_ALLOCATION_FAILED;
    }
    struct rt_band *band = (struct rt_band *)calloc(1, sizeof(struct rt_band));
    if (band == NULL)
    {
        printf("Error: Memory allocation failed.\n");
        destroy_state(state_r);
        destroy_state(state_l);
        return (struct rt_band *)(intptr_t)MEMORY_ALLOCATION_FAILED;
    }
    band->state_right = state_r;
    band->state_left = state_l;

    band->hz = start_hz;
    band->end_hz = end_hz;
    band->db_gain = db_gain;

    float center_hz = sqrtf(start_hz * end_hz);
    float N = logf(end_hz / start_hz) / 0.69314718f;
    float q = 1.0f / (2.0f * sinhf(0.34657359f * N));

    float A = powf(10.0f, db_gain / 40.0f);
    float omega = 2.0f * 3.1415926535f * center_hz / sample_rate;
    float sn = sinf(omega);
    float cs = cosf(omega);

    float alpha = sn / (2.0f * q);

    float a0_raw = 1.0f + (alpha / A);
    float b0 = 1.0f + (alpha * A);
    float b1 = -2.0f * cs;
    float b2 = 1.0f - (alpha * A);
    float a1 = -2.0f * cs;
    float a2 = 1.0f - (alpha / A);

    float inv_a0 = 1.0f / a0_raw;

    band->q = q;

    update_state(band->state_left, b0 * inv_a0, b1 * inv_a0, b2 * inv_a0, a1 * inv_a0, a2 * inv_a0);
    update_state(band->state_right, b0 * inv_a0, b1 * inv_a0, b2 * inv_a0, a1 * inv_a0, a2 * inv_a0);

    return band;
}
static inline void destroy_band(struct rt_band *band)
{
    printf("destroy band\n");
    if (band == NULL)
        return;

    if (band->state_right != NULL)
        destroy_state(band->state_right);
    if (band->state_left != NULL)
        destroy_state(band->state_left);

    free(band);
}

// Filtering                                               *array   sample
static inline float process_sample(struct rt_biquad_state *state, float input)
{
    // Biquad difference equation direct form 1
    float output = (state->b0 * input) + (state->b1 * state->x1) + (state->b2 * state->x2) - (state->a1 * state->y1) - (state->a2 * state->y2);

    // update state
    state->x2 = state->x1;
    state->x1 = input;
    state->y2 = state->y1;
    state->y1 = output;

    return output;
}

static inline void filter(struct rt_biquad_state *state, float *samples, int length)
{
    for (int i = 0; i < length; i++)
    {
        samples[i] = process_sample(state, samples[i]);
    }
}

static inline void filter_from_hz_list(struct rt_band *bands, float *samples, int length, float sample_rate, int band_count)
{
    for (int i = 0; i < band_count; i++)
    {
        printf("filter band %d\n", i);
        for (int j = 0; j < length; j += 2)
        {
            samples[j] = process_sample(bands[i].state_left, samples[j]);
            samples[j + 1] = process_sample(bands[i].state_right, samples[j + 1]);
        }
    }
}

#endif // RT_BIQUAD_H