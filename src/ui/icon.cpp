#include "icon.h"

#include <raylib.h>

#if defined(EMBEDDED_ICON)
extern "C"
{
    extern const unsigned char _binary_assets_icon_icon_png_start[];
    extern const unsigned char _binary_assets_icon_icon_png_end[];
}
#endif

void WFZSetWindowIcon()
{
#if defined(EMBEDDED_ICON)
    const int icon_data_size = static_cast<int>(_binary_assets_icon_icon_png_end - _binary_assets_icon_icon_png_start);
    Image icon = LoadImageFromMemory(".png", _binary_assets_icon_icon_png_start, icon_data_size);
#else
    Image icon = LoadImage("assets/icon/icon.png");
#endif

    if (IsImageValid(icon))
    {
        SetWindowIcon(icon);
        UnloadImage(icon);
    }
}
