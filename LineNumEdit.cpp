#include "LineNumEdit.hpp"

LineNumStatic::LineNumStatic(HWND hwnd)
    : LineNumBase(hwnd)
    , m_rgbText(::GetSysColor(COLOR_WINDOWTEXT))
    , m_rgbBack(::GetSysColor(COLOR_3DFACE))
    , m_topmargin(0), m_linedelta(1)
    , m_hbm(NULL)
    , m_siz { 0, 0 }
{
    ::SHStrDup(TEXT("%d"), &m_format);
}

LineNumStatic::~LineNumStatic()
{
    ::DeleteObject(m_hbm);
    ::CoTaskMemFree(m_format);
}

LRESULT CALLBACK
LineNumStatic::WindowProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    RECT rc;
    POINT pt;
    PAINTSTRUCT ps;
    switch (uMsg)
    {
    case WM_PAINT:
        if (HDC hDC = ::BeginPaint(hwnd, &ps))
        {
            OnDrawClient(hwnd, hDC);
            ::EndPaint(hwnd, &ps);
        }
        return 0;
    case WM_LBUTTONDOWN:
    case WM_LBUTTONDBLCLK:
    case WM_MOUSEMOVE:
        ::GetClientRect(hwnd, &rc);
        pt.x = rc.right + 1;
        pt.y = GET_Y_LPARAM(lParam);
        ::MapWindowPoints(hwnd, GetEdit(), &pt, 1);
        break;
    case WM_ERASEBKGND:
        return TRUE;
    case WM_DESTROY:
        DeleteProps(hwnd);
        break;
    }
    return DefWndProc(hwnd, uMsg, wParam, lParam);
}

static INT getLogicalLineIndexFromCharIndex(LPCTSTR psz, INT ich)
{
    INT ich0 = 0, iLine = 0;
    while (*psz && ich0 < ich)
    {
        if (*psz == TEXT('\n'))
            ++iLine;
        ++psz;
        ++ich0;
    }
    return iLine;
}

void LineNumStatic::OnDrawClient(HWND hwnd, HDC hDC)
{
    RECT rcClient;
    ::GetClientRect(hwnd, &rcClient);

    // prepare for double buffering
    SIZE siz = { rcClient.right - rcClient.left, rcClient.bottom - rcClient.top };
    if (!siz.cx || !siz.cy)
        return;

    HDC hdcMem = ::CreateCompatibleDC(hDC);
    HBITMAP hbm;
    if (m_hbm && siz.cx <= m_siz.cx && siz.cy <= m_siz.cy)
    {
        hbm = m_hbm;
    }
    else
    {
        hbm = ::CreateCompatibleBitmap(hDC, siz.cx, siz.cy);
        m_siz = siz;
    }
    HGDIOBJ hbmOld = ::SelectObject(hdcMem, hbm);

    // fill background
    UINT uMsg;
    HWND hwndEdit = GetEdit();
    if (!::IsWindowEnabled(hwndEdit) || (GetWindowLong(hwndEdit, GWL_STYLE) & ES_READONLY))
        uMsg = WM_CTLCOLORSTATIC;
    else
        uMsg = WM_CTLCOLOREDIT;
    HBRUSH hbr = reinterpret_cast<HBRUSH>(
        ::SendMessage(GetParent(hwndEdit), uMsg,
                      reinterpret_cast<WPARAM>(hDC), reinterpret_cast<LPARAM>(hwndEdit)));
    ::FillRect(hdcMem, &rcClient, hbr);

    // get margins
    DWORD dwMargins = DWORD(::SendMessage(hwndEdit, EM_GETMARGINS, 0, 0));
    INT leftmargin = LOWORD(dwMargins), rightmargin = HIWORD(dwMargins);

    // shrink rectangle
    rcClient.right -= leftmargin;
    siz.cx -= leftmargin;

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
    HFONT hFont = GetWindowFont(hwndEdit);
    HGDIOBJ hFontOld = ::SelectObject(hdcMem, hFont);
    ::SetBkMode(hdcMem, TRANSPARENT);
    WCHAR szText[32];
    INT yLine = m_topmargin, cyLine = GetLineHeight();
    {
        // get the edit text
        INT cch = Edit_GetTextLength(hwndEdit);
        BSTR bstrText = ::SysAllocStringLen(NULL, cch);
        if (bstrText)
        {
            Edit_GetText(hwndEdit, bstrText, cch + 1);

            // initialize variables for lines loop
            INT iPhysicalLine = Edit_GetFirstVisibleLine(hwndEdit);
            INT ich = Edit_LineIndex(hwndEdit, iPhysicalLine);
            if (ich == -1)
                ich = cch;
            INT ichOld = ich;
            INT cLogicalLines = ::getLogicalLineIndexFromCharIndex(bstrText, MAXLONG);
            INT iLogicalLine = ::getLogicalLineIndexFromCharIndex(bstrText, ich);

            INT iOldLogicalLine;
            if (ich == 0)
                iOldLogicalLine = -1;
            else if (ich > 0 && bstrText[ich - 1] == L'\n')
                iOldLogicalLine = iLogicalLine - 1;
            else
                iOldLogicalLine = iLogicalLine;

            // for each physical lines
            do
            {
                RECT rc = { 0, yLine, siz.cx - 1, yLine + cyLine }; // one line
                INT nLabel = iLogicalLine + m_linedelta; // label

                // fill the background if necessary, and set text color
                HANDLE hProp = ::GetProp(hwnd, GetPropName(nLabel));
                if (hProp && (ich < cch || iOldLogicalLine < iLogicalLine || iLogicalLine < cLogicalLines))
                {
                    COLORREF rgbBack = (COLORREF(reinterpret_cast<ULONG_PTR>(hProp)) & 0xFFFFFF);
                    HBRUSH hbr = ::CreateSolidBrush(rgbBack);
                    ::FillRect(hdcMem, &rc, hbr);
                    ::DeleteObject(hbr);
                    INT value = (GetRValue(rgbBack) + GetGValue(rgbBack) + GetBValue(rgbBack)) / 3;
                    if (value < 255 / 3)
                        ::SetTextColor(hdcMem, RGB(255, 255, 255));
                    else
                        ::SetTextColor(hdcMem, RGB(0, 0, 0));
                }
                else
                {
                    ::SetTextColor(hdcMem, m_rgbText);
                }

                // draw text
                if (ich <= cch && iOldLogicalLine != iLogicalLine)
                {
                    StringCchPrintfW(szText, _countof(szText), m_format, nLabel);
                    rc.right -= rightmargin;
                    UINT uFormat = DT_RIGHT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX;
                    ::DrawTextW(hdcMem, szText, ::lstrlenW(szText), &rc, uFormat);
                }

                // go to next line
                yLine += cyLine;
                ++iPhysicalLine;

                // go to next newline
                ichOld = ich;
                while (bstrText[ich])
                {
                    if (Edit_LineFromChar(hwndEdit, ich) == iPhysicalLine)
                        break;
                    if (bstrText[ich] == L'\n')
                    {
                        ++ich;
                        break;
                    }
                    ++ich;
                }

                // update logical line if necessary
                iOldLogicalLine = iLogicalLine;
                iLogicalLine = ::getLogicalLineIndexFromCharIndex(bstrText, ich);
                if (iLogicalLine == iOldLogicalLine && ich == ichOld)
                    break;
            } while (yLine < rcClient.bottom); // beyond the client area?

            // clean up
            ::SysFreeString(bstrText);
        }
    }
    ::SelectObject(hdcMem, hFontOld);

    // send the image to the window
    ::BitBlt(hDC, 0, 0, rcClient.right, rcClient.bottom, hdcMem, 0, 0, SRCCOPY);
    ::SelectObject(hdcMem, hbmOld);

    // clean up
    ::DeleteDC(hdcMem);

    // do cache
    if (m_hbm != hbm)
    {
        DeleteObject(m_hbm);
        m_hbm = hbm;
    }
}

