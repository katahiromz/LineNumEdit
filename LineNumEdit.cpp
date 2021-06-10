#include "LineNumEdit.hpp"
#include <cstring>

LineNumStatic::LineNumStatic(HWND hwnd)
    : LineNumBase(hwnd)
    , m_rgbText(::GetSysColor(COLOR_WINDOWTEXT))
    , m_rgbBack(::GetSysColor(COLOR_3DFACE))
    , m_topmargin(0)
    , m_topline(0)
    , m_bottomline(0)
    , m_linedelta(1)
{
    SHStrDup(TEXT("%d"), &m_format);
}

LRESULT CALLBACK
LineNumStatic::WindowProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_ERASEBKGND, OnEraseBkgnd);
        HANDLE_MSG(hwnd, WM_PAINT, OnPaint);
        HANDLE_MSG(hwnd, WM_LBUTTONDOWN, OnLButtonDown);
        HANDLE_MSG(hwnd, WM_LBUTTONDBLCLK, OnLButtonDown);
        HANDLE_MSG(hwnd, WM_MOUSEMOVE, OnMouseMove);
        HANDLE_MSG(hwnd, WM_DESTROY, OnDestroy);
    }
    return DefWndProc(hwnd, uMsg, wParam, lParam);
}

void LineNumStatic::OnDrawClient(HWND hwnd, HDC hDC, RECT& rcClient)
{
    INT cx = rcClient.right - rcClient.left;
    INT cy = rcClient.bottom - rcClient.top;

    HDC hdcMem = ::CreateCompatibleDC(hDC);
    HBITMAP hbm = ::CreateCompatibleBitmap(hDC, cx, cy);
    HGDIOBJ hbmOld = ::SelectObject(hdcMem, hbm);

    HWND hwndEdit = GetParent(hwnd);

    // fill background
    UINT uMsg;
    if (!::IsWindowEnabled(hwndEdit) || (GetWindowLong(hwndEdit, GWL_STYLE) & ES_READONLY))
        uMsg = WM_CTLCOLORSTATIC;
    else
        uMsg = WM_CTLCOLOREDIT;

    HBRUSH hbr = (HBRUSH)SendMessage(GetParent(hwndEdit), uMsg, (WPARAM)hDC, (LPARAM)hwndEdit);
    ::FillRect(hdcMem, &rcClient, hbr);

    // get margins
    DWORD dwMargins = (DWORD)SendMessage(hwndEdit, EM_GETMARGINS, 0, 0);
    INT leftmargin = LOWORD(dwMargins), rightmargin = HIWORD(dwMargins);

    // shrink rectangle
    rcClient.right -= leftmargin;
    cx -= leftmargin;

    // fill background
    hbr = ::CreateSolidBrush(m_rgbBack);
    ::FillRect(hdcMem, &rcClient, hbr);
    ::DeleteObject(hbr);

    // right line
    HPEN hPen = ::CreatePen(PS_SOLID, 0, m_rgbText);
    HGDIOBJ hPenOld = ::SelectObject(hdcMem, hPen);
    ::MoveToEx(hdcMem, rcClient.right - 1, rcClient.top, NULL);
    ::LineTo(hdcMem, rcClient.right - 1, rcClient.bottom);
    ::DeleteObject(::SelectObject(hdcMem, hPenOld));

    // draw text
    HFONT hFont = GetFont();
    HGDIOBJ hFontOld = ::SelectObject(hdcMem, hFont);
    if (m_bottomline)
    {
        SIZE siz;
        GetTextExtentPoint32(hdcMem, TEXT("0"), 1, &siz);
        INT cyLine = siz.cy;

        TCHAR szText[32];
        ::SetTextColor(hdcMem, m_rgbText);
        ::SetBkColor(hdcMem, m_rgbBack);
        ::SetBkMode(hdcMem, TRANSPARENT);
        for (INT iLine = m_topline; iLine < m_bottomline; ++iLine)
        {
            INT yLine = m_topmargin + cyLine * (iLine - m_topline);
            RECT rc = { 0, yLine, cx - 1, yLine + cyLine };
            StringCchPrintf(szText, _countof(szText), m_format, iLine + m_linedelta);

            if (HANDLE hProp = ::GetProp(hwnd, GetPropName(iLine)))
            {
                COLORREF rgbBack = COLORREF(reinterpret_cast<ULONG_PTR>(hProp));
                HBRUSH hbr = ::CreateSolidBrush(rgbBack);
                ::FillRect(hdcMem, &rc, hbr);
                ::DeleteObject(hbr);
            }

            rc.right -= rightmargin;
            UINT uFormat = DT_RIGHT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX;
            ::DrawText(hdcMem, szText, ::lstrlen(szText), &rc, uFormat);
        }
    }
    ::SelectObject(hdcMem, hFontOld);

    ::BitBlt(hDC, 0, 0, rcClient.right, rcClient.bottom,
             hdcMem, 0, 0, SRCCOPY);
    ::SelectObject(hdcMem, hbmOld);
}

