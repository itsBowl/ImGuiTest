#include "ImGuiExtention.h"

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

	int ImGui_GetTextureWidth(ImTextureID texture)
    {
        if (TEXTURE* tex = (TEXTURE*)(texture))
            return tex->Width;
        else
            return 0;
    }

    int ImGui_GetTextureHeight(ImTextureID texture)
    {
        if (TEXTURE* tex = (TEXTURE*)(texture))
            return tex->Height;
        else
            return 0;
    }






}