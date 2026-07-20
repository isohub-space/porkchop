#include "ui/ui.h"
#include "ui/colormap.h"
#include "core/constants.h"
#include "core/julian.h"
#include "core/ephemeris.h"

#include "imgui.h"
#include "imgui_internal.h"   // DockBuilder
#include <GL/gl.h>
#include <cmath>
#include <cstdio>
#include <vector>

extern "C" {
#include "k26astro_conics/lambert.h"
#include "k26astro_conics/kepler.h"
#include "k26m3d.h"
}

using namespace pk;

// ---- small helpers ---------------------------------------------------------------

// The scalar shown for a cell, given the current metric and Type; false if invalid.
static bool cell_value(const Cell& c, int metric, int type, double& out) {
    bool valid = (type == 0) ? c.valid_I : c.valid_II;
    if (!valid) return false;
    if (type == 0) out = (metric == 0) ? c.c3_I : c.vinf_arr_I;
    else           out = (metric == 0) ? c.c3_II : c.vinf_arr_II;
    return true;
}

static void find_min(const AppState& s, int& mi, int& mj, double& mv) {
    mi = mj = -1; mv = 1e30;
    for (int i = 0; i < s.grid.nd; ++i)
        for (int j = 0; j < s.grid.na; ++j) {
            double v;
            if (cell_value(s.grid.at(i, j), s.metric, s.type, v) && v < mv) {
                mv = v; mi = i; mj = j;
            }
        }
}

// JD -> calendar (Richards algorithm, Gregorian).
static void jd_to_ymd(double jd, int& y, int& m, int& d) {
    long J = (long)(jd + 0.5);
    long f = J + 1401 + (((4 * J + 274277) / 146097) * 3) / 4 - 38;
    long e = 4 * f + 3;
    long g = (e % 1461) / 4;
    long h = 5 * g + 2;
    d = (int)((h % 153) / 5 + 1);
    m = (int)((h / 153 + 2) % 12 + 1);
    y = (int)(e / 1461 - 4716 + (12 + 2 - m) / 12);
}

static void date_label(double jd, char* buf, size_t n) {
    int y, m, d; jd_to_ymd(jd, y, m, d);
    std::snprintf(buf, n, "%04d-%02d-%02d", y, m, d);
}

// ---- compute + texture -----------------------------------------------------------

static void upload_texture(AppState& s) {
    int W = s.grid.nd, H = s.grid.na;
    if (W <= 0 || H <= 0) return;
    std::vector<unsigned char> px((size_t)W * H * 4);
    for (int i = 0; i < W; ++i)
        for (int j = 0; j < H; ++j) {
            double v;
            bool ok = cell_value(s.grid.at(i, j), s.metric, s.type, v);
            int row = H - 1 - j;   // flip so larger arrival date is at the top
            size_t idx = ((size_t)row * W + i) * 4;
            if (!ok) {
                px[idx] = 26; px[idx + 1] = 28; px[idx + 2] = 38; px[idx + 3] = 255;
            } else {
                float t = (float)((v - s.cmin) / (s.cmax - s.cmin + 1e-9));
                unsigned char r, g, b;
                colormap_energy(t, r, g, b);
                px[idx] = r; px[idx + 1] = g; px[idx + 2] = b; px[idx + 3] = 255;
            }
        }
    if (s.tex == 0) glGenTextures(1, &s.tex);
    glBindTexture(GL_TEXTURE_2D, s.tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, W, H, 0, GL_RGBA, GL_UNSIGNED_BYTE, px.data());
}

static void recompute(AppState& s) {
    // Clamp to Standish Table 1 validity and sane ordering.
    auto clamp_year = [](int y) { return y < 1800 ? 1800 : (y > 2050 ? 2050 : y); };
    s.dep_y0 = clamp_year(s.dep_y0); s.dep_y1 = clamp_year(s.dep_y1);
    s.arr_y0 = clamp_year(s.arr_y0); s.arr_y1 = clamp_year(s.arr_y1);
    if (s.dep_y1 < s.dep_y0) s.dep_y1 = s.dep_y0;
    if (s.arr_y1 < s.arr_y0) s.arr_y1 = s.arr_y0;

    double d0 = julian_date(s.dep_y0, 1, 1, 0, 0, 0);
    double d1 = julian_date(s.dep_y1, 12, 31, 0, 0, 0);
    double a0 = julian_date(s.arr_y0, 1, 1, 0, 0, 0);
    double a1 = julian_date(s.arr_y1, 12, 31, 0, 0, 0);

    s.grid = compute_porkchop(s.dep_planet, s.arr_planet, d0, d1, s.nd, a0, a1, s.na);

    // Auto colour scale: from the minimum up by a fixed span, so the low-energy basin
    // shows detail rather than being washed out by the worst cells.
    int mi, mj; double mv;
    find_min(s, mi, mj, mv);
    if (mi < 0) { s.cmin = 0.0f; s.cmax = 1.0f; }
    else {
        s.cmin = (float)mv;
        s.cmax = (float)(mv + (s.metric == 0 ? 40.0 : 4.0));
    }
    s.sel_i = mi; s.sel_j = mj;   // open on the best window, arc shown
    upload_texture(s);
    s.dirty = false;
}

