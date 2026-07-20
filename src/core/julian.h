#pragma once

namespace pk {

// Calendar (UTC) to Julian Date. Vallado, "Fundamentals of Astrodynamics and
// Applications", Algorithm 14 (jday); valid 1900-2100. Returns the JD in the same
// time scale as the input (UTC here).
double julian_date(int year, int month, int day, int hour, int minute, double second);

// TT - UTC, in seconds: 32.184 s (TT-TAI, fixed) + 37 s (TAI-UTC leap seconds,
// current through 2026 per IERS Bulletin C). Ephemerides are indexed by TT; add
// this when forming the ephemeris JD from a civil (UTC) date. Negligible at
// day-resolution, applied for consistency.
constexpr double TT_MINUS_UTC = 69.184;

// Convert a JD to an approximate decimal calendar year (for labels only).
double jd_to_decimal_year(double jd);

} // namespace pk
