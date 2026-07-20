#include "core/julian.h"
#include "core/constants.h"

namespace pk {

double julian_date(int year, int month, int day, int hour, int minute, double second) {
    // Gregorian calendar date -> Julian Date, Fliegel & Van Flandern (1968) closed
    // form. Unlike the simpler 1900-2100 forms it carries the century leap-year
    // correction (/100, /400), so it is exact across the whole Gregorian calendar and
    // agrees with jd_to_ymd's inverse over Porkchop's 1800-2050 range.
    long a = (14 - month) / 12;
    long y = (long)year + 4800 - a;
    long m = month + 12 * a - 3;
    long jdn = day + (153 * m + 2) / 5 + 365 * y + y / 4 - y / 100 + y / 400 - 32045;
    // jdn is the Julian Day Number (referenced to noon); shift to the time of day.
    return (double)jdn - 0.5 + (hour + minute / 60.0 + second / 3600.0) / 24.0;
}

double jd_to_decimal_year(double jd) {
    // Good enough for axis labels (Julian year of 365.25 days from J2000).
    return 2000.0 + (jd - J2000_JD) / 365.25;
}

} // namespace pk
