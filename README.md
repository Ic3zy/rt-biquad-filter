# rt_biquad

`rt_biquad` is a lightweight C99 biquad filter library designed for real-time digital audio processing applications.

The primary objective of this library is to reproduce the acoustic response and tone-shaping characteristics of classic 3-band (Bass / Mid / Treble) analog potentiometers found in 2000s-era stereo receivers and amplifiers in the digital domain.

Initial versions relied solely on fixed-bandwidth graphic EQ formulas. However, graphic EQ curves do not accurately mirror the frequency interaction of analog amplifier circuits. To address this, RBJ (Robert Bristow-Johnson) **Low Shelf**, **High Shelf**, and **Peak** filter types were integrated. The Low Shelf and High Shelf filters emulate classic analog tone controls, while the Graphic EQ mode retains automatic Q calculation based on start and end frequencies.

## Features

- **Header-Only**: Zero external dependencies; drop `rt_biquad.h` directly into your project.
- **Real-Time Safe**: Heap allocation occurs only during initialization; the real-time audio processing path performs minimal control flow and no trigonometric or mathematical library calls.
- **Direct Form 1**: Implements standard Direct Form I biquad filters.
- **Stereo Support**: Independent delay line state (`rt_biquad_state`) for left and right channels per band.
- **Dynamic Coefficient Updating**: `update_band()` recalculates coefficients only when parameters change, using epsilon-based parameter comparisons to avoid unnecessary coefficient recalculation.
- **Low CPU Overhead**: Real-time processing loops perform multiply-accumulate operations using precomputed coefficients.

## Filter Types

| Filter Type | Identifier | Description |
| :--- | :--- | :--- |
| **Low Shelf** | `RT_FILTER_LOW_SHELF` | 2nd-order RBJ Low-Shelf filter ($S=1.0$) for Bass control. |
| **Peak EQ** | `RT_FILTER_PEAK` | Standard RBJ Peaking EQ with configurable center frequency and Q factor. |
| **High Shelf** | `RT_FILTER_HIGH_SHELF` | 2nd-order RBJ High-Shelf filter ($S=1.0$) for Treble control. |
| **Graphic EQ** | `RT_FILTER_GRAPHIC_EQ` | Band filter automatically deriving center frequency and Q factor from start and end frequencies. |

## Usage Example

```c
#include <stdio.h>
#include "rt_biquad.h"

#define SAMPLE_RATE 48000.0f
#define BAND_COUNT 3
#define BUFFER_SIZE 512

int main(void) {
    // 1. Create filter bands (Bass, Mid, Treble)
    struct rt_band *bass = create_band(RT_FILTER_LOW_SHELF, 100.0f, 0.0f, 0.0f, 3.0f, SAMPLE_RATE);
    struct rt_band *mid  = create_band(RT_FILTER_PEAK, 1000.0f, 0.0f, 0.75f, -2.0f, SAMPLE_RATE);
    struct rt_band *treb = create_band(RT_FILTER_HIGH_SHELF, 10000.0f, 0.0f, 0.0f, 4.0f, SAMPLE_RATE);

    struct rt_band eq_stack[BAND_COUNT] = { *bass, *mid, *treb };

    // 2. Dynamically update bass gain
    update_band(bass, RT_FILTER_LOW_SHELF, 100.0f, 0.0f, 0.0f, 6.0f, SAMPLE_RATE);
    eq_stack[0] = *bass;

    // 3. Process stereo audio buffer (Interleaved L/R)
    float audio_buffer[BUFFER_SIZE];

    // Real-Time Audio Loop
    filter_from_hz_list(eq_stack, audio_buffer, BUFFER_SIZE, BAND_COUNT);

    // 4. Cleanup
    destroy_band(bass);
    destroy_band(mid);
    destroy_band(treb);

    return 0;
}
```