// ---- panels ----------------------------------------------------------------------

static void build_layout(ImGuiID dock, ImVec2 size) {
    ImGui::DockBuilderRemoveNode(dock);
    ImGui::DockBuilderAddNode(dock, ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(dock, size);
    ImGuiID center = dock;
    ImGuiID left  = ImGui::DockBuilderSplitNode(center, ImGuiDir_Left,  0.20f, nullptr, &center);
    ImGuiID right = ImGui::DockBuilderSplitNode(center, ImGuiDir_Right, 0.34f, nullptr, &center);
    ImGuiID right_bottom = ImGui::DockBuilderSplitNode(right, ImGuiDir_Down, 0.5f, nullptr, &right);
    ImGui::DockBuilderDockWindow("Controls", left);
    ImGui::DockBuilderDockWindow("Porkchop", center);
    ImGui::DockBuilderDockWindow("Trajectory", right);
    ImGui::DockBuilderDockWindow("Details", right_bottom);
    ImGui::DockBuilderFinish(dock);
}

static void panel_controls(AppState& s) {
    if (ImGui::Begin("Controls")) {
        const char* names = "Mercury\0Venus\0Earth\0Mars\0Jupiter\0Saturn\0Uranus\0Neptune\0";
        if (ImGui::Combo("from", &s.dep_planet, names)) s.dirty = true;
        if (ImGui::Combo("to", &s.arr_planet, names)) s.dirty = true;
        ImGui::Separator();
        ImGui::TextUnformatted("Departure years (1800-2050)");
        if (ImGui::InputInt("dep from", &s.dep_y0)) s.dirty = true;
        if (ImGui::InputInt("dep to", &s.dep_y1)) s.dirty = true;
        ImGui::TextUnformatted("Arrival years");
        if (ImGui::InputInt("arr from", &s.arr_y0)) s.dirty = true;
        if (ImGui::InputInt("arr to", &s.arr_y1)) s.dirty = true;
        ImGui::Separator();
        if (ImGui::Combo("colour", &s.metric, "launch C3\0arrival v-infinity\0")) s.dirty = true;
        if (ImGui::Combo("type", &s.type, "Type I (short way)\0Type II (long way)\0")) s.dirty = true;
        if (ImGui::SliderInt("dep res", &s.nd, 40, 400)) s.dirty = true;
        if (ImGui::SliderInt("arr res", &s.na, 40, 400)) s.dirty = true;
        ImGui::Separator();
        ImGui::TextWrapped("Click the plot to select a transfer. Blue is low energy "
                           "(a good window); the white ring marks the minimum.");
    }
    ImGui::End();
}

static void panel_porkchop(AppState& s) {
    if (ImGui::Begin("Porkchop")) {
        char buf[64];
        std::snprintf(buf, sizeof buf, "%s  ->  %s     colour: %s",
                      planet_name(s.dep_planet), planet_name(s.arr_planet),
                      s.metric == 0 ? "launch C3 (km^2/s^2)" : "arrival v-inf (km/s)");
        ImGui::TextUnformatted(buf);

        ImVec2 avail = ImGui::GetContentRegionAvail();
        float pw = avail.x - 4.0f, ph = avail.y - 22.0f;
        if (pw < 60.0f) pw = 60.0f;
        if (ph < 60.0f) ph = 60.0f;

        if (s.tex) {
            ImGui::Image((ImTextureID)(intptr_t)s.tex, ImVec2(pw, ph));
            ImVec2 p0 = ImGui::GetItemRectMin(), p1 = ImGui::GetItemRectMax();

            if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                ImVec2 m = ImGui::GetMousePos();
                float u = (m.x - p0.x) / (p1.x - p0.x);
                float v = (m.y - p0.y) / (p1.y - p0.y);
                int i = (int)(u * s.grid.nd);
                int j = (int)((1.0f - v) * s.grid.na);
                if (i < 0) i = 0; if (i >= s.grid.nd) i = s.grid.nd - 1;
                if (j < 0) j = 0; if (j >= s.grid.na) j = s.grid.na - 1;
                s.sel_i = i; s.sel_j = j;
            }

            ImDrawList* dl = ImGui::GetWindowDrawList();
            auto to_screen = [&](int i, int j) {
                float x = p0.x + (i + 0.5f) / s.grid.nd * (p1.x - p0.x);
                float y = p0.y + (1.0f - (j + 0.5f) / s.grid.na) * (p1.y - p0.y);
                return ImVec2(x, y);
            };
            int mi, mj; double mv; find_min(s, mi, mj, mv);
            if (mi >= 0) {
                ImVec2 c = to_screen(mi, mj);
                dl->AddCircle(c, 7.0f, IM_COL32(255, 255, 255, 235), 20, 2.0f);
            }
            if (s.sel_i >= 0) {
                ImVec2 c = to_screen(s.sel_i, s.sel_j);
                dl->AddLine(ImVec2(c.x - 9, c.y), ImVec2(c.x + 9, c.y), IM_COL32(255, 70, 70, 255), 1.5f);
                dl->AddLine(ImVec2(c.x, c.y - 9), ImVec2(c.x, c.y + 9), IM_COL32(255, 70, 70, 255), 1.5f);
            }
            // axis labels
            ImU32 lab = IM_COL32(170, 175, 190, 255);
            char a[16];
            std::snprintf(a, sizeof a, "%d", s.dep_y0); dl->AddText(ImVec2(p0.x, p1.y + 3), lab, a);
            std::snprintf(a, sizeof a, "%d", s.dep_y1); dl->AddText(ImVec2(p1.x - 28, p1.y + 3), lab, a);
            dl->AddText(ImVec2(p0.x + (p1.x - p0.x) * 0.5f - 30, p1.y + 3), lab, "departure ->");
        }
    }
    ImGui::End();
}

