#pragma once
#include <imgui/imgui.h>
#include "drawing.h"

namespace ax
{
namespace Widgets
{
	using Drawing::IconType;
	void Icon(const ImVec2&, IconType, bool, const ImVec4& co = ImVec4(1, 1, 1, 1), const ImVec4& inCol = ImVec4(0, 0, 0, 0));
}
}
