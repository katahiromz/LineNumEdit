#include "LineNumEdit.hpp"

BOOL WINAPI
DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        LineNumEdit::SuperclassWindow();
        break;
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