static void panel_details(AppState& s) {
    if (ImGui::Begin("Details")) {
        char db[16], ab[16];
        int mi, mj; double mv; find_min(s, mi, mj, mv);
        if (mi >= 0) {
            date_label(s.grid.dep_jd(mi), db, sizeof db);
            date_label(s.grid.arr_jd(mj), ab, sizeof ab);
            const Cell& c = s.grid.at(mi, mj);
            ImGui::TextUnformatted("Best window (minimum):");
            ImGui::Text("  %s : %.2f %s", s.metric == 0 ? "C3" : "v-inf", mv,
                        s.metric == 0 ? "km^2/s^2" : "km/s");
            ImGui::Text("  depart %s", db);
            ImGui::Text("  arrive %s", ab);
            ImGui::Text("  time of flight %.0f days", c.tof_days);
        } else {
            ImGui::TextUnformatted("No valid transfer in this window.");
        }
        ImGui::Separator();
        if (s.sel_i >= 0) {
            date_label(s.grid.dep_jd(s.sel_i), db, sizeof db);
            date_label(s.grid.arr_jd(s.sel_j), ab, sizeof ab);
            const Cell& c = s.grid.at(s.sel_i, s.sel_j);
            ImGui::TextUnformatted("Selected transfer:");
            ImGui::Text("  depart %s", db);
            ImGui::Text("  arrive %s", ab);
            ImGui::Text("  time of flight %.0f days", c.tof_days);
            if (c.valid_I)
                ImGui::Text("  Type I : C3 %.2f, arrival v-inf %.2f", c.c3_I, c.vinf_arr_I);
            if (c.valid_II)
                ImGui::Text("  Type II: C3 %.2f, arrival v-inf %.2f", c.c3_II, c.vinf_arr_II);
        } else {
            ImGui::TextDisabled("Click the plot to inspect a transfer.");
        }
    }
    ImGui::End();
}

// Sample a planet's orbit into ecliptic-plane AU points (one period).
static void sample_orbit(int planet, double jd0, std::vector<ImVec2>& out) {
    out.clear();
    State s0 = planet_state(planet, jd0);
    double r = std::sqrt(s0.r[0]*s0.r[0] + s0.r[1]*s0.r[1] + s0.r[2]*s0.r[2]);
    double v = std::sqrt(s0.v[0]*s0.v[0] + s0.v[1]*s0.v[1] + s0.v[2]*s0.v[2]);
    double a = 1.0 / (2.0 / r - v * v / MU_SUN);          // m
    double period_days = 2.0 * M_PI * std::sqrt(a * a * a / MU_SUN) / SEC_PER_DAY;
    const int N = 160;
    for (int k = 0; k <= N; ++k) {
        State st = planet_state(planet, jd0 + (double)k / N * period_days);
        out.push_back(ImVec2((float)(st.r[0] / AU), (float)(st.r[1] / AU)));
    }
}

