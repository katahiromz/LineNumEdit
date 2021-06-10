#ifndef _INC_WINDOWS
    #include <windows.h>
#endif
#ifndef _INC_WINDOWSX
    #include <windowsx.h>
#endif
#ifndef _INC_COMMCTRL
    #include <commctrl.h>
#endif
#ifndef _INC_SHLWAPI
    #include <shlwapi.h>
#endif

// messages for LineNumEdit
#define LNEM_SETLINENUMFORMAT (WM_USER + 100)
#define LNEM_SETNUMOFDIGITS (WM_USER + 101)
#define LNEM_SETLINEMARK (WM_USER + 102)
#define LNEM_CLEARLINEMARKS (WM_USER + 103)
#define LNEM_SETLINEDELTA (WM_USER + 104)
#define LNEM_SETCOLUMNWIDTH (WM_USER + 105)
#define LNEM_GETCOLUMNWIDTH (WM_USER + 106)

#ifdef LINENUMEDIT_IMPL

#include <strsafe.h>
#include <cassert>

class LineNumBase
{
public:
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
            reinterpret_cast<LineNumBase *>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

        if (pBase)
            return pBase->WindowProcDx(hwnd, uMsg, wParam, lParam);

        return ::DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    LRESULT CALLBACK
    DefWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        if (m_fnOldWndProc)
            return ::CallWindowProc(m_fnOldWndProc, hwnd, uMsg, wParam, lParam);
        return ::DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    BOOL Attach(HWND hwnd)
    {
        assert(m_hwnd == NULL);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
        m_fnOldWndProc = SubclassWindow(hwnd, LineNumBase::WindowProc);
        m_hwnd = hwnd;
        return TRUE;
    }

    HWND Detach()
    {
        SetWindowLongPtr(m_hwnd, GWLP_USERDATA, 0);
        SubclassWindow(m_hwnd, m_fnOldWndProc);
        m_fnOldWndProc = NULL;
        HWND hwnd = m_hwnd;
        m_hwnd = NULL;
        return hwnd;
    }

    void Redraw(BOOL bErase = FALSE)
    {
        ::InvalidateRect(m_hwnd, NULL, bErase);
    }

protected:
    HWND m_hwnd;
    WNDPROC m_fnOldWndProc;
};

class LineNumStatic : public LineNumBase
{
public:
    LineNumStatic(HWND hwnd = NULL);

    ~LineNumStatic()
    {
        CoTaskMemFree(m_format);
    }

    LRESULT CALLBACK
    WindowProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

    void SetColors(INT text, INT back, BOOL redraw = TRUE)
    {
        m_rgbText = ::GetSysColor(text);
        m_rgbBack = ::GetSysColor(back);
        if (redraw)
            Redraw();
    }

    void SetTopAndBottom(INT topline, INT bottomline)
    {
        m_topline = topline;
        m_bottomline = bottomline;
        Redraw();
    }

    void SetTopMargin(INT topmargin)
    {
        m_topmargin = topmargin;
    }

    void SetLineNumberFormat(LPCTSTR format)
    {
        CoTaskMemFree(m_format);
        SHStrDup(format, &m_format);
        Redraw();
    }

protected:
    COLORREF m_rgbText;
    COLORREF m_rgbBack;
    INT m_topmargin;
    INT m_topline;
    INT m_bottomline;
    INT m_linedelta;
    LPTSTR m_format;

    HWND GetEdit() const
    {
        return ::GetParent(m_hwnd);
    }

    HFONT GetFont() const
    {
        return (HFONT)GetWindowFont(GetEdit());
    }

    INT GetLineHeight() const
    {
        HDC hDC = ::GetDC(m_hwnd);
        HGDIOBJ hFontOld = SelectObject(hDC, GetFont());
        TEXTMETRIC tm;
        ::GetTextMetrics(hDC, &tm);
        SelectObject(hDC, hFontOld);
        ::ReleaseDC(m_hwnd, hDC);
        return tm.tmAveCharWidth;
    }

    LPCTSTR GetPropName(INT iLine) const
    {
        static TCHAR s_szProp[64];
        StringCchPrintf(s_szProp, _countof(s_szProp), TEXT("LineNumStatic-%d"), iLine);
        return s_szProp;
    }

    static BOOL CALLBACK
    PropEnumProc(HWND hwnd, LPCTSTR lpszString, HANDLE hData)
    {
        if (StrCmpNI(lpszString, TEXT("LineNumStatic-"), 14) == 0)
            ::RemoveProp(hwnd, lpszString);
        return TRUE;
    }

