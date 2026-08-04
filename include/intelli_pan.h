#ifndef RT_INTELLIPAN_H
#define RT_INTELLIPAN_H

#include "rt_biquad.h"

#define IP_LOW_FREQ_MIN 120.0f
#define IP_LOW_FREQ_MAX 320.0f

#define IP_MID_FREQ_MIN 1200.0f
#define IP_MID_FREQ_MAX 4500.0f

#define IP_HIGH_FREQ_MIN 5000.0f
#define IP_HIGH_FREQ_MAX 10000.0f

#define IP_MAX_GAIN_DB 8.0f
#define IP_LOW_SHELF_GAIN 5.0f
#define IP_HIGH_SHELF_GAIN 6.0f

typedef struct {
  struct rt_band *low_shelf;
  struct rt_band *mid_peak;
  struct rt_band *high_shelf;

  float x;
  float y;
  float sample_rate;
  int is_enabled;
} IntelliPan;

static inline float ip_clamp(float val, float min, float max) {
  return (val < min) ? min : ((val > max) ? max : val);
}

static inline float ip_log_interp(float norm_x, float f_min, float f_max) {
  return f_min * powf(f_max / f_min, norm_x);
}

static inline float ip_smoothstep_gain(float y, float max_db) {
  float abs_y = fabsf(y);
  // Smoothstep: 3y^2 - 2y^3
  float smooth = abs_y * abs_y * (3.0f - 2.0f * abs_y);
  return (y < 0.0f ? -smooth : smooth) * max_db;
}

static inline IntelliPan *intellipan_create(float sample_rate) {
  IntelliPan *ip = (IntelliPan *)calloc(1, sizeof(IntelliPan));
  if (!ip)
    return NULL;

  ip->sample_rate = sample_rate;
  ip->x = 0.0f;
  ip->y = 0.0f;
  ip->is_enabled = 1;

  ip->low_shelf =
      create_band(RT_FILTER_LOW_SHELF, 200.0f, 0.0f, 0.707f, 0.0f, sample_rate);

  ip->mid_peak =
      create_band(RT_FILTER_PEAK, 2200.0f, 0.0f, 0.8f, 0.0f, sample_rate);

  ip->high_shelf = create_band(RT_FILTER_HIGH_SHELF, 7000.0f, 0.0f, 0.707f,
                               0.0f, sample_rate);

  // Allocation Fail Check
  if ((uintptr_t)ip->low_shelf <= MEMORY_ALLOCATION_FAILED ||
      (uintptr_t)ip->mid_peak <= MEMORY_ALLOCATION_FAILED ||
      (uintptr_t)ip->high_shelf <= MEMORY_ALLOCATION_FAILED) {
    if (ip->low_shelf)
      destroy_band(ip->low_shelf);
    if (ip->mid_peak)
      destroy_band(ip->mid_peak);
    if (ip->high_shelf)
      destroy_band(ip->high_shelf);
    free(ip);
    return NULL;
  }

  return ip;
}

static inline void intellipan_set_position(IntelliPan *ip, float x, float y) {
  if (!ip)
    return;

  x = ip_clamp(x, -1.0f, 1.0f);
  y = ip_clamp(y, -1.0f, 1.0f);

  ip->x = x;
  ip->y = y;

  float norm_x = (x + 1.0f) * 0.5f; // [0.0, 1.0]

  float low_hz = ip_log_interp(1.0f - norm_x, IP_LOW_FREQ_MIN, IP_LOW_FREQ_MAX);
  float low_gain = (-x * 0.6f - y * 0.4f) * IP_LOW_SHELF_GAIN;
  low_gain = ip_clamp(low_gain, -IP_LOW_SHELF_GAIN, IP_LOW_SHELF_GAIN);

  update_band(ip->low_shelf, RT_FILTER_LOW_SHELF, low_hz, 0.0f, 0.707f,
              low_gain, ip->sample_rate);

  float mid_hz = ip_log_interp(norm_x, IP_MID_FREQ_MIN, IP_MID_FREQ_MAX);
  float mid_gain = ip_smoothstep_gain(y, IP_MAX_GAIN_DB);

  float mid_q = 0.65f + (fabsf(y) * 0.55f);

  update_band(ip->mid_peak, RT_FILTER_PEAK, mid_hz, 0.0f, mid_q, mid_gain,
              ip->sample_rate);

  float high_hz = ip_log_interp(norm_x, IP_HIGH_FREQ_MIN, IP_HIGH_FREQ_MAX);
  float high_gain = (x * 0.7f + y * 0.3f) * IP_HIGH_SHELF_GAIN;
  high_gain = ip_clamp(high_gain, -IP_HIGH_SHELF_GAIN, IP_HIGH_SHELF_GAIN);

  update_band(ip->high_shelf, RT_FILTER_HIGH_SHELF, high_hz, 0.0f, 0.707f,
              high_gain, ip->sample_rate);
}

static inline void intellipan_process(IntelliPan *restrict ip,
                                      float *restrict in_l,
                                      float *restrict in_r,
                                      uint32_t n_samples) {
  struct rt_biquad_state *ls_l = ip->low_shelf->state_left;
  struct rt_biquad_state *ls_r = ip->low_shelf->state_right;

  struct rt_biquad_state *mp_l = ip->mid_peak->state_left;
  struct rt_biquad_state *mp_r = ip->mid_peak->state_right;

  struct rt_biquad_state *hs_l = ip->high_shelf->state_left;
  struct rt_biquad_state *hs_r = ip->high_shelf->state_right;

  for (uint32_t i = 0; i < n_samples; i++) {
    float l = in_l[i];
    float r = in_r[i];

    // 1. Low-Shelf
    l = process_sample(ls_l, l);
    r = process_sample(ls_r, r);

    // 2. Mid-Peak (Presence)
    l = process_sample(mp_l, l);
    r = process_sample(mp_r, r);

    // 3. High-Shelf (Brightness/Air)
    l = process_sample(hs_l, l);
    r = process_sample(hs_r, r);

    in_l[i] = l;
    in_r[i] = r;
  }
}

static inline void intellipan_destroy(IntelliPan *ip) {
  if (!ip)
    return;
  if (ip->low_shelf)
    destroy_band(ip->low_shelf);
  if (ip->mid_peak)
    destroy_band(ip->mid_peak);
  if (ip->high_shelf)
    destroy_band(ip->high_shelf);
  free(ip);
}

#endif // RT_INTELLIPAN_H