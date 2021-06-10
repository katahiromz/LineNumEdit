#ifndef _INC_WINDOWS
    #include <windows.h>
#endif
#ifndef _INC_WINDOWSX
    #include <windowsx.h>
#endif
#ifndef _INC_COMMCTRL
    #include <commctrl.h>
#endif
#include <string>
#include <strsafe.h>
#include <cassert>

class LineNumBase
{
public:
    HWND m_hwnd;
    operator HWND() const
    {
        return m_hwnd;
    }

    LineNumBase(HWND hwnd = NULL) : m_hwnd(NULL), m_fnOldWndProc(NULL)
    {
        if (hwnd)
            Attach(hwnd);
    }

    virtual ~LineNumBase()
    {
    }

    virtual LRESULT CALLBACK
    WindowProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) = 0;

    static LRESULT CALLBACK
    WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        LineNumBase *pBase =
            reinterpret_cast<LineNumBase *>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

        if (!pBase)
            return ::DefWindowProcW(hwnd, uMsg, wParam, lParam);

        return pBase->WindowProcDx(hwnd, uMsg, wParam, lParam);
    }

    LRESULT CALLBACK
    DefWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        if (m_fnOldWndProc)
            return ::CallWindowProcW(m_fnOldWndProc, hwnd, uMsg, wParam, lParam);
        return ::DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }

    BOOL Attach(HWND hwnd)
    {
        assert(m_hwnd == NULL);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
        m_fnOldWndProc = SubclassWindow(hwnd, LineNumBase::WindowProc);
        m_hwnd = hwnd;
        return TRUE;
    }

    HWND Detach()
    {
        SetWindowLongPtrW(m_hwnd, GWLP_USERDATA, 0);
        SubclassWindow(m_hwnd, m_fnOldWndProc);
        m_fnOldWndProc = NULL;
        HWND hwnd = m_hwnd;
        m_hwnd = NULL;
        return hwnd;
    }

protected:
    WNDPROC m_fnOldWndProc;
};

class LineNumStatic : public LineNumBase
{
public:
    LineNumStatic(HWND hwnd = NULL) : LineNumBase(hwnd)
    {
        m_rgbFore = ::GetSysColor(COLOR_WINDOWTEXT);
        m_rgbBack = ::GetSysColor(COLOR_3DFACE);
        m_topmargin = 0;
        m_topline = 0;
        m_bottomline = 0;
        m_format = L"%d";
    }

    LRESULT CALLBACK
    WindowProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override
    {
        switch (uMsg)
        {
            HANDLE_MSG(hwnd, WM_ERASEBKGND, OnEraseBkgnd);
            HANDLE_MSG(hwnd, WM_PAINT, OnPaint);
            HANDLE_MSG(hwnd, WM_LBUTTONDOWN, OnLButtonDown);
            HANDLE_MSG(hwnd, WM_LBUTTONDBLCLK, OnLButtonDown);
            HANDLE_MSG(hwnd, WM_MOUSEMOVE, OnMouseMove);
        }
        return DefWndProc(hwnd, uMsg, wParam, lParam);
    }

    void SetForeColor(COLORREF rgb, BOOL redraw = TRUE)
    {
        m_rgbFore = rgb;
        if (redraw)
            ::InvalidateRect(m_hwnd, NULL, TRUE);
    }

    void SetBackColor(COLORREF rgb, BOOL redraw = TRUE)
    {
        m_rgbBack = rgb;
        if (redraw)
            ::InvalidateRect(m_hwnd, NULL, TRUE);
    }

    void SetTopAndBottom(INT topline, INT bottomline)
    {
        m_topline = topline;
        m_bottomline = bottomline;
        ::InvalidateRect(m_hwnd, NULL, TRUE);
    }

    void SetTopMargin(INT topmargin)
    {
        m_topmargin = topmargin;
    }

    void SetLineNumberFormat(LPCWSTR format)
    {
        m_format = format;
        ::InvalidateRect(m_hwnd, NULL, TRUE);
    }

protected:
    COLORREF m_rgbFore;
    COLORREF m_rgbBack;
    INT m_topmargin;
    INT m_topline;
    INT m_bottomline;
    std::wstring m_format;

    HFONT GetFont() const
    {
        return (HFONT)GetWindowFont(::GetParent(m_hwnd));
    }

