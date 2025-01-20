#pragma once
#include <imgui/imgui.h>

namespace ImGui
{
    struct TEXTURE
    {
        TEXTURE()
        {
            Width = 0;
            Height = 0;
        }

        int                         Width;
        int                         Height;
        ImVector<unsigned char>     Data;
    };

    int ImGui_GetTextureWidth(ImTextureID);
    int ImGui_GetTextureHeight(ImTextureID);
}