void LineNumEdit::Prepare()
{
    assert(::IsWindow(m_hwnd));
    assert(!!(::GetWindowLong(m_hwnd, GWL_STYLE) & WS_CHILD));
    assert(!!(::GetWindowLong(m_hwnd, GWL_STYLE) & ES_MULTILINE));

    RECT rcClient;
    ::GetClientRect(m_hwnd, &rcClient);
    INT cxColumn = GetColumnWidth();
    INT cyColumn = rcClient.bottom - rcClient.top;

    // get margins
    DWORD dwMargins = DWORD(SendMessage(m_hwnd, EM_GETMARGINS, 0, 0));
    INT leftmargin = LOWORD(dwMargins), rightmargin = HIWORD(dwMargins);

    // adjust rectangle
    RECT rcEdit = rcClient;
    rcEdit.left += cxColumn;
    rcEdit.right -= rightmargin;
    Edit_SetRect(m_hwnd, &rcEdit);

    if (m_hwndStatic)
    {
        ::MoveWindow(m_hwndStatic, 0, 0, cxColumn, cyColumn, TRUE);
    }
    else
    {
        DWORD style = WS_CHILD | WS_VISIBLE | SS_NOTIFY;
        HWND hwndStatic = ::CreateWindow(TEXT("STATIC"), NULL, style,
            0, 0, cxColumn, cyColumn, m_hwnd, NULL,
            GetModuleHandle(NULL), NULL);
        m_hwndStatic.Attach(hwndStatic);
    }

    Edit_GetRect(m_hwnd, &rcEdit);
    m_hwndStatic.SetTopMargin(rcEdit.top);
    UpdateTopAndBottom();
}

