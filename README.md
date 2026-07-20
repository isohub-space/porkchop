# Porkchop

An interplanetary transfer designer: pick two planets and a pair of date ranges, and Porkchop
draws the classic "porkchop plot" of launch energy for every possible departure/arrival pairing.

## What it is

Porkchop is a small desktop application (C++17, dear ImGui, OpenGL — Linux only) for exploring
interplanetary launch windows. You choose a departure planet, an arrival planet, and a range of
calendar years for each end of the trip. Porkchop solves Lambert's problem for every date pair in
the resulting grid and paints a heatmap of the launch energy C3, alongside a heliocentric view of
the trajectory for whichever cell you've selected. It's a scoping tool — good for seeing *when* a
transfer is cheap and roughly *how* cheap — not a navigation-grade tool.

## Build & run on Linux

Porkchop depends on **KFL**, an astrodynamics toolkit that is a separate project and is not
vendored here: https://github.com/savannah-i-g/KFL. Clone and build KFL first (per its own
instructions), then point Porkchop's build at the resulting tree.

Install the system dependencies (Debian/Ubuntu package names):

```
sudo apt install build-essential cmake pkg-config libglfw3-dev libgl-dev
```

(`libgl-dev` is the OpenGL development headers package; on older Ubuntu releases this may be
named `libgl1-mesa-dev` instead.)

Then build Porkchop against your built KFL tree:

```
cmake -S . -B build -DKFL_DIR=/path/to/KFL && cmake --build build -j
./build/porkchop
```

`-DKFL_DIR` must point at the root of a *built* KFL tree (the directory containing
`libk26astro_conics/`, `libk26astro_body/`, `libk26astro_core/`, and `libk26m3d/` with their
compiled `.a` archives). It has no default — pass `-DKFL_DIR` explicitly, or the build stops with
a clear "KFL not found" message. Dear ImGui (docking branch) is vendored under `third_party/imgui/`
and needs nothing extra.

## How it works

For every (departure date, arrival date) pair in the grid, Porkchop looks up both planets'
heliocentric position and velocity from JPL's Standish approximate ephemeris, then hands the two
position vectors and the time of flight to KFL's Lambert solver to get the transfer orbit's
velocity at each end. The departure speed relative to the departure planet gives the launch energy
C3 = |v∞|²; the arrival speed relative to the arrival planet gives the arrival v∞. Every grid cell
becomes one pixel of the heatmap, and the same Lambert solve — run once more for whichever cell is
selected — draws the transfer arc in the trajectory view.

## Reading a porkchop plot

- Each pixel is one departure/arrival date pair. **Blue is low C3 — a cheap, good launch
  window; red is high C3 — expensive.** Dark, uncolored cells mean no valid transfer was found
  there (the flight is too short, or the transfer angle is near 180° where the orbit plane is
  indeterminate).
- The **white ring** marks the single cheapest cell (the minimum) in the current grid.
- **Click anywhere on the plot** to inspect that cell — a red crosshair marks your selection, and
  the Details panel fills in its dates, time of flight, C3, and arrival v∞.
- The **Trajectory** panel shows a top-down heliocentric view: both planets' orbits, their
  positions on the selected dates, and the transfer arc itself (in white), re-solved live for
  whatever cell you last clicked.

Controls on the left let you choose the two planets, the departure/arrival year ranges, which
quantity to color by (launch C3 or arrival v∞), the transfer type (Type I short-way / Type II
long-way), and the grid resolution.

## Status / accuracy

Planet positions come from JPL's Standish "Approximate Positions of the Planets" Keplerian
elements, valid **1800–2050**. Accuracy is arc-second level (about 20″ in longitude, 6000 km in
distance, for Earth) — adequate for scoping which windows exist and roughly what they cost, but
**not** for navigation-grade planning. A full DE441 kernel is a documented future upgrade. Only
direct (zero-revolution) **Type I / Type II** transfers are computed; multi-revolution transfers
aren't yet an option.

## License and provenance

MIT License — see `LICENSE`.

Porkchop was designed and written by an autonomous Claude (Opus 4.8) instance operating within
the Athena framework, in July 2026. See `NOTICE` for full dependency and data provenance
(KFL, Dear ImGui, the JPL ephemeris source, and the DE440 solar GM value), and `docs/DESIGN.md`
and `docs/AUDIT_porkchop_pre-flight_2026-07-20.md` for the cited physical constants and their
verification.
