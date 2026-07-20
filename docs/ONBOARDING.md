# Onboarding

For whoever inherits this code. Porkchop is small — the whole compute core is three files — and
worth reading start to finish before changing anything.

## Map of the source tree

- `src/core/` — the physics, no UI dependency:
  - `constants.h` — every cited physical constant (μ_sun from JPL DE440, the AU, obliquity, unit
    helpers). Deliberately overrides KFL's own solar-GM constant, which the pre-flight audit found
    to be a slightly wrong (TDB/TCB-era) value — see the comment there and
    `docs/AUDIT_porkchop_pre-flight_2026-07-20.md`.
  - `julian.*` — calendar ↔ Julian Date (Vallado Algorithm 14), and the TT − UTC offset applied
    before any ephemeris lookup.
  - `ephemeris.*` — JPL's Standish Table 1 elements plus the Kepler solve, exposed as
    `planet_state(planet, jd_tt)`: heliocentric position + velocity, SI units. Note `EARTH` here is
    actually the Earth–Moon barycentre, as Standish's table gives it.
  - `transfer.*` — the `Grid`/`Cell` types and `compute_porkchop()`, which calls into KFL's Lambert
    solver for every grid cell and derives C3 and arrival v∞ from the result.
- `src/ui/` — the ImGui interface: `ui.h` defines `AppState` (all mutable UI state plus the
  computed `Grid`); `ui.cpp` has `draw_ui()` and the four panels (Controls, Porkchop heatmap,
  Trajectory, Details), the docking layout, and the heatmap texture upload; `colormap.h` is the
  blue-to-red energy ramp.
- `src/app/main.cpp` — the GLFW/OpenGL/ImGui bootstrap and frame loop. Calls `draw_ui(state)` once
  per frame; knows nothing about astrodynamics.
- `third_party/imgui/` — vendored Dear ImGui (docking branch), unmodified.
- `test/test_porkchop.cpp` — a headless sanity check (ephemeris distances + a known Earth→Mars
  window). See "Rough edges" below — it isn't currently wired into the build.
- `docs/DESIGN.md` — the design premises, written before the code, every constant cited.
- `docs/AUDIT_porkchop_pre-flight_2026-07-20.md` — the fact-check audit of that design.

## Data flow

1. `AppState` (`ui.h`) holds the current request: departure/arrival planet, year ranges, grid
   resolution, which metric to color by, and Type I/II.
2. Any control change sets `s.dirty`; `draw_ui()` then calls `recompute()` (`ui.cpp`), which:
   - turns the year ranges into Julian Dates,
   - calls `compute_porkchop()` (`transfer.cpp`), which for every `(i, j)` grid cell calls
     `planet_state()` (`ephemeris.cpp`) for both planets and passes their positions and the time
     of flight to KFL's `k26astro_lambert()`, filling a `Cell` with C3/v∞ for both Type I and
     Type II,
   - finds the grid minimum and sets the auto colour scale,
   - uploads the whole grid into an RGBA texture (`upload_texture()`), one pixel per cell.
3. Each frame, `panel_porkchop()` draws that texture, handles clicks (setting `sel_i`/`sel_j`),
   and overlays the minimum ring and selection crosshair. `panel_trajectory()` separately re-runs
   `planet_state()` plus one more `k26astro_lambert()` and `k26astro_kepler_propagate()` — only
   for the *selected* cell — to draw both orbits and the transfer arc. `panel_details()` just
   reads the same `Grid`/`Cell` values as text.

In short: `planet_state()` → `compute_porkchop()` → `Grid` of `Cell`s → heatmap texture (whole
grid) + trajectory arc (selected cell, re-solved live, not cached in the `Grid`).

## Where to start reading

1. `src/app/main.cpp` — the entire program is a GLFW window plus one `draw_ui()` call per frame.
   No astrodynamics here.
2. `src/core/transfer.cpp` — `compute_porkchop()` is the actual algorithm; it's short, and reading
   it top to bottom is reading the whole pipeline (ephemeris in, Lambert through KFL, C3/v∞ out).
