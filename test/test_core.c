#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "rt_biquad.h"

#define BUFFER_SIZE 1024
#define BAND_COUNT 3

int main()
{
    FILE *fin = fopen("test.raw", "rb");
    if (!fin)
    {
        printf("Error: Could not open test.raw.\n");
        return 1;
    }

    FILE *fout = fopen("test_filtering.raw", "wb");
    if (!fout)
    {
        printf("Error: Could not create test_filtering.raw.\n");
        fclose(fin);
        return 1;
    }

    float sample_rate = 48000.0f;
    struct rt_band eq_bands[BAND_COUNT];

    struct rt_band *b1 = create_band(20.0f, 200.0f, 6.0f, sample_rate);
    struct rt_band *b2 = create_band(200.0f, 2000.0f, -3.0f, sample_rate);
    struct rt_band *b3 = create_band(2000.0f, 20000.0f, 4.0f, sample_rate);

    if ((intptr_t)b1 == MEMORY_ALLOCATION_FAILED ||
        (intptr_t)b2 == MEMORY_ALLOCATION_FAILED ||
        (intptr_t)b3 == MEMORY_ALLOCATION_FAILED)
    {
        printf("Error: Memory allocation failed.\n");
        fclose(fin);
        fclose(fout);
        return 1;
    }

    eq_bands[0] = *b1;
    eq_bands[1] = *b2;
    eq_bands[2] = *b3;

    printf("Processing stream using exact 5-parameter signature...\n");

    float buffer[BUFFER_SIZE];
    size_t samples_read;
    size_t total_samples = 0;

    while ((samples_read = fread(buffer, sizeof(float), BUFFER_SIZE, fin)) > 0)
    {
        filter_from_hz_list(eq_bands, buffer, (int)samples_read, sample_rate, BAND_COUNT);

        fwrite(buffer, sizeof(float), samples_read, fout);
        total_samples += samples_read;
    }

    printf("Done! Processed %lu samples.\n", total_samples);

    for (int i = 0; i < BAND_COUNT; i++)
    {
        destroy_state(eq_bands[i].state_right);
        destroy_state(eq_bands[i].state_left);
    }

    fclose(fin);
    fclose(fout);
    return 0;
}