void LineNumEdit::Prepare()
{
    // sanity check
    assert(::IsWindow(m_hwnd));
    assert(!!(::GetWindowLong(m_hwnd, GWL_STYLE) & WS_CHILD));
    assert(!!(::GetWindowLong(m_hwnd, GWL_STYLE) & ES_MULTILINE));

    RECT rcClient;
    ::GetClientRect(m_hwnd, &rcClient);
    INT cxColumn = GetColumnWidth(), cyColumn = rcClient.bottom - rcClient.top;

    // get margins
    DWORD dwMargins = DWORD(::SendMessage(m_hwnd, EM_GETMARGINS, 0, 0));
    INT leftmargin = LOWORD(dwMargins), rightmargin = HIWORD(dwMargins);

    // adjust rectangle
    RECT rcEdit = rcClient;
    rcEdit.left += cxColumn;
    rcEdit.right -= rightmargin;
    Edit_SetRectNoPaint(m_hwnd, &rcEdit);

    if (::IsWindow(m_hwndStatic))
    {
        ::MoveWindow(m_hwndStatic, 0, 0, cxColumn, cyColumn, TRUE);
    }
    else
    {
        DWORD style = WS_CHILD | WS_VISIBLE | SS_NOTIFY;
        HWND hwndStatic = ::CreateWindow(TEXT("STATIC"), NULL, style,
                                         0, 0, cxColumn, cyColumn, m_hwnd,
                                         NULL, ::GetModuleHandle(NULL), NULL);
        m_hwndStatic.Attach(hwndStatic);
    }

    Edit_GetRect(m_hwnd, &rcEdit);
    m_hwndStatic.m_topmargin = rcEdit.top;
    m_hwndStatic.Redraw();
}

