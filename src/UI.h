#pragma once

class UI
{
public:
    static void Register();
    static void HelpMarker(const char* a_desc);
    static void __stdcall RenderMenu();

private:
    inline static char _consumableFilterBuf[256]{};
    inline static char _miscFilterBuf[256]{};
};
