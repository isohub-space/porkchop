# Post-flight audit — Porkchop

Date: 2026-07-20
Method: independent Opus-class outside-eye + fact-check on the built code — ephemeris
data re-checked against JPL Table 1, the perifocal→ecliptic rotation re-derived, KFL call
sites checked against the actual headers, date arithmetic independently re-run. Findings
were then fixed and the code re-tested.

## Verdict
SOUND-WITH-FIXES. The physics core is correct sign-for-sign and unit-for-unit; one real
range defect (julian_date) and three minor issues, all now resolved.

## Fixed
- **[medium] `julian_date` valid only 1900–2100** (no Gregorian century correction) while
  the UI allows 1800–2050 → pre-1900 dates were off 1–2 days, and a window straddling 1900
  gave the two endpoints different offsets, corrupting TOF. Replaced with the
  century-correct Fliegel–Van Flandern formula; the regression test now checks J2000
  (2451545.0) and 1800-01-01 (2378496.5). (The default Earth→Mars 2026 window was
  unaffected — 1901–2050 were already correct — which is why the screenshots were right.)
- **[low] `TT_MINUS_UTC`** documented as applied but never used. Comment + DESIGN corrected
  to "not applied; ~69 s, negligible and cancels in TOF."
- **[low] Type II labelled "prograde"** while KFL's `LONG_WAY` is documented retrograde.
  Comment corrected; Type I flagged as the validated primary result.
- **[doc] Regression test not built; `KFL_DIR` unvalidated.** Added a `porkchop_test` CMake
  target and a KFL-not-found `FATAL_ERROR`.

## Verified correct (outside eye, against source + independent arithmetic)
- **Ephemeris:** all 8 planets' Table 1 elements/rates match JPL; the AU↔m split is
  consistent for position *and* velocity; the 3-1-3 rotation matches the standard matrix
  sign-for-sign; Kepler solve and `wrap180` correct; Earth ≈ 29.8 km/s and P ≈ 365 d fall
  out of the analytic velocity.
- **Constants:** `MU_SUN` = NAIF `gm_de440` `BODY10_GM` ×1e9 exactly; `AU` exact (IAU 2012);
  KFL's wrong `K26A_GM_SUN` correctly avoided.
- **Transfer:** Lambert call site matches the header; `C3 = |v_t1 − v_dep|²·1e-6` and
  `v∞_arr = |v_t2 − v_arr|·1e-3` dimensionally correct; TOF = days·86400; MIN_TOF guard and
  grid stride correct.
- **UI/general:** GL texture generated once (no per-frame leak); colormap index bounded;
  divide-by-zero guarded (`cmax−cmin+1e-9`, `maxr`, `1−e·cosE`); click→cell mapping and NaN
  handling sound.

Re-tested after fixes: `porkchop_test` ALL PASS; GUI builds and runs clean.
