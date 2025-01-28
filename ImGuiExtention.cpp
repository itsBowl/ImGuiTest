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

	int getTextureWidth(ImTextureID texture)
    {
        if (TEXTURE* tex = (TEXTURE*)(texture))
            return tex->Width;
        else
            return 0;
    }

    int getTextureHeight(ImTextureID texture)
    {
        if (TEXTURE* tex = (TEXTURE*)(texture))
            return tex->Height;
        else
            return 0;
    }






}