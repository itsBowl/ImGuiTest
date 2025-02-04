#pragma once
#include <imgui/imgui.h>

namespace ax {
namespace Drawing
{
	enum class IconType : ImU32 { Flow, Circle, Square, Grid, RoundSquare, Diamond };

	void drawIcon(ImDrawList*, const ImVec2&, const ImVec2&, IconType, bool, ImU32, ImU32);
}
}