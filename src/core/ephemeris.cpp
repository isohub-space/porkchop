#include "core/ephemeris.h"
#include "core/constants.h"
#include <cmath>

namespace pk {

// JPL Table 1 Keplerian elements (heliocentric, mean ecliptic/equinox J2000),
// valid 1800 AD - 2050 AD. https://ssd.jpl.nasa.gov/planets/approx_pos.html
// Columns: a(au), e, I(deg), L(deg), varpi(deg), Omega(deg), then each rate/century.
// For this date range no b,c,s,f correction terms apply (those are Table 2 only), so
// the mean anomaly is simply M = L - varpi for every planet.
struct Elements {
    double a, e, I, L, varpi, Omega;                  // at J2000
    double da, de, dI, dL, dvarpi, dOmega;            // per Julian century
};

static const Elements TABLE1[N_PLANETS] = {
    // Mercury
    { 0.38709927, 0.20563593, 7.00497902, 252.25032350, 77.45779628, 48.33076593,
      0.00000037, 0.00001906, -0.00594749, 149472.67411175, 0.16047689, -0.12534081 },
    // Venus
    { 0.72333566, 0.00677672, 3.39467605, 181.97909950, 131.60246718, 76.67984255,
      0.00000390, -0.00004107, -0.00078890, 58517.81538729, 0.00268329, -0.27769418 },
    // Earth-Moon barycentre
    { 1.00000261, 0.01671123, -0.00001531, 100.46457166, 102.93768193, 0.0,
      0.00000562, -0.00004392, -0.01294668, 35999.37244981, 0.32327364, 0.0 },
    // Mars
    { 1.52371034, 0.09339410, 1.84969142, -4.55343205, -23.94362959, 49.55953891,
      0.00001847, 0.00007882, -0.00813131, 19140.30268499, 0.44441088, -0.29257343 },
    // Jupiter
    { 5.20288700, 0.04838624, 1.30439695, 34.39644051, 14.72847983, 100.47390909,
      -0.00011607, -0.00013253, -0.00183714, 3034.74612775, 0.21252668, 0.20469106 },
    // Saturn
    { 9.53667594, 0.05386179, 2.48599187, 49.95424423, 92.59887831, 113.66242448,
      -0.00125060, -0.00050991, 0.00193609, 1222.49362201, -0.41897216, -0.28867794 },
    // Uranus
    { 19.18916464, 0.04725744, 0.77263783, 313.23810451, 170.95427630, 74.01692503,
      -0.00196176, -0.00004397, -0.00242939, 428.48202785, 0.40805281, 0.04240589 },
    // Neptune
    { 30.06992276, 0.00859048, 1.77004347, -55.12002969, 44.96476227, 131.78422574,
      0.00026291, 0.00005105, 0.00035372, 218.45945325, -0.32241464, -0.00508664 },
};

static const char* NAMES[N_PLANETS] = {
    "Mercury", "Venus", "Earth", "Mars", "Jupiter", "Saturn", "Uranus", "Neptune"
};

const char* planet_name(int planet) {
    if (planet < 0 || planet >= N_PLANETS) return "?";
    return NAMES[planet];
}

// Wrap an angle (degrees) into [-180, 180].
static double wrap180(double deg) {
    deg = std::fmod(deg, 360.0);
    if (deg > 180.0) deg -= 360.0;
    if (deg < -180.0) deg += 360.0;
    return deg;
}

State planet_state(int planet, double jd_tt) {
    const Elements& el = TABLE1[planet];
    double T = (jd_tt - J2000_JD) / 36525.0;          // centuries past J2000

    double a     = el.a     + el.da * T;              // au
    double e     = el.e     + el.de * T;
    double I     = (el.I    + el.dI * T)     * DEG2RAD;
    double L     =  el.L    + el.dL * T;              // deg
    double varpi =  el.varpi+ el.dvarpi * T;          // deg
    double Omega = (el.Omega+ el.dOmega * T) * DEG2RAD;

    double omega = (varpi - el.Omega - el.dOmega * T);      // arg. of perihelion, deg
    omega *= DEG2RAD;
    double M = wrap180(L - varpi) * DEG2RAD;          // mean anomaly, rad

    // Solve Kepler's equation M = E - e sinE by Newton iteration.
    double E = M + e * std::sin(M);
    for (int it = 0; it < 30; ++it) {
        double dM = M - (E - e * std::sin(E));
        double dE = dM / (1.0 - e * std::cos(E));
        E += dE;
        if (std::fabs(dE) < 1e-12) break;
    }

    // Perifocal position (au) and velocity (au/s).
    double cosE = std::cos(E), sinE = std::sin(E);
    double sqrt1me2 = std::sqrt(1.0 - e * e);
    double xp = a * (cosE - e);
    double yp = a * sqrt1me2 * sinE;

    double a_m = a * AU;
    double n = std::sqrt(MU_SUN / (a_m * a_m * a_m));  // mean motion, rad/s
    double Edot = n / (1.0 - e * cosE);                // rad/s
    double xpd = -a * sinE * Edot;                     // au/s
    double ypd =  a * sqrt1me2 * cosE * Edot;          // au/s

    // Rotate perifocal -> ecliptic J2000 (classical 3-1-3: Omega, I, omega).
    double cO = std::cos(Omega), sO = std::sin(Omega);
    double ci = std::cos(I),     si = std::sin(I);
    double cw = std::cos(omega), sw = std::sin(omega);
    double R11 = cO * cw - sO * sw * ci, R12 = -cO * sw - sO * cw * ci;
    double R21 = sO * cw + cO * sw * ci, R22 = -sO * sw + cO * cw * ci;
    double R31 = sw * si,                R32 =  cw * si;

    State s;
    s.r[0] = (R11 * xp + R12 * yp) * AU;   // au -> m
    s.r[1] = (R21 * xp + R22 * yp) * AU;
    s.r[2] = (R31 * xp + R32 * yp) * AU;
    s.v[0] = (R11 * xpd + R12 * ypd) * AU; // au/s -> m/s
    s.v[1] = (R21 * xpd + R22 * ypd) * AU;
    s.v[2] = (R31 * xpd + R32 * ypd) * AU;
    return s;
}

} // namespace pk
