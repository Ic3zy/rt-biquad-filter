#ifndef RT_BIQUAD_H
#define RT_BIQUAD_H

#include <math.h>
#include <stdlib.h>
#include <stdint.h>
#include <exceptions.h>

#define RT_PI 3.14159265358979323846f
#define RT_EPSILON 1e-6f

enum rt_filter_type
{
    RT_FILTER_GRAPHIC_EQ,
    RT_FILTER_PEAK,
    RT_FILTER_LOW_SHELF,
    RT_FILTER_HIGH_SHELF
};

// Combined biquad coefficients and delay line state
struct rt_biquad_state
{
    float b0, b1, b2, a1, a2;
    float x1, x2, y1, y2;
};

struct rt_band
{
    enum rt_filter_type type;
    float hz;
    float end_hz;
    float q;
    float db_gain;
    float sample_rate;
    struct rt_biquad_state *state_right;
    struct rt_biquad_state *state_left;
};

static inline int rt_float_equal(float a, float b)
{
    return fabsf(a - b) < RT_EPSILON;
}

static inline int update_state(struct rt_biquad_state *state, float b0, float b1, float b2, float a1, float a2)
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

static inline struct rt_biquad_state *create_state(void)
{
    return (struct rt_biquad_state *)calloc(1, sizeof(struct rt_biquad_state));
}

static inline void destroy_state(struct rt_biquad_state *state)
{
    free(state);
}

static inline void destroy_band(struct rt_band *band)
{
    if (band == NULL)
        return;

    if (band->state_right != NULL)
        destroy_state(band->state_right);
    if (band->state_left != NULL)
        destroy_state(band->state_left);

    free(band);
}

static inline int update_band(
    struct rt_band *band,
    enum rt_filter_type type,
    float hz,
    float end_hz,
    float q,
    float db_gain,
    float sample_rate)
{
    if (band == NULL || band->state_left == NULL || band->state_right == NULL)
    {
        return STATE_IS_NULL;
    }

    if (band->type == type &&
        rt_float_equal(band->hz, hz) &&
        rt_float_equal(band->end_hz, end_hz) &&
        rt_float_equal(band->db_gain, db_gain) &&
        rt_float_equal(band->sample_rate, sample_rate) &&
        (type != RT_FILTER_PEAK || rt_float_equal(band->q, q)))
    {
        return SUCCESS;
    }

    float center_hz = hz;
    float filter_q = q;

    if (type == RT_FILTER_GRAPHIC_EQ)
    {
        center_hz = sqrtf(hz * end_hz);
        float N = logf(end_hz / hz) / 0.69314718f;
        filter_q = 1.0f / (2.0f * sinhf(0.34657359f * N));
    }

    band->type = type;
    band->hz = hz;
    band->end_hz = end_hz;
    band->q = filter_q;
    band->db_gain = db_gain;
    band->sample_rate = sample_rate;

    float A = powf(10.0f, db_gain / 40.0f);
    float omega = 2.0f * RT_PI * center_hz / sample_rate;
    float sn = sinf(omega);
    float cs = cosf(omega);

    float b0 = 0.0f, b1 = 0.0f, b2 = 0.0f;
    float a0_raw = 1.0f, a1 = 0.0f, a2 = 0.0f;

    switch (type)
    {
    case RT_FILTER_GRAPHIC_EQ:
    case RT_FILTER_PEAK:
    {
        float alpha = sn / (2.0f * filter_q);
        a0_raw = 1.0f + (alpha / A);
        b0     = 1.0f + (alpha * A);
        b1     = -2.0f * cs;
        b2     = 1.0f - (alpha * A);
        a1     = -2.0f * cs;
        a2     = 1.0f - (alpha / A);
        break;
    }
    case RT_FILTER_LOW_SHELF:
    case RT_FILTER_HIGH_SHELF:
    {
        float S = 1.0f;
        float alpha = (sn / 2.0f) * sqrtf((A + 1.0f / A) * (1.0f / S - 1.0f) + 2.0f);
        float beta = 2.0f * sqrtf(A) * alpha;
        float Ap1 = A + 1.0f;
        float Am1 = A - 1.0f;

        if (type == RT_FILTER_LOW_SHELF)
        {
            a0_raw = Ap1 + Am1 * cs + beta;
            b0     = A * (Ap1 - Am1 * cs + beta);
            b1     = 2.0f * A * (Am1 - Ap1 * cs);
            b2     = A * (Ap1 - Am1 * cs - beta);
            a1     = -2.0f * (Am1 + Ap1 * cs);
            a2     = Ap1 + Am1 * cs - beta;
        }
        else
        {
            a0_raw = Ap1 - Am1 * cs + beta;
            b0     = A * (Ap1 + Am1 * cs + beta);
            b1     = -2.0f * A * (Am1 + Ap1 * cs);
            b2     = A * (Ap1 + Am1 * cs - beta);
            a1     = 2.0f * (Am1 - Ap1 * cs);
            a2     = Ap1 - Am1 * cs - beta;
        }
        break;
    }
    }

    float inv_a0 = 1.0f / a0_raw;

    update_state(band->state_left,  b0 * inv_a0, b1 * inv_a0, b2 * inv_a0, a1 * inv_a0, a2 * inv_a0);
    update_state(band->state_right, b0 * inv_a0, b1 * inv_a0, b2 * inv_a0, a1 * inv_a0, a2 * inv_a0);

    return SUCCESS;
}

static inline struct rt_band *create_band(
    enum rt_filter_type type,
    float hz,
    float end_hz,
    float q,
    float db_gain,
    float sample_rate)
{
    struct rt_biquad_state *state_r = create_state();
    struct rt_biquad_state *state_l = create_state();
    if (state_r == NULL || state_l == NULL)
    {
        if (state_r)
            destroy_state(state_r);
        if (state_l)
            destroy_state(state_l);
        return (struct rt_band *)(intptr_t)MEMORY_ALLOCATION_FAILED;
    }

    struct rt_band *band = (struct rt_band *)calloc(1, sizeof(struct rt_band));
    if (band == NULL)
    {
        destroy_state(state_r);
        destroy_state(state_l);
        return (struct rt_band *)(intptr_t)MEMORY_ALLOCATION_FAILED;
    }

    band->state_right = state_r;
    band->state_left = state_l;
    band->sample_rate = -1.0f;

    int result = update_band(band, type, hz, end_hz, q, db_gain, sample_rate);
    if (result != SUCCESS)
    {
        destroy_band(band);
        return (struct rt_band *)(intptr_t)result;
    }

    return band;
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

static inline void filter_from_hz_list(struct rt_band *bands, float *samples, int length, int band_count)
{
    for (int i = 0; i < band_count; i++)
    {
        if (rt_float_equal(bands[i].db_gain, 0.0f))
            continue;

        for (int j = 0; j < length; j += 2)
        {
            samples[j] = process_sample(bands[i].state_left, samples[j]);
            samples[j + 1] = process_sample(bands[i].state_right, samples[j + 1]);
        }
    }
}

#endif // RT_BIQUAD_H