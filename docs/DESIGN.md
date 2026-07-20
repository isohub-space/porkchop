# Porkchop — design premises (pre-flight)

*The design intent and its cited premises, written before the code so `fact-check` can
audit the physics, constants, units, and sign conventions first. Every load-bearing
number carries its source.*

**What it is.** An interactive interplanetary transfer designer. For a departure planet,
an arrival planet, and a grid of dates, solve Lambert's problem against analytic planetary
ephemeris and draw the *porkchop plot* — contours of launch energy C3 (and arrival v∞,
total Δv) over departure × arrival date — plus the selected transfer trajectory in a
heliocentric view.

**Stack.** C++17, dear ImGui (docking), OpenGL. **KFL** for the astrodynamics core (its
Lambert solver, already verified against a textbook case). Linux. Self-contained — no
DE441 kernel required.

## The compute pipeline
For each grid cell (t1 = departure date, t2 = arrival date, t2 > t1):
1. **Planet states** — heliocentric position+velocity (r1, v1) of the departure planet at
   t1, and (r2, v2) of the arrival planet at t2. Frame: heliocentric **mean ecliptic and
   equinox of J2000**. (See Ephemeris.)
2. **TOF** = (t2 − t1), in seconds.
3. **Lambert** — (vt1, vt2) = transfer-orbit velocities from
   `k26astro_lambert_multi_rev(r1, r2, μ_sun, TOF, n_rev, direction, branch)`. **SI units
   throughout: metres, m/s, seconds, m³/s².**
4. **Departure** — v∞,dep = vt1 − v1; **C3 = |v∞,dep|²** (the launch characteristic energy).
5. **Arrival** — v∞,arr = vt2 − v2; arrival v∞ = |v∞,arr|.
6. **Contour** the grid by C3 (displayed in km²/s²), arrival v∞ (km/s), or their sum as a
   total-Δv proxy.

## Constants (authoritative — deliberately NOT KFL's; see Trap 1)
- **μ_sun (GM☉) = 1.327 124 400 412 794 2 × 10²⁰ m³/s²** — JPL **DE440/441 fitted** value
  (NAIF `gm_de440.tpc`, `BODY10_GM`; Park, Folkner, Williams & Boggs 2021, *AJ* 161, 105).
  Chosen for consistency with ephemeris practice.
- **AU = 149 597 870 700 m** exactly — IAU 2012 Resolution B2 (defining constant).
- Obliquity of the ecliptic ε = 23.439 28° — only needed if converting ecliptic→equatorial.
- **Units:** SI internally (m, m/s, s, m³/s²); convert for display only (C3 → km²/s²,
  v∞ → km/s).
- A patched-conic porkchop needs **only μ_sun**; planetary GM enters solely in the
  departure/capture burn, which is out of the first version's scope.

## Lambert (via KFL — verified)
- `k26astro_lambert` (Vallado §7.6, Algorithm 58, universal variable) and
  `k26astro_lambert_multi_rev` (Izzo 2015, *CMDA* 121:1). **Verified**: KFL reproduces
  Vallado Example 7-5 to < 0.2 m/s from our own linked test.
- **Branch policy (v1):** default **prograde**, **n_rev = 0** (direct Type I/II); short-way
  vs long-way chosen per cell from the transfer angle (sign of the z-component of r1×r2 in
  the ecliptic frame). Multi-revolution is a later optional toggle. A wrong flag yields a
  valid-but-wrong (usually huge-Δv) arc silently — hence the fixed policy.
- **Edge case:** a 180° transfer (r1, r2 nearly colinear) leaves the plane indeterminate —
  guard and skip those cells.

## Ephemeris (Standish approximate elements — self-contained)
- Source: JPL SSD, **"Keplerian Elements for Approximate Positions of the Major Planets"**
  (E. M. Standish), Table 1, valid **1800–2050**, heliocentric mean ecliptic J2000.
- Algorithm: T = (JD_TT − 2451545.0)/36525 centuries; each element = value₀ + rate·T;
  ω = ϖ − Ω, M = L − ϖ (wrapped to [−180°, 180°]; for Jupiter–Neptune only, add Standish
  Table 2b terms b·T² + c·cos(fT) + s·sin(fT), which are zero for Mercury–Mars); solve Kepler
  E = M + e·sinE by Newton iteration; perifocal x′ = a(cosE − e), y′ = a√(1−e²) sinE;
  rotate R_z(−Ω) R_x(−I) R_z(−ω) into the ecliptic J2000 frame.
- **Velocity:** analytic — Ė = n/(1 − e·cosE), n = √(μ_sun/a³); perifocal ẋ′ = −a·sinE·Ė,
  ẏ′ = a√(1−e²)·cosE·Ė, rotated identically to position. (Cross-checked in tests against a
  finite difference.) Standish gives *position only*; the velocity route is standard practice,
  a flagged gap-fill, not sourced to Standish.
- **Accuracy:** ~20″ longitude, 8″ latitude, 6000 km distance for Earth over 1800–2050
  (JPL SSD). Adequate for porkchop scoping; stated honestly as an approximation. DE441 is a
  documented future upgrade.

## Time
- Calendar (UTC) → Julian Date via the USNO / Vallado `jday` algorithm. J2000 = JD 2451545.0 TT.
- Apply TT − UTC ≈ 69.184 s (= 32.184 + 37 leap seconds, IERS, current through 2026) when
  forming the ephemeris JD — negligible at day resolution, applied for consistency.
- TOF = (JD2 − JD1) · 86400 s, **both epochs on the same time scale.**

## Correctness traps (where a wrong constant/sign/unit silently corrupts results)
1. **μ_sun.** KFL's `consts.h` `K26A_GM_SUN = 1.327 124 420 99 × 10²⁰` is off by ~1.6×10⁻⁸
   (a TDB/TCB-era value mislabeled "IAU nominal"). **Do not use it** — use the DE440 value
   above. (Negligible against arc-minute ephemeris, corrected on principle.)
2. **km vs m.** KFL Lambert is SI (m); Standish gives a in AU. Convert AU→m before the solve;
   convert m→km only for display. One mix is a 1000× error.
3. **Lambert branch.** A wrong short/long-way or prograde/retrograde flag gives a wrong arc
   with no error signalled.
4. **TOF time scale.** Mixing a UTC-based JD for one epoch and a TT-based JD for the other is
   a classic silent bug; keep one scale.
5. **Anomaly wrapping.** Wrap M consistently before the Kepler iteration or convergence stalls
   or lands on the wrong root.
