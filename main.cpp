#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include "resource.h"

#ifndef LINENUMEDIT_SUPERCLASSING
    #include "LineNumEdit.hpp"
    static LineNumEdit s_hwndEdit;
#endif

BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
#ifndef LINENUMEDIT_SUPERCLASSING
    s_hwndEdit.Attach(GetDlgItem(hwnd, edt1));
    s_hwndEdit.Prepare();
    if (0)
    {
        ::SendMessage(s_hwndEdit, LNEM_SETLINEDELTA, 0, 0);
        ::SendMessage(s_hwndEdit, LNEM_SETLINENUMFORMAT, 0, (LPARAM)L"%08X");
        ::SendMessage(s_hwndEdit, LNEM_SETNUMOFDIGITS, 8, 0);
    }
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
#ifdef LINENUMEDIT_SUPERCLASSING
    LoadLibraryA("LineNumEdit");
#endif
    DialogBoxW(hInstance, MAKEINTRESOURCEW(IDD_MAIN), NULL, DialogProc);
    return 0;
}
