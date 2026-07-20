#pragma once

namespace pk {

// Gregorian calendar date (UTC) to Julian Date. Fliegel & Van Flandern (1968) closed
// form, century-correct across the whole Gregorian calendar (so it agrees with the
// jd_to_ymd inverse used for date labels). Returns the JD in the input scale (UTC).
double julian_date(int year, int month, int day, int hour, int minute, double second);

// TT - UTC, in seconds: 32.184 s (TT-TAI, fixed) + 37 s (TAI-UTC leap seconds, current
// through 2026 per IERS Bulletin C). Ephemerides are indexed by TT, but Porkchop does
// NOT apply this offset: ~69 s is far below the arc-second ephemeris error and it
// cancels in the time-of-flight (both epochs share the scale). Kept only to document
// the magnitude.
constexpr double TT_MINUS_UTC = 69.184;

// Convert a JD to an approximate decimal calendar year (for labels only).
double jd_to_decimal_year(double jd);

} // namespace pk
