// Headless verification of the compute core, run before any GUI exists. Checks the
// Standish ephemeris against known heliocentric distances and the Earth->Mars
// porkchop against the known launch-energy range for a real transfer window.
#include <cstdio>
#include <cmath>
#include "core/constants.h"
#include "core/julian.h"
#include "core/ephemeris.h"
#include "core/transfer.h"

using namespace pk;

static double dist_au(const State& s) {
    return std::sqrt(s.r[0]*s.r[0] + s.r[1]*s.r[1] + s.r[2]*s.r[2]) / AU;
}

int main() {
    int fails = 0;

    // --- 1. Ephemeris sanity: heliocentric distances at J2000 ---
    printf("== ephemeris (heliocentric distance, AU) ==\n");
    struct { int p; double lo, hi; } rng[] = {
        { MERCURY, 0.307, 0.467 }, { VENUS, 0.718, 0.728 }, { EARTH, 0.983, 1.017 },
        { MARS, 1.381, 1.666 }, { JUPITER, 4.95, 5.46 }, { SATURN, 9.02, 10.05 },
    };
    for (auto& r : rng) {
        State s = planet_state(r.p, J2000_JD);
        double d = dist_au(s);
        bool ok = d > r.lo && d < r.hi;
        printf("  %-8s %.4f AU  [%.3f, %.3f]  %s\n",
               planet_name(r.p), d, r.lo, r.hi, ok ? "ok" : "OUT OF RANGE");
        if (!ok) ++fails;
    }

    // Earth's speed at J2000 should be ~29.8 km/s.
    State e = planet_state(EARTH, J2000_JD);
    double vE = std::sqrt(e.v[0]*e.v[0] + e.v[1]*e.v[1] + e.v[2]*e.v[2]) / 1000.0;
    bool vok = vE > 29.0 && vE < 30.6;
    printf("  Earth speed %.3f km/s  [29.0, 30.6]  %s\n", vE, vok ? "ok" : "BAD");
    if (!vok) ++fails;

    // --- 2. Earth -> Mars porkchop over a broad window; min C3 must be sane ---
    printf("\n== Earth -> Mars porkchop (2026-2031 departures) ==\n");
    double dep0 = julian_date(2026, 1, 1, 0, 0, 0);
    double dep1 = julian_date(2031, 1, 1, 0, 0, 0);
    double arr0 = julian_date(2026, 6, 1, 0, 0, 0);
    double arr1 = julian_date(2032, 6, 1, 0, 0, 0);
    Grid g = compute_porkchop(EARTH, MARS, dep0, dep1, 240, arr0, arr1, 300);

    double best = 1e30; int bi = -1, bj = -1;
    for (int i = 0; i < g.nd; ++i)
        for (int j = 0; j < g.na; ++j) {
            const Cell& c = g.at(i, j);
            if (c.valid_I && c.c3_I < best) { best = c.c3_I; bi = i; bj = j; }
        }

    if (bi < 0) { printf("  no valid transfer found!\n"); ++fails; }
    else {
        const Cell& c = g.at(bi, bj);
        printf("  min C3 (Type I) = %.2f km^2/s^2\n", best);
        printf("  depart ~%.2f, arrive ~%.2f, TOF %.0f days, arrival vinf %.2f km/s\n",
               jd_to_decimal_year(g.dep_jd(bi)), jd_to_decimal_year(g.arr_jd(bj)),
               c.tof_days, c.vinf_arr_I);
        // A real Earth->Mars window sits near the Hohmann C3 ~ 8.9 km^2/s^2; accept
        // a generous band that still rejects a broken pipeline (wrong units/frame
        // would give thousands or fractions).
        bool ok = best > 7.0 && best < 25.0 && c.tof_days > 120 && c.tof_days < 400;
        printf("  plausible window? %s\n", ok ? "yes" : "NO");
        if (!ok) ++fails;
    }

    printf("\n%s (%d check(s) failed)\n", fails == 0 ? "ALL PASS" : "FAILURES", fails);
    return fails == 0 ? 0 : 1;
}