LRESULT CALLBACK
LineNumEdit::WindowProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_ENABLE, OnEnable);
        HANDLE_MSG(hwnd, WM_SYSCOLORCHANGE, OnSysColorChange);
        HANDLE_MSG(hwnd, WM_VSCROLL, OnVScroll);
        HANDLE_MSG(hwnd, WM_SIZE, OnSize);
        HANDLE_MSG(hwnd, WM_SETFONT, OnSetFont);
        HANDLE_MSG(hwnd, WM_SETTEXT, OnSetText);
        HANDLE_MSG(hwnd, WM_CHAR, OnChar);
        HANDLE_MSG(hwnd, WM_KEYDOWN, OnKey);
        HANDLE_MSG(hwnd, WM_KEYUP, OnKey);
        HANDLE_MSG(hwnd, WM_MOUSEWHEEL, OnMouseWheel);
    case EM_LINESCROLL:
        return OnLineScroll(hwnd, wParam, lParam);
    case LNEM_SETLINENUMFORMAT:
        SetLineNumberFormat(reinterpret_cast<LPCTSTR>(lParam));
        return 0;
    case LNEM_SETNUMOFDIGITS:
        SetNumberOfDigits(INT(wParam));
        return 0;
    case LNEM_SETBACKCOLOR:
        m_hwndStatic.SetBackColor(COLORREF(wParam));
        return 0;
    case LNEM_SETTEXTCOLOR:
        m_hwndStatic.SetTextColor(COLORREF(wParam));
        return 0;
    case LNEM_SETLINEMARK:
        {
            LPCTSTR pszName = m_hwndStatic.GetPropName(INT(wParam));
            ::RemoveProp(m_hwndStatic, pszName);
            if (COLORREF(lParam) != CLR_INVALID && COLORREF(lParam) != CLR_NONE)
                ::SetProp(m_hwndStatic, pszName, reinterpret_cast<HANDLE>(lParam));
        }
        ::InvalidateRect(m_hwndStatic, NULL, FALSE);
        return 0;
    case LNEM_CLEARLINEMARKS:
        m_hwndStatic.DeleteProps(m_hwndStatic);
        ::InvalidateRect(m_hwndStatic, NULL, FALSE);
        return 0;
    case LNEM_SETLINEDELTA:
        m_hwndStatic.m_linedelta = INT(wParam);
        ::InvalidateRect(m_hwndStatic, NULL, FALSE);
        return 0;
    case LNEM_SETCOLUMNWIDTH:
        m_cxColumn = (INT)wParam;
        Prepare();
        return 0;
    case LNEM_GETCOLUMNWIDTH:
        return m_cxColumn;
    default:
        break;
    }
    return DefWndProc(hwnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK
LineNumEdit::SuperclassWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == WM_NCCREATE)
    {
        LineNumEdit* pCtrl = new LineNumEdit();
        pCtrl->Attach(hwnd);
        pCtrl->m_fnOldWndProc = SuperclassWindow();
    }

    LRESULT ret = LineNumBase::WindowProc(hwnd, uMsg, wParam, lParam);

    if (uMsg == WM_NCDESTROY)
    {
        LineNumBase *pBase =
            reinterpret_cast<LineNumBase *>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
        delete pBase;
    }

    return ret;
}

INT LineNumEdit::GetColumnWidth()
{
    if (m_cxColumn)
        return m_cxColumn;

    HFONT hFont = GetWindowFont(m_hwnd);

    SIZE siz;
    HDC hDC = ::GetDC(m_hwnd);
    HGDIOBJ hFontOld = ::SelectObject(hDC, hFont);
    ::GetTextExtentPoint32(hDC, TEXT("0"), 1, &siz);
    ::SelectObject(hDC, hFontOld);
    ::ReleaseDC(m_hwnd, hDC);

    // get margins
    DWORD dwMargins = DWORD(SendMessage(m_hwnd, EM_GETMARGINS, 0, 0));
    INT leftmargin = LOWORD(dwMargins), rightmargin = HIWORD(dwMargins);

    m_cxColumn = leftmargin + (m_num_digits * siz.cx) + rightmargin + leftmargin;
    return m_cxColumn;
}

void LineNumEdit::UpdateTopAndBottom()
{
    RECT rcClient;
    ::GetClientRect(m_hwnd, &rcClient);
    INT cyClient = rcClient.bottom - rcClient.top;

    INT maxline = Edit_GetLineCount(m_hwnd);

    INT lineheight = m_hwndStatic.GetLineHeight();
    if (lineheight == 0)
        return;

    INT topline = Edit_GetFirstVisibleLine(m_hwnd);
    if (topline + (cyClient / lineheight) < maxline)
        maxline = topline + ((cyClient + lineheight - 1) / lineheight);

    m_hwndStatic.SetTopAndBottom(topline, maxline);
}

WNDPROC LineNumEdit::SuperclassWindow()
{
    static WNDPROC s_fnOldWndProc = NULL;
    if (s_fnOldWndProc)
        return s_fnOldWndProc;
    WNDCLASSEX wcx = { sizeof(wcx) };
    ::GetClassInfoEx(::GetModuleHandle(NULL), TEXT("EDIT"), &wcx);
    s_fnOldWndProc = wcx.lpfnWndProc;
    wcx.lpszClassName = LineNumEdit::SuperWndClassName();
    wcx.lpfnWndProc = LineNumEdit::SuperclassWndProc;
    if (::RegisterClassEx(&wcx))
        return s_fnOldWndProc;
    return NULL;
}

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
