#include "ui/WinUITool.hpp"
#ifdef _WIN64
#define NOMINMAX
#include <dwmapi.h>
#pragma comment(lib, "Dwmapi.lib")
#endif


void WinUITool::set_caption_color(unsigned long long id, unsigned long color)
{
#ifdef _WIN64
    DwmSetWindowAttribute((HWND)id, DWMWA_CAPTION_COLOR, &color, sizeof(unsigned long));
#endif
}