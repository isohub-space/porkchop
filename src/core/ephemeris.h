#pragma once

namespace pk {

// The eight planets Porkchop knows, in order. (EARTH is the Earth-Moon barycentre,
// as Standish's Table 1 gives it — good to the table's arc-second accuracy.)
enum Planet { MERCURY = 0, VENUS, EARTH, MARS, JUPITER, SATURN, URANUS, NEPTUNE,
              N_PLANETS };

// A heliocentric state in the mean ecliptic and equinox of J2000, in SI units:
// position in metres, velocity in m/s.
struct State {
    double r[3];
    double v[3];
};

// Position + velocity of a planet at a given TT Julian Date, from JPL's Standish
// "approximate positions" Keplerian elements (Table 1, valid 1800-2050). Accuracy
// for Earth: ~20" longitude, ~6000 km distance -- adequate for porkchop scoping.
// Source: https://ssd.jpl.nasa.gov/planets/approx_pos.html
State planet_state(int planet, double jd_tt);

const char* planet_name(int planet);

} // namespace pk
