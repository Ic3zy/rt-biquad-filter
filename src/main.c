#include <stdio.h>
#include <stdlib.h>
#include <../include/exceptions.h>

// Combined biquad coefficients and delay line state
struct rt_biquad_state
{
    float b0, b1, b2, a1, a2;
    float x1, x2, y1, y2;
};

int history_update(struct rt_biquad_state *state, float b0, float b1, float b2, float a1, float a2)
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

struct rt_biquad_state *create_state()
{
    struct rt_biquad_state *state = calloc(1, sizeof(struct rt_biquad_state));

    return state;
}

void destroy_state(struct rt_biquad_state *state)
{
    free(state);
}

// Filtering                                       *array   array len
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

inline void filter(struct rt_biquad_state *state, float *samples, int length)
{
    for (int i = 0; i < length; i++)
    {
        samples[i] = process_sample(state, samples[i]);
    }
}