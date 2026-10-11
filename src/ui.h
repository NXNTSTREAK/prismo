// Adwaita-style UI drawn with Dear ImGui; shared by the Linux and Windows front ends.
#pragma once
#include "imgui.h"

namespace ui {

// Call once after ImGui::CreateContext(). `scale` is the UI scale (1 = 96 dpi).
void setup(float scale, bool dark);

// Draws the whole app for this frame. Returns true while something is animating
// (the caller should keep rendering instead of waiting for input).
bool frame(bool draw_header);

ImVec4 background();

}  // namespace ui