LRESULT CALLBACK
LineNumEdit::WindowProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT ret;
    switch (uMsg)
    {
    case WM_ENABLE: case WM_SYSCOLORCHANGE: case EM_SETREADONLY:
        ret = DefWndProc(hwnd, uMsg, wParam, lParam);
        RefreshColors();
        return ret;
    case LNEM_SETLINENUMFORMAT:
        SetLineNumberFormat(reinterpret_cast<LPCTSTR>(lParam));
        return 0;
    case LNEM_SETNUMOFDIGITS:
        SetNumberOfDigits(INT(wParam));
        return 0;
    case LNEM_SETLINEMARK:
        {
            LPCTSTR pszName = m_hwndStatic.GetPropName(INT(wParam));
            ::RemoveProp(m_hwndStatic, pszName);
            COLORREF rgb = COLORREF(lParam);
            if (rgb != CLR_INVALID && rgb != CLR_NONE)
            {
                lParam |= 0xFF000000;
                ::SetProp(m_hwndStatic, pszName, reinterpret_cast<HANDLE>(lParam));
            }
        }
        m_hwndStatic.Redraw();
        return 0;
    case LNEM_CLEARLINEMARKS:
        m_hwndStatic.DeleteProps(m_hwndStatic);
        return 0;
    case LNEM_SETLINEDELTA:
        m_hwndStatic.m_linedelta = INT(wParam);
        m_hwndStatic.Redraw();
        return 0;
    case LNEM_SETCOLUMNWIDTH:
        m_cxColumn = INT(wParam);
        Prepare();
        return 0;
    case LNEM_GETCOLUMNWIDTH:
        return m_cxColumn;
    case LNEM_GETLINEMARK:
        {
            LPCTSTR pszName = m_hwndStatic.GetPropName(INT(wParam));
            HANDLE hData = ::GetProp(m_hwndStatic, pszName);
            if (hData == NULL)
                return CLR_INVALID;
            COLORREF rgb = COLORREF(reinterpret_cast<ULONG_PTR>(hData));
            rgb &= 0x00FFFFFF;
            return rgb;
        }
    case WM_SETTEXT: case WM_CHAR: case WM_KEYDOWN: case WM_KEYUP: case WM_VSCROLL:
    case WM_CUT: case WM_PASTE: case WM_UNDO: case WM_MOUSEWHEEL:
    case EM_UNDO: case EM_SCROLL: case EM_SCROLLCARET: case EM_LINESCROLL:
    case EM_REPLACESEL: case EM_SETHANDLE: case EM_SETMARGINS:
        ret = DefWndProc(hwnd, uMsg, wParam, lParam);
        m_hwndStatic.Redraw();
        return ret;
    case WM_SIZE: case WM_SETFONT:
        ret = DefWndProc(hwnd, uMsg, wParam, lParam);
        Prepare();
        return ret;
    }
    return DefWndProc(hwnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK
LineNumEdit::SuperclassWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == WM_NCCREATE)
    {
        LineNumEdit* pCtrl = new LineNumEdit(hwnd);
        pCtrl->m_fnOldWndProc = SuperclassWindow();
    }

    LineNumBase *pBase =
        reinterpret_cast<LineNumBase *>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

    LRESULT ret = LineNumBase::WindowProc(hwnd, uMsg, wParam, lParam);
    if (uMsg == WM_NCDESTROY)
        delete pBase;
    return ret;
}

INT LineNumEdit::GetColumnWidth()
{
    if (m_cxColumn)
        return m_cxColumn; // cached

    // get text extent
    SIZE siz;
    HDC hDC = ::GetDC(m_hwnd);
    HGDIOBJ hFontOld = ::SelectObject(hDC, GetWindowFont(m_hwnd));
    ::GetTextExtentPoint32(hDC, TEXT("0"), 1, &siz);
    ::SelectObject(hDC, hFontOld);
    ::ReleaseDC(m_hwnd, hDC);

    // get margins
    DWORD dwMargins = DWORD(::SendMessage(m_hwnd, EM_GETMARGINS, 0, 0));
    INT leftmargin = LOWORD(dwMargins), rightmargin = HIWORD(dwMargins);

    // save and return
    m_cxColumn = leftmargin + (m_num_digits * siz.cx) + rightmargin + leftmargin;
    return m_cxColumn;
}

WNDPROC LineNumEdit::SuperclassWindow()
{
    static WNDPROC s_fnOldWndProc = NULL;
    if (s_fnOldWndProc)
        return s_fnOldWndProc;

    WNDCLASSEX wcx = { sizeof(wcx) };
    if (!::GetClassInfoEx(::GetModuleHandle(NULL), TEXT("EDIT"), &wcx))
        return NULL;

    s_fnOldWndProc = wcx.lpfnWndProc;
    wcx.lpszClassName = LineNumEdit::SuperWndClassName();
    wcx.lpfnWndProc = LineNumEdit::SuperclassWndProc;
    if (::RegisterClassEx(&wcx))
        return s_fnOldWndProc;
    return NULL;
}

#ifdef LINENUMEDIT_DLL
    BOOL WINAPI
    DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
    {
        if (fdwReason == DLL_PROCESS_ATTACH)
            return (LineNumEdit::SuperclassWindow() != NULL);
        return TRUE;
    }
#endif
