#pragma once

// Physical constants for Porkchop. Every value is cited; the design audit
// (docs/AUDIT_porkchop_pre-flight_2026-07-20.md) verified them against primary
// sources. SI throughout: metres, seconds, m^3/s^2.
namespace pk {

// Heliocentric gravitational parameter (GM of the Sun), JPL DE440/441 fitted value.
// NAIF gm_de440.tpc: BODY10_GM = 1.3271244004127942e11 km^3/s^2; x1e9 -> m^3/s^2.
// (Park, Folkner, Williams & Boggs 2021, AJ 161:105.)
// NOTE: this is deliberately NOT KFL's consts.h K26A_GM_SUN (1.32712442099e20),
// which is wrong by ~1.6e-8 (a TDB/TCB-era value mislabeled "IAU nominal").
constexpr double MU_SUN = 1.3271244004127942e20;   // m^3/s^2

// The astronomical unit, IAU 2012 Resolution B2 (a defining constant, exact).
constexpr double AU = 149597870700.0;              // m

// Obliquity of the ecliptic at J2000 (JPL/Standish). Only needed to rotate
// ecliptic <-> equatorial; Porkchop works entirely in the ecliptic frame.
constexpr double OBLIQUITY_DEG = 23.43928;

constexpr double DEG2RAD = 0.017453292519943295;   // pi / 180
constexpr double SEC_PER_DAY = 86400.0;
constexpr double J2000_JD = 2451545.0;             // 2000-01-01 12:00 TT

} // namespace pk
