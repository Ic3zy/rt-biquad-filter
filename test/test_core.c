#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "rt_biquad.h"

#define BUFFER_SIZE 1024
#define BAND_COUNT 4

int main()
{
    float sample_rate = 48000.0f;
    struct rt_band eq_bands[BAND_COUNT];

    struct rt_band *b1 = create_band(RT_FILTER_LOW_SHELF, 80.0f, 0.0f, 0.0f, 6.0f, sample_rate);
    struct rt_band *b2 = create_band(RT_FILTER_PEAK, 1000.0f, 0.0f, 0.7f, 4.0f, sample_rate);
    struct rt_band *b3 = create_band(RT_FILTER_HIGH_SHELF, 10000.0f, 0.0f, 0.0f, 6.0f, sample_rate);
    struct rt_band *b4 = create_band(RT_FILTER_GRAPHIC_EQ, 20.0f, 200.0f, 0.0f, 6.0f, sample_rate);

    if ((intptr_t)b1 == MEMORY_ALLOCATION_FAILED ||
        (intptr_t)b2 == MEMORY_ALLOCATION_FAILED ||
        (intptr_t)b3 == MEMORY_ALLOCATION_FAILED ||
        (intptr_t)b4 == MEMORY_ALLOCATION_FAILED)
    {
        printf("Error: Memory allocation failed.\n");
        return 1;
    }

    eq_bands[0] = *b1;
    eq_bands[1] = *b2;
    eq_bands[2] = *b3;
    eq_bands[3] = *b4;

    // Test cached update_band call with identical parameters
    int res = update_band(b1, RT_FILTER_LOW_SHELF, 80.0f, 0.0f, 0.0f, 6.0f, sample_rate);
    if (res != SUCCESS)
    {
        printf("Error: update_band cached call failed.\n");
        return 1;
    }

    printf("Processing test signal using updated API...\n");

    float buffer[BUFFER_SIZE];
    for (int i = 0; i < BUFFER_SIZE; i++)
    {
        buffer[i] = (float)(i % 100) / 100.0f;
    }

    filter_from_hz_list(eq_bands, buffer, BUFFER_SIZE, BAND_COUNT);

    printf("Done! Filtered %d test samples successfully.\n", BUFFER_SIZE);

    for (int i = 0; i < BAND_COUNT; i++)
    {
        destroy_state(eq_bands[i].state_right);
        destroy_state(eq_bands[i].state_left);
    }

    free(b1);
    free(b2);
    free(b3);
    free(b4);

    return 0;
}