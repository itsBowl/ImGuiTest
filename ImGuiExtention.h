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

    int getTextureWidth(ImTextureID);
    int getTextureHeight(ImTextureID);


}
