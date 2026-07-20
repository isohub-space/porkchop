# Pre-flight audit — Porkchop (Lambert / porkchop transfer designer)

Date: 2026-07-20
Method: claim inventory + primary-source verification (WebFetch of canonical URLs) +
independent re-derivation. No code exists yet. Scope: `docs/DESIGN.md`.

## CRITICAL — fix in the plan before coding
None. (The one critical-class hazard — a wrong GM☉ — is already designed around; see
MEDIUM-1 and "Verified.")

## HIGH — resolve before the relevant sub-phase
None.

## MEDIUM — track before/while coding
- **M-1 (regression guard) — do not use KFL's `K26A_GM_SUN`.** KFL `consts.h` carries
  `1.32712442099e20 m³/s²`, which is wrong by ~1.6×10⁻⁸ (a TDB/TCB-era value mislabeled
  "IAU nominal"). The design correctly specifies the DE440 value instead. This is only a
  finding in the sense that the code must **not** regress to KFL's constant; keep µ_sun a
  Porkchop-owned constant, cited. Verified below.
- **M-2 — Standish outer-planet correction terms.** For Jupiter–Neptune, Standish Table 2b
  adds `b·T² + c·cos(fT) + s·sin(fT)` to the mean anomaly. The design's `M = L − ϖ` is
  correct **only for the inner planets** (Table 1: Mercury…Mars, where b=c=s=f=0). Earth↔Mars
  v1 is unaffected; if the tool ever adds Jupiter+, these terms are mandatory or the giant
  planets are silently mis-placed by up to ~degrees. Flag carried to code.
- **M-3 (doc precision) — accuracy phrasing.** DESIGN.md says "~arc-minutes"; the JPL page
  states 20″ longitude, 8″ latitude, 6000 km distance for Earth over 1800–2050. Tighten the
  doc to the real figures (sub-arcminute in angle).

## Insufficiently specified — decide before coding
- **S-1 — Lambert branch selection per cell.** The design names the branch flags but not the
  policy. Decision: default **prograde**, **n_rev = 0** (direct Type I/II); short-way vs
  long-way chosen by the transfer angle (sign of the z-component of r1×r2 in the ecliptic
  frame). Multi-revolution is a later optional toggle, not v1.
- **S-2 — velocity from Standish elements.** "Analytic or finite difference" must be one.
  Decision: **analytic** — Ė = n/(1 − e·cosE), n = √(µ_sun/a³), then the perifocal velocity
  ẋ′ = −a·sinE·Ė, ẏ′ = a√(1−e²)·cosE·Ė, rotated identically to position. Keep a
  finite-difference cross-check in tests.

## Verified against primary source (three-tier)
- **µ_sun = 1.3271244004127942e20 m³/s²** — (a) DESIGN.md §Constants; (b) NAIF `gm_de440.tpc`
  `BODY10_GM = 1.3271244004127942E+11 km³/s²`, ref Park et al. 2021, AJ 161:105, fetched
  directly; (c) km³/s²→m³/s² is ×10⁹ → 1.3271244004127942e20, matches to all 17 digits. VERIFIED.
- **Obliquity ε = 23.43928°** — (a) DESIGN.md; (b) JPL SSD approx_pos page, fetched; (c) matches. VERIFIED.
- **C3 = |v∞|², v∞ = v_transfer − v_planet** — (a) DESIGN.md; (b) standard patched-conic
  (Vallado; NASA Trajectory Browser: "C3 = square of hyperbolic excess velocity"); (c)
  re-derived from vis-viva: ε = v²/2 − µ/r → v∞²/2 as r→∞, and C3 ≡ 2ε = v∞². Dimensions
  m²/s² ✓. VERIFIED.
- **Lambert (KFL, Vallado Alg 58 / Izzo 2015)** — (a) DESIGN.md; (b) KFL header cites the
  methods; (c) numerically re-verified: KFL reproduces Vallado Example 7-5 to < 0.2 m/s from
  our own linked test. VERIFIED.
- **Standish algorithm & Earth/Mars elements** — (a) DESIGN.md §Ephemeris; (b) JPL SSD page,
  fetched (element rows + rotation sequence + Kepler solve); (c) step sequence matches. VERIFIED.

## Citation gaps / lower-tier
- **AU = 149 597 870 700 m (IAU 2012 B2).** Defining constant, exact. The primary IAU B2 PDF
  is a scanned image (not text-verifiable); value corroborated by three independent secondary
  sources and universally established. Tier: citation-verified (secondary); not in doubt.
- **TDB−TT periodic term** — sub-2 ms, negligible at day-resolution; deferred, not needed.

## Verdict
Design is sound to build. No CRITICAL/HIGH. The dominant risk (GM☉) is primary-source
confirmed and already designed around. Resolve S-1/S-2 (done, folded into DESIGN.md), carry
M-2 as a code-time guard for any outer-planet extension.
