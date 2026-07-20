#pragma once
#include "core/ephemeris.h"
#include "core/transfer.h"

// The whole application state: what to compute, the computed grid, the current
// selection, and the heatmap texture. draw_ui() renders one frame and recomputes
// the grid whenever `dirty` is set (on any control change).
struct AppState {
    int dep_planet = pk::EARTH;
    int arr_planet = pk::MARS;
    int dep_y0 = 2026, dep_y1 = 2031;   // departure calendar-year range
    int arr_y0 = 2026, arr_y1 = 2032;   // arrival calendar-year range
    int nd = 160, na = 200;             // grid resolution (departures, arrivals)
    int metric = 0;                     // 0 = launch C3, 1 = arrival v-infinity
    int type = 0;                       // 0 = Type I (short-way), 1 = Type II (long-way)

    pk::Grid grid;
    bool dirty = true;
    int sel_i = -1, sel_j = -1;

    float cmin = 0.0f, cmax = 50.0f;    // colour scale (auto on recompute)
    unsigned int tex = 0;               // heatmap GL texture id
};

void draw_ui(AppState& s);
