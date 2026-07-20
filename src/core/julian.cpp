#include "core/julian.h"
#include "core/constants.h"
#include <cmath>

namespace pk {

double julian_date(int year, int month, int day, int hour, int minute, double second) {
    // Vallado Algorithm 14.
    double jd = 367.0 * year
              - std::floor(7.0 * (year + std::floor((month + 9.0) / 12.0)) / 4.0)
              + std::floor(275.0 * month / 9.0)
              + day + 1721013.5;
    double jdfrac = (second + 60.0 * minute + 3600.0 * hour) / 86400.0;
    return jd + jdfrac;
}

double jd_to_decimal_year(double jd) {
    // Good enough for axis labels (Julian year of 365.25 days from J2000).
    return 2000.0 + (jd - J2000_JD) / 365.25;
}

} // namespace pk
