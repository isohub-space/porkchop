#pragma once
#include <vector>

namespace pk {

// One porkchop grid cell. Type I is the short-way transfer (angle < 180 deg, prograde)
// -- the primary, validated result (its minimum C3 matches the known Earth->Mars value).
// Type II is KFL's long-way branch (angle > 180 deg; KFL's header treats this as
// retrograde), provided for completeness and not independently validated. Energies in
// km^2/s^2, speeds in km/s.
struct Cell {
    bool   valid_I  = false, valid_II = false;
    double c3_I = 0, vinf_arr_I = 0;    // Type I: launch C3, arrival v-infinity
    double c3_II = 0, vinf_arr_II = 0;  // Type II
    double tof_days = 0;
    bool has() const { return valid_I || valid_II; }
};

// A rectangular grid of transfers over departure x arrival dates (TT Julian Dates).
struct Grid {
    int dep_planet = 0, arr_planet = 0;
    double dep_jd0 = 0, dep_step = 0;   // departure JD of column 0, step (days)
    double arr_jd0 = 0, arr_step = 0;   // arrival JD of row 0, step (days)
    int nd = 0, na = 0;                 // departures (columns), arrivals (rows)
    std::vector<Cell> cells;            // size nd*na, row-major index i*na + j

    const Cell& at(int i, int j) const { return cells[(size_t)i * na + j]; }
    Cell&       at(int i, int j)       { return cells[(size_t)i * na + j]; }
    double dep_jd(int i) const { return dep_jd0 + i * dep_step; }
    double arr_jd(int j) const { return arr_jd0 + j * arr_step; }
};

// Compute a porkchop grid: departure planet leaving between [dep_jd0, dep_jd1]
// (nd samples), arriving at arr_planet between [arr_jd0, arr_jd1] (na samples).
// Uses KFL's verified Lambert solver with mu_sun (DE440).
Grid compute_porkchop(int dep_planet, int arr_planet,
                      double dep_jd0, double dep_jd1, int nd,
                      double arr_jd0, double arr_jd1, int na);

} // namespace pk