    INT GetLineHeight() const
    {
        HDC hDC = ::GetDC(m_hwnd);
        HGDIOBJ hFontOld = SelectObject(hDC, GetFont());
        TEXTMETRICW tm;
        ::GetTextMetricsW(hDC, &tm);
        SelectObject(hDC, hFontOld);
        ::ReleaseDC(m_hwnd, hDC);
        return tm.tmAveCharWidth;
    }

    BOOL OnEraseBkgnd(HWND hwnd, HDC hdc)
    {
        return TRUE;
    }

    void OnDrawClient(HWND hwnd, HDC hDC, RECT& rcClient)
    {
        INT cx = rcClient.right - rcClient.left;
        INT cy = rcClient.bottom - rcClient.top;

        HDC hdcMem = ::CreateCompatibleDC(hDC);
        HBITMAP hbm = ::CreateCompatibleBitmap(hDC, cx, cy);
        HGDIOBJ hbmOld = ::SelectObject(hdcMem, hbm);

        // fill background
        ::FillRect(hdcMem, &rcClient, (HBRUSH)(COLOR_WINDOW + 1));

        // get margins
        HWND hwndEdit = GetParent(hwnd);
        DWORD dwMargins = SendMessageW(hwndEdit, EM_GETMARGINS, 0, 0);
        INT leftmargin = LOWORD(dwMargins);

        // shrink rectangle
        rcClient.right -= leftmargin;
        cx -= leftmargin;

        // fill background
        HBRUSH hbr = ::CreateSolidBrush(m_rgbBack);
        ::FillRect(hdcMem, &rcClient, hbr);
        ::DeleteObject(hbr);

        // right line
        HPEN hPen = ::CreatePen(PS_SOLID, 0, m_rgbFore);
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
            GetTextExtentPoint32W(hdcMem, L"0", 1, &siz);
            INT cyLine = siz.cy;

            WCHAR szText[32];
            ::SetTextColor(hdcMem, m_rgbFore);
            ::SetBkColor(hdcMem, m_rgbBack);
            ::SetBkMode(hdcMem, TRANSPARENT);
            for (INT iLine = m_topline; iLine <= m_bottomline; ++iLine)
            {
                StringCchPrintfW(szText, _countof(szText), m_format.c_str(), iLine);
                INT yLine = m_topmargin + cyLine * (iLine - m_topline);
                RECT rc = { 0, yLine, cx - 2, yLine + cyLine };
                UINT uFormat = DT_RIGHT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX;
                ::DrawTextW(hdcMem, szText, ::lstrlenW(szText), &rc, uFormat);
            }
        }
        ::SelectObject(hdcMem, hFontOld);

        ::BitBlt(hDC, 0, 0, rcClient.right, rcClient.bottom,
                 hdcMem, 0, 0, SRCCOPY);
        ::SelectObject(hdcMem, hbmOld);
    }

    void OnPaint(HWND hwnd)
    {
        PAINTSTRUCT ps;
        if (HDC hDC = ::BeginPaint(hwnd, &ps))
        {
            RECT rcClient;
            ::GetClientRect(hwnd, &rcClient);

            OnDrawClient(hwnd, hDC, rcClient);

            ::EndPaint(hwnd, &ps);
        }
    }

    void OnLButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags)
    {
        RECT rc;
        GetClientRect(hwnd, &rc);
        POINT pt = { rc.right + 1, y };
        HWND hwndEdit = GetParent(hwnd);
        MapWindowPoints(hwnd, hwndEdit, &pt, 1);
        FORWARD_WM_LBUTTONDOWN(hwndEdit, fDoubleClick, pt.x, pt.y, keyFlags, SendMessageW);
    }

    void OnMouseMove(HWND hwnd, int x, int y, UINT keyFlags)
    {
        RECT rc;
        GetClientRect(hwnd, &rc);
        POINT pt = { rc.right + 1, y };
        HWND hwndEdit = GetParent(hwnd);
        MapWindowPoints(hwnd, hwndEdit, &pt, 1);
        FORWARD_WM_MOUSEMOVE(hwndEdit, pt.x, pt.y, keyFlags, SendMessageW);
    }
};

