#pragma once

// A cool-to-warm ramp for the porkchop: low launch energy (good windows) reads blue,
// high energy reads red. t in [0,1]; writes 8-bit RGB.
inline void colormap_energy(float t, unsigned char& r, unsigned char& g, unsigned char& b) {
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    static const float stop[5][3] = {
        { 0.12f, 0.20f, 0.60f },   // deep blue  (low = good)
        { 0.10f, 0.65f, 0.85f },   // cyan
        { 0.25f, 0.75f, 0.30f },   // green
        { 0.95f, 0.85f, 0.20f },   // yellow
        { 0.82f, 0.16f, 0.12f },   // red        (high)
    };
    float x = t * 4.0f;
    int k = (int)x;
    if (k > 3) k = 3;
    float f = x - k;
    float rr = stop[k][0] + (stop[k + 1][0] - stop[k][0]) * f;
    float gg = stop[k][1] + (stop[k + 1][1] - stop[k][1]) * f;
    float bb = stop[k][2] + (stop[k + 1][2] - stop[k][2]) * f;
    r = (unsigned char)(rr * 255.0f);
    g = (unsigned char)(gg * 255.0f);
    b = (unsigned char)(bb * 255.0f);
}
