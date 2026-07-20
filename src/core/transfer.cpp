#include "core/transfer.h"
#include "core/ephemeris.h"
#include "core/constants.h"
#include <cmath>

// KFL: the verified Lambert solver (Vallado Alg 58 / Izzo 2015). SI units.
extern "C" {
#include "k26astro_conics/lambert.h"
#include "k26m3d.h"
}

namespace pk {

static double norm3(const double v[3]) {
    return std::sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
}

// Solve one Lambert arc and fill the (C3, arrival v-inf) for a given direction.
// Returns true if KFL converged. Excess velocities are heliocentric transfer
// velocity minus the planet's own velocity.
static bool solve_arc(const State& sd, const State& sa, double tof, int direction,
                      double& out_c3_km2s2, double& out_vinf_arr_kms) {
    K26V3 r1 = { sd.r[0], sd.r[1], sd.r[2] };
    K26V3 r2 = { sa.r[0], sa.r[1], sa.r[2] };
    K26V3 vt1, vt2;
    if (k26astro_lambert(&vt1, &vt2, r1, r2, MU_SUN, tof, direction) != 0)
        return false;

    double vinf_dep[3] = { vt1.x - sd.v[0], vt1.y - sd.v[1], vt1.z - sd.v[2] };
    double vinf_arr[3] = { vt2.x - sa.v[0], vt2.y - sa.v[1], vt2.z - sa.v[2] };

    double c3 = vinf_dep[0]*vinf_dep[0] + vinf_dep[1]*vinf_dep[1] + vinf_dep[2]*vinf_dep[2];
    out_c3_km2s2     = c3 * 1e-6;             // m^2/s^2 -> km^2/s^2
    out_vinf_arr_kms = norm3(vinf_arr) * 1e-3; // m/s -> km/s
    return true;
}

Grid compute_porkchop(int dep_planet, int arr_planet,
                      double dep_jd0, double dep_jd1, int nd,
                      double arr_jd0, double arr_jd1, int na) {
    Grid g;
    g.dep_planet = dep_planet; g.arr_planet = arr_planet;
    g.nd = nd; g.na = na;
    g.dep_jd0 = dep_jd0; g.dep_step = (nd > 1) ? (dep_jd1 - dep_jd0) / (nd - 1) : 0.0;
    g.arr_jd0 = arr_jd0; g.arr_step = (na > 1) ? (arr_jd1 - arr_jd0) / (na - 1) : 0.0;
    g.cells.assign((size_t)nd * na, Cell{});

    const double MIN_TOF_DAYS = 10.0;   // shorter transfers are unphysical here

    for (int i = 0; i < nd; ++i) {
        double jd_d = g.dep_jd(i);
        State sd = planet_state(dep_planet, jd_d);
        for (int j = 0; j < na; ++j) {
            double jd_a = g.arr_jd(j);
            Cell& c = g.at(i, j);
            double tof_days = jd_a - jd_d;
            if (tof_days < MIN_TOF_DAYS) continue;   // arrival must follow departure
            c.tof_days = tof_days;

            State sa = planet_state(arr_planet, jd_a);
            double tof = tof_days * SEC_PER_DAY;

            c.valid_I  = solve_arc(sd, sa, tof, K26A_LAMBERT_SHORT_WAY,
                                   c.c3_I,  c.vinf_arr_I);
            c.valid_II = solve_arc(sd, sa, tof, K26A_LAMBERT_LONG_WAY,
                                   c.c3_II, c.vinf_arr_II);
        }
    }
    return g;
}

} // namespace pk