static void panel_trajectory(AppState& s) {
    if (ImGui::Begin("Trajectory")) {
        ImVec2 avail = ImGui::GetContentRegionAvail();
        if (avail.x < 40) avail.x = 40;
        if (avail.y < 40) avail.y = 40;
        ImVec2 org = ImGui::GetCursorScreenPos();
        ImGui::InvisibleButton("sys", avail);
        ImVec2 center = ImVec2(org.x + avail.x * 0.5f, org.y + avail.y * 0.5f);
        ImDrawList* dl = ImGui::GetWindowDrawList();

        double ref_jd = (s.sel_i >= 0) ? s.grid.dep_jd(s.sel_i) : s.grid.dep_jd0;

        std::vector<ImVec2> dep_orbit, arr_orbit;
        sample_orbit(s.dep_planet, ref_jd, dep_orbit);
        sample_orbit(s.arr_planet, ref_jd, arr_orbit);

        double maxr = 1e-6;
        for (auto& p : dep_orbit) maxr = std::fmax(maxr, std::sqrt(p.x*p.x + p.y*p.y));
        for (auto& p : arr_orbit) maxr = std::fmax(maxr, std::sqrt(p.x*p.x + p.y*p.y));
        float scale = 0.44f * std::min(avail.x, avail.y) / (float)maxr;
        auto proj = [&](ImVec2 au) { return ImVec2(center.x + au.x * scale, center.y - au.y * scale); };

        auto draw_orbit = [&](std::vector<ImVec2>& o, ImU32 col) {
            for (size_t k = 1; k < o.size(); ++k) dl->AddLine(proj(o[k-1]), proj(o[k]), col, 1.2f);
        };
        draw_orbit(dep_orbit, IM_COL32(120, 150, 220, 180));
        draw_orbit(arr_orbit, IM_COL32(220, 150, 110, 180));

        dl->AddCircleFilled(center, 4.0f, IM_COL32(255, 220, 90, 255));  // the Sun

        if (s.sel_i >= 0) {
            double dep_jd = s.grid.dep_jd(s.sel_i), arr_jd = s.grid.arr_jd(s.sel_j);
            State sd = planet_state(s.dep_planet, dep_jd);
            State sa = planet_state(s.arr_planet, arr_jd);
            ImVec2 pd = proj(ImVec2((float)(sd.r[0]/AU), (float)(sd.r[1]/AU)));
            ImVec2 pa = proj(ImVec2((float)(sa.r[0]/AU), (float)(sa.r[1]/AU)));
            dl->AddCircleFilled(pd, 4.0f, IM_COL32(150, 190, 255, 255));
            dl->AddCircleFilled(pa, 4.0f, IM_COL32(255, 180, 130, 255));

            // Transfer arc: solve Lambert for the selected type, propagate (r1, v1).
            double tof = (arr_jd - dep_jd) * SEC_PER_DAY;
            K26V3 r1 = { sd.r[0], sd.r[1], sd.r[2] };
            K26V3 r2 = { sa.r[0], sa.r[1], sa.r[2] };
            K26V3 vt1, vt2;
            int dir = (s.type == 0) ? K26A_LAMBERT_SHORT_WAY : K26A_LAMBERT_LONG_WAY;
            if (k26astro_lambert(&vt1, &vt2, r1, r2, MU_SUN, tof, dir) == 0) {
                const int N = 120;
                ImVec2 prev = pd; bool have = false;
                for (int k = 0; k <= N; ++k) {
                    double t = (double)k / N * tof;
                    K26V3 rp, vp;
                    if (k26astro_kepler_propagate(&rp, &vp, r1, vt1, MU_SUN, t, 32) == 0) {
                        ImVec2 cur = proj(ImVec2((float)(rp.x/AU), (float)(rp.y/AU)));
                        if (have) dl->AddLine(prev, cur, IM_COL32(240, 240, 245, 235), 1.6f);
                        prev = cur; have = true;
                    }
                }
            }
        }
        ImGui::SetCursorScreenPos(ImVec2(org.x + 4, org.y + 4));
        ImGui::TextDisabled(s.sel_i >= 0 ? "transfer arc (white)"
                                         : "select a cell to see the transfer");
    }
    ImGui::End();
}

// ---- top level -------------------------------------------------------------------

void draw_ui(AppState& s) {
    if (s.dirty) recompute(s);

    // Full-viewport dockspace host.
    ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->WorkPos);
    ImGui::SetNextWindowSize(vp->WorkSize);
    ImGui::SetNextWindowViewport(vp->ID);
    ImGuiWindowFlags host = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus |
        ImGuiWindowFlags_NoDocking;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("##host", nullptr, host);
    ImGui::PopStyleVar(3);
    ImGuiID dock = ImGui::GetID("PorkchopDock");
    ImGui::DockSpace(dock, ImVec2(0, 0));
    static bool built = false;
    if (!built) { built = true; build_layout(dock, vp->WorkSize); }
    ImGui::End();

    panel_controls(s);
    panel_porkchop(s);
    panel_trajectory(s);
    panel_details(s);
}