class LineNumEdit : public LineNumBase
{
public:
    LineNumEdit(HWND hwnd = NULL) : m_linedelta(1), m_num_digits(6)
    {
        if (hwnd)
            Attach(hwnd);
    }

    static LPCWSTR SuperWndClassName()
    {
        return L"LineNumEdit";
    }

    void SetLineDelta(INT delta)
    {
        m_linedelta = delta;
    }

    void Prepare()
    {
        RECT rcClient;
        ::GetClientRect(m_hwnd, &rcClient);
        INT cxColumn = GetLineNumberColumnWidth();
        INT cyColumn = rcClient.bottom - rcClient.top;

        // get margins
        DWORD dwMargins = SendMessageW(m_hwnd, EM_GETMARGINS, 0, 0);
        INT leftmargin = LOWORD(dwMargins), rightmargin = HIWORD(dwMargins);

        // get client area
        RECT rcEdit;
        GetClientRect(m_hwnd, &rcEdit);

        // adjust rectangle
        rcEdit.left += leftmargin;
        rcEdit.right -= rightmargin;
        rcEdit.left += GetLineNumberColumnWidth();
        Edit_SetRect(m_hwnd, &rcEdit);

        if (m_hwndStatic)
        {
            ::MoveWindow(m_hwndStatic, 0, 0, cxColumn, cyColumn, TRUE);
        }
        else
        {
            DWORD style = WS_CHILD | WS_VISIBLE | SS_NOTIFY;
            HWND hwndStatic = ::CreateWindowW(L"STATIC", NULL, style,
                0, 0, cxColumn, cyColumn, m_hwnd, NULL,
                GetModuleHandleW(NULL), NULL);
            m_hwndStatic.Attach(hwndStatic);
        }

        Edit_GetRect(m_hwnd, &rcEdit);
        m_hwndStatic.SetTopMargin(rcEdit.top);
        UpdateTopAndBottom();
    }

    void SetWindowColor(BOOL fEnable)
    {
        if (fEnable)
            m_hwndStatic.SetForeColor(::GetSysColor(COLOR_WINDOWTEXT));
        else
            m_hwndStatic.SetForeColor(::GetSysColor(COLOR_GRAYTEXT));

        m_hwndStatic.SetBackColor(::GetSysColor(COLOR_3DFACE));
    }

    void SetLineNumberFormat(LPCWSTR format = L"%d")
    {
        m_hwndStatic.SetLineNumberFormat(format);
        ::InvalidateRect(m_hwndStatic, NULL, TRUE);
    }

    LRESULT CALLBACK
    WindowProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override
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
        default:
            break;
        }
        return DefWndProc(hwnd, uMsg, wParam, lParam);
    }

    static LRESULT CALLBACK
    SuperclassWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
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
                reinterpret_cast<LineNumBase *>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
            delete pBase;
        }

        return ret;
    }

    static WNDPROC SuperclassWindow()
    {
        static WNDPROC s_fnOldWndProc = NULL;
        if (s_fnOldWndProc)
            return s_fnOldWndProc;
        WNDCLASSEXW wcx = { sizeof(wcx) };
        ::GetClassInfoExW(::GetModuleHandleW(NULL), L"EDIT", &wcx);
        s_fnOldWndProc = wcx.lpfnWndProc;
        wcx.lpszClassName = LineNumEdit::SuperWndClassName();
        wcx.lpfnWndProc = LineNumEdit::SuperclassWndProc;
        if (::RegisterClassExW(&wcx))
            return s_fnOldWndProc;
        return NULL;
    }

    void SetNumberOfDigits(INT num = 6)
    {
        m_num_digits = num;
        Prepare();
    }

