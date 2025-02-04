#define IMGUI_DEFINE_MATH_OPERATIONS
#include "widgets.h"
#include <imgui/imgui_internal.h>

void ax::Widgets::Icon(const ImVec2& size, IconType type, bool filled, const ImVec4& col, const ImVec4& inCol)
{
    if (ImGui::IsRectVisible(size))
    {
        auto cursorPos = ImGui::GetCursorScreenPos();
        auto drawList = ImGui::GetWindowDrawList();
        ax::Drawing::drawIcon(drawList, cursorPos, cursorPos + size, type, filled, ImColor(col), ImColor(inCol));
    }

    ImGui::Dummy(size);
}