    void DeleteProps(HWND hwnd)
    {
        ::EnumProps(hwnd, PropEnumProc);
    }

    void OnDestroy(HWND hwnd)
    {
        DeleteProps(hwnd);
    }

    BOOL OnEraseBkgnd(HWND hwnd, HDC hdc)
    {
        return TRUE;
    }

    void OnDrawClient(HWND hwnd, HDC hDC, RECT& rcClient);

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
        HWND hwndEdit = GetEdit();
        MapWindowPoints(hwnd, hwndEdit, &pt, 1);
        FORWARD_WM_LBUTTONDOWN(hwndEdit, fDoubleClick, pt.x, pt.y, keyFlags, SendMessage);
    }

    void OnMouseMove(HWND hwnd, int x, int y, UINT keyFlags)
    {
        RECT rc;
        GetClientRect(hwnd, &rc);
        POINT pt = { rc.right + 1, y };
        HWND hwndEdit = GetEdit();
        MapWindowPoints(hwnd, hwndEdit, &pt, 1);
        FORWARD_WM_MOUSEMOVE(hwndEdit, pt.x, pt.y, keyFlags, SendMessage);
    }

    friend class LineNumEdit;
};

class LineNumEdit : public LineNumBase
{
public:
    LineNumEdit(HWND hwnd = NULL)
        : LineNumBase(hwnd)
        , m_num_digits(6)
        , m_cxColumn(0)
    {
    }

    static LPCTSTR SuperWndClassName()
    {
        return TEXT("LineNumEdit");
    }

    void Prepare();

    void RefreshColors()
    {
        if (::IsWindowEnabled(m_hwnd) && !(GetWindowLong(m_hwnd, GWL_STYLE) & ES_READONLY))
            m_hwndStatic.SetColors(COLOR_WINDOWTEXT, COLOR_3DFACE);
        else
            m_hwndStatic.SetColors(COLOR_GRAYTEXT, COLOR_3DFACE);
    }

    void SetLineNumberFormat(LPCTSTR format)
    {
        if (!format)
            format = TEXT("%d");
        m_hwndStatic.SetLineNumberFormat(format);
        m_hwndStatic.Redraw();
    }

    LRESULT CALLBACK
    WindowProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

    static LRESULT CALLBACK
    SuperclassWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    static WNDPROC SuperclassWindow();

    void SetNumberOfDigits(INT num = 6)
    {
        m_cxColumn = 0;
        m_num_digits = num;
        Prepare();
    }

protected:
    INT m_num_digits;
    INT m_cxColumn;
    LineNumStatic m_hwndStatic;

    void OnEnable(HWND hwnd, BOOL fEnable)
    {
        FORWARD_WM_ENABLE(hwnd, fEnable, DefWndProc);
        RefreshColors();
    }

    void OnSysColorChange(HWND hwnd)
    {
        FORWARD_WM_SYSCOLORCHANGE(hwnd, DefWndProc);
        RefreshColors();
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
        m_hwndStatic.Redraw();
    }

    void OnKey(HWND hwnd, UINT vk, BOOL fDown, int cRepeat, UINT flags)
    {
        if (fDown)
            FORWARD_WM_KEYDOWN(hwnd, vk, cRepeat, flags, DefWndProc);
        else
            FORWARD_WM_KEYUP(hwnd, vk, cRepeat, flags, DefWndProc);
        UpdateTopAndBottom();
        m_hwndStatic.Redraw();
    }

#undef FORWARD_WM_MOUSEWHEEL
#define FORWARD_WM_MOUSEWHEEL(hwnd, xPos, yPos, zDelta, fwKeys, fn) \
    (void)(fn)((hwnd), WM_MOUSEWHEEL, MAKEWPARAM((fwKeys),(zDelta)), MAKELPARAM((xPos),(yPos)))

    void OnMouseWheel(HWND hwnd, int xPos, int yPos, int zDelta, UINT fwKeys)
    {
        FORWARD_WM_MOUSEWHEEL(hwnd, xPos, yPos, zDelta, fwKeys, DefWndProc);
        UpdateTopAndBottom();
        m_hwndStatic.Redraw();
    }

    INT GetColumnWidth();
    void UpdateTopAndBottom();
};

#endif  // def LINENUMEDIT_IMPL
