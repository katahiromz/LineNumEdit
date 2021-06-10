#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include "resource.h"

#ifndef USE_DLL
    #include "LineNumEdit.hpp"
    static LineNumEdit s_hwndEdit;
#endif

BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
#ifndef USE_DLL
    s_hwndEdit.Attach(GetDlgItem(hwnd, edt1));
    s_hwndEdit.Prepare();
#endif
    return TRUE;
}

void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    switch (id)
    {
    case IDOK:
    case IDCANCEL:
        EndDialog(hwnd, id);
        break;
    }
}

INT_PTR CALLBACK
DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
        HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
    }
    return 0;
}

INT WINAPI
WinMain(HINSTANCE   hInstance,
        HINSTANCE   hPrevInstance,
        LPSTR       lpCmdLine,
        INT         nCmdShow)
{
    InitCommonControls();
#ifdef USE_DLL
    LoadLibraryA("LineNumEdit");
#endif
    DialogBoxW(hInstance, MAKEINTRESOURCEW(IDD_MAIN), NULL, DialogProc);
    return 0;
}
