#include <stdio.h>
#include <stdlib.h>
#include "rt_biquad.h"

#define BUFFER_SIZE 1024

int main()
{
    // Open raw audio input 32 bit float PCM
    FILE *fin = fopen("test.raw", "rb");
    if (!fin)
    {
        printf("Error: Could not open test.raw. Make sure it exists.\n");
        return 1;
    }

    // Changing output file name for save processed data here
    FILE *fout = fopen("test_filtering.raw", "wb");
    if (!fout)
    {
        printf("Error: Could not create test_filtering.raw.\n");
        fclose(fin);
        return 1;
    }

    // Initialize biquad state
    struct rt_biquad_state *state = create_state();
    if (!state)
    {
        printf("Failed to allocate filter state.\n");
        fclose(fin);
        fclose(fout);
        return 1;
    }

    // Quick dummy low-pass coefficients
    // Using simple hardcoded values just to see if the math alters the array
    float b0 = 0.067455f, b1 = 0.134910f, b2 = 0.067455f;
    float a1 = -1.142980f, a2 = 0.412800f;

    update_state(state, b0, b1, b2, a1, a2);

    float buffer[BUFFER_SIZE];
    size_t samples_read;
    size_t total_samples = 0;

    printf("Processing audio stream...\n");

    // Stream through file block by block
    while ((samples_read = fread(buffer, sizeof(float), BUFFER_SIZE, fin)) > 0)
    {

        // Process block inline using header loop
        filter(state, buffer, (int)samples_read);

        // Write processed float block to output file
        fwrite(buffer, sizeof(float), samples_read, fout);
        total_samples += samples_read;
    }

    printf("Done! Processed %lu samples successfully.\n", total_samples);

    // Clean resources up
    destroy_state(state);
    fclose(fin);
    fclose(fout);

    return 0;
}