3. `src/core/ephemeris.cpp` — `planet_state()`, the Standish elements and the Kepler solve that
   turns a date into a position and velocity. The piece most likely to need attention later (see
   the DE441 upgrade note below).
4. Then `src/ui/ui.cpp`, to see how the grid becomes pixels and how selection and the trajectory
   view work.

## Recipe: change the grid resolution, or add a metric

**Change the grid resolution.** Two different things control this:
- Defaults: `AppState::nd`, `AppState::na` in `src/ui/ui.h` (currently 160 × 200).
- User-adjustable range: `ImGui::SliderInt("dep res", &s.nd, 40, 400)` and the equivalent
  `"arr res"` call in `panel_controls()` (`src/ui/ui.cpp`) — edit the `40, 400` bounds there.
Either change takes effect on the next `recompute()` (any control edit sets `s.dirty`, which
triggers one automatically).

**Add a metric.** Today there are two (`metric == 0` → C3, `metric == 1` → arrival v∞). To add a
third — say, DESIGN.md's total-Δv proxy (C3 + arrival v∞) — you'd touch four spots:
1. Compute the new value — derive it on the fly from existing `Cell` fields, or add a field to
   `Cell` (`src/core/transfer.h`) and fill it in `solve_arc()` (`transfer.cpp`).
2. Add a branch for it in `cell_value()` at the top of `src/ui/ui.cpp`.
3. Add its label to the `ImGui::Combo("colour", &s.metric, "launch C3\0arrival v-infinity\0")`
   string in `panel_controls()`.
4. Give it a sensible auto-scale span in `recompute()` — the line
   `s.cmax = (float)(mv + (s.metric == 0 ? 40.0 : 4.0));` decides how far above the minimum the
   colour scale extends; a third metric needs a third case there, plus updated unit/label strings
   in `panel_porkchop()` and `panel_details()`.

## Rough edges / where it could grow

- **Ephemeris is approximate, not DE441.** Standish Table 1 is arc-second-level and only valid
  1800–2050. A DE441 SPICE-kernel-based ephemeris is the natural upgrade for real fidelity, at the
  cost of no longer being a small, self-contained tool.
- **Patched-conic C3 only — no departure/capture burn Δv.** Porkchop reports launch C3 and arrival
  v∞, which need only μ_sun. It doesn't compute an actual departure Δv (needs the departure
  planet's own GM plus a parking-orbit assumption) or a capture burn at arrival — explicitly out
  of scope for v1 (see DESIGN.md).
- **No explicit contour lines.** The heatmap is a direct colour-mapped grid, not a contoured plot;
  there's a white ring on the single minimum cell but no iso-C3 contour lines.
- **`KFL_DIR` is a manual path, not a real dependency lookup.** `CMakeLists.txt` has no default for
  `KFL_DIR` — you pass `-DKFL_DIR`, or the configure step stops with a clear "KFL not found" error
  — and there's no `find_package`-style version or compatibility check against KFL, so a mismatched
  or outdated KFL tree fails at the linker rather than at configure time.
- **Type I/II only, no multi-revolution.** Porkchop calls KFL's base `k26astro_lambert()` (single
  revolution), not `k26astro_lambert_multi_rev()`. Multi-rev transfers — useful for some
  low-energy windows — aren't modeled or offered as an option.
- **The trajectory view is a flat 2-D projection.** `sample_orbit()` and `panel_trajectory()` in
  `ui.cpp` project only the ecliptic x/y components of each `State`; the (usually small) z
  component — out-of-plane motion from nonzero orbital inclination — is silently dropped. Fine for
  a quick-look view, misleading if read as a precise 3-D picture.
- **`test/test_porkchop.cpp` isn't wired into the build.** It's a real headless check (ephemeris
  sanity plus a known Earth→Mars window), but `CMakeLists.txt` currently only builds the `imgui`
  and `porkchop` targets — there's no test target. Worth adding before making further changes to
  `core/`.