protected:
    INT m_linedelta;
    INT m_num_digits;
    LineNumStatic m_hwndStatic;

    void OnEnable(HWND hwnd, BOOL fEnable)
    {
        SetWindowColor(fEnable);
    }

    void OnSysColorChange(HWND hwnd)
    {
        DefWndProc(hwnd, WM_SYSCOLORCHANGE, 0, 0);
        SetWindowColor(::IsWindowEnabled(hwnd));
    }

    void OnVScroll(HWND hwnd, HWND hwndCtl, UINT code, int pos)
    {
        FORWARD_WM_VSCROLL(hwnd, hwndCtl, code, pos, DefWndProc);
        UpdateTopAndBottom();
    }

    void OnSize(HWND hwnd, UINT state, int cx, int cy)
    {
        FORWARD_WM_SIZE(hwnd, state, cx, cy, DefWndProc);
        Prepare();
    }

    void OnSetFont(HWND hwnd, HFONT hfont, BOOL fRedraw)
    {
        FORWARD_WM_SETFONT(hwnd, hfont, fRedraw, DefWndProc);
        Prepare();
    }

    void OnSetText(HWND hwnd, LPCTSTR lpszText)
    {
        FORWARD_WM_SETTEXT(hwnd, lpszText, DefWndProc);
        UpdateTopAndBottom();
    }

    LRESULT OnLineScroll(HWND hwnd, WPARAM wParam, LPARAM lParam)
    {
        LRESULT ret = DefWndProc(hwnd, EM_LINESCROLL, wParam, lParam);
        UpdateTopAndBottom();
        return ret;
    }

    void OnChar(HWND hwnd, TCHAR ch, int cRepeat)
    {
        FORWARD_WM_CHAR(hwnd, ch, cRepeat, DefWndProc);
        UpdateTopAndBottom();
        InvalidateRect(m_hwndStatic, NULL, FALSE);
    }

    void OnKey(HWND hwnd, UINT vk, BOOL fDown, int cRepeat, UINT flags)
    {
        if (fDown)
            FORWARD_WM_KEYDOWN(hwnd, vk, cRepeat, flags, DefWndProc);
        else
            FORWARD_WM_KEYUP(hwnd, vk, cRepeat, flags, DefWndProc);
        UpdateTopAndBottom();
        InvalidateRect(m_hwndStatic, NULL, FALSE);
    }

#undef FORWARD_WM_MOUSEWHEEL
#define FORWARD_WM_MOUSEWHEEL(hwnd, xPos, yPos, zDelta, fwKeys, fn) \
    (void)(fn)((hwnd), WM_MOUSEWHEEL, MAKEWPARAM((fwKeys),(zDelta)), MAKELPARAM((xPos),(yPos)))

    void OnMouseWheel(HWND hwnd, int xPos, int yPos, int zDelta, UINT fwKeys)
    {
        FORWARD_WM_MOUSEWHEEL(hwnd, xPos, yPos, zDelta, fwKeys, DefWndProc);
        UpdateTopAndBottom();
        InvalidateRect(m_hwndStatic, NULL, FALSE);
    }

    INT GetLineNumberColumnWidth()
    {
        HFONT hFont = GetWindowFont(m_hwnd);

        TEXTMETRICW tm;
        HDC hDC = ::GetDC(m_hwnd);
        HGDIOBJ hFontOld = ::SelectObject(hDC, hFont);
        ::GetTextMetricsW(hDC, &tm);
        ::SelectObject(hDC, hFontOld);
        ::ReleaseDC(m_hwnd, hDC);

        DWORD dwMargins = SendMessageW(m_hwnd, EM_GETMARGINS, 0, 0);
        INT leftmargin = LOWORD(dwMargins);

        return m_num_digits * tm.tmAveCharWidth + 1 + leftmargin;
    }

    INT GetLineNumberLineHeight()
    {
        HFONT hFont = GetWindowFont(m_hwnd);

        TEXTMETRICW tm;
        HDC hDC = ::GetDC(m_hwnd);
        HGDIOBJ hFontOld = ::SelectObject(hDC, hFont);
        ::GetTextMetricsW(hDC, &tm);
        ::SelectObject(hDC, hFontOld);
        ::ReleaseDC(m_hwnd, hDC);

        return tm.tmHeight;
    }

    void UpdateTopAndBottom()
    {
        RECT rcClient;
        ::GetClientRect(m_hwnd, &rcClient);
        INT cyClient = rcClient.bottom - rcClient.top;

        INT maxline = Edit_GetLineCount(m_hwnd);

        INT lineheight = GetLineNumberLineHeight();
        if (lineheight == 0)
            return;

        INT topline = Edit_GetFirstVisibleLine(m_hwnd) + m_linedelta;
        if (topline + (cyClient / lineheight) < maxline)
            maxline = topline + (cyClient / lineheight);

        m_hwndStatic.SetTopAndBottom(topline, maxline);
    }
};
