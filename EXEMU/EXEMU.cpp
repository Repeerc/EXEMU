// EXEMU.cpp : 定义应用程序的入口点。
//
#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>

#include "framework.h"
#include "EXEMU.h"
#include "EX_EMU.h"



constexpr auto MAX_LOADSTRING = 100;
constexpr auto MAX_LOG_SIZE = 1048576 * 4;
constexpr auto L_width = 600;
constexpr auto SCREEN_SCALE = 2;

// 全局变量:
HINSTANCE hInst;                                // 当前实例
WCHAR szTitle[MAX_LOADSTRING];                  // 标题栏文本
WCHAR szWindowClass[MAX_LOADSTRING];            // 主窗口类名

WCHAR *log_buffer;
unsigned int log_ptr;

// 此代码模块中包含的函数的前向声明:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

HWND    log_text = NULL;
HWND    uart_text = NULL;
HWND    reg_dump_window = NULL;
HWND    main_window_hWnd;

HBITMAP vRamBitMap;
HDC hWndDC;
HDC hBackbufferDC;

RECT screen_rect;

int vram[256][128];

void log(char *text) {

    WCHAR LPWSTRBUFF[1024];
    memset(LPWSTRBUFF, 0, sizeof(WCHAR) * 1024);
    MultiByteToWideChar(CP_ACP, 0, text, strlen(text), LPWSTRBUFF, strlen(text));
    /*
    if (log_ptr + strlen(text) > MAX_LOG_SIZE) {
        memset(log_buffer, 0, MAX_LOG_SIZE * sizeof(WCHAR));
        log_ptr = 0;
    }
    
    wcscpy(&log_buffer[log_ptr], LPWSTRBUFF);
    log_ptr += strlen(text);
    */
    //SetWindowText(log_text, log_buffer);

    SendMessageA(log_text, EM_SETSEL, -2, -1);
    SendMessageA(log_text, EM_REPLACESEL, true, reinterpret_cast<LPARAM>(text));
    
    
    SendMessageA(log_text, WM_VSCROLL, SB_BOTTOM, 0);

}

void clr_reg_dump_text() {
    SetWindowText(reg_dump_window, NULL);
}

void put_reg_dump_text(char *text) {
    SendMessageA(reg_dump_window, EM_SETSEL, -2, -1);
    SendMessageA(reg_dump_window, EM_REPLACESEL, true, reinterpret_cast<LPARAM>(text));
    SendMessageA(reg_dump_window, WM_VSCROLL, SB_BOTTOM, 0);
}

void uart_output(char *text) {
    SendMessageA(uart_text, EM_SETSEL, -2, -1);
    SendMessageA(uart_text, EM_REPLACESEL, true, reinterpret_cast<LPARAM>(text));
    SendMessageA(uart_text, WM_VSCROLL, SB_BOTTOM, 0);

}

void uart_putc(char c) {
    char cs[2];
    cs[0] = c;
    cs[1] = 0;
    SendMessageA(uart_text, EM_SETSEL, -2, -1);
    SendMessageA(uart_text, EM_REPLACESEL, true, reinterpret_cast<LPARAM>(cs));
    SendMessageA(uart_text, WM_VSCROLL, SB_BOTTOM, 0);

}


void log(const char text[]) {
    log((char *)text);
}


int log_f(const char* f, ...) {
    va_list ap;
    int ret;
    char str_buf[1024];
    va_start(ap, f);
    ret = vsprintf(str_buf, f, ap);
    log(str_buf);
    va_end(ap);
    return ret;
}

int reg_dump_log_f(const char* f, ...) {
    va_list ap;
    int ret;
    char str_buf[1024];
    va_start(ap, f);
    ret = vsprintf(str_buf, f, ap);
    put_reg_dump_text(str_buf);
    va_end(ap);
    return ret;
}

void screen_point(int x, int y, int c) {

    SetPixel(hBackbufferDC, x* SCREEN_SCALE, y* SCREEN_SCALE, c);
    SetPixel(hBackbufferDC, x* SCREEN_SCALE+1, y* SCREEN_SCALE, c);
    SetPixel(hBackbufferDC, x* SCREEN_SCALE, y* SCREEN_SCALE+1, c);
    SetPixel(hBackbufferDC, x* SCREEN_SCALE+1, y* SCREEN_SCALE+1, c);

}

void screen_flush() {
    SendMessage(main_window_hWnd, WM_PAINT, 0, 0);
}

void clear_screen() {

    for (int x = 0; x < 256;x++) {
        for (int y = 0; y < 128;y++) {
            screen_point(x, y, RGB(255, 255, 255));
        }
    }

}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: 在此处放置代码。
    memset(vram, 0xff, sizeof(vram));
    screen_rect.left = L_width + 30;
    screen_rect.top = 24;
    screen_rect.right = L_width + 30 + 256 * SCREEN_SCALE;
    screen_rect.bottom = 24 + 128 * SCREEN_SCALE;

    // 初始化全局字符串
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_EXEMU, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // 执行应用程序初始化:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_EXEMU));

    MSG msg;

    HANDLE hThread;
    DWORD  threadId;
   
    emu_start();
    // 主消息循环:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}



//
//  函数: MyRegisterClass()
//
//  目标: 注册窗口类。
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_EXEMU));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_EXEMU);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   函数: InitInstance(HINSTANCE, int)
//
//   目标: 保存实例句柄并创建主窗口
//
//   注释:
//
//        在此函数中，我们在全局变量中保存实例句柄并
//        创建和显示主程序窗口。
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // 将实例句柄存储在全局变量中

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, 1500, 800, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }
   main_window_hWnd = hWnd;
   reg_dump_window = CreateWindowW(TEXT("Edit"), TEXT(""), WS_CHILD | WS_VISIBLE | ES_LEFT |
       WS_VSCROLL | ES_MULTILINE | WS_EX_CLIENTEDGE,
       10, 22, L_width, 240, hWnd, (HMENU)8, (HINSTANCE)hInst, NULL);

   log_text = CreateWindowW(TEXT("Edit"), TEXT("Begin\r\n"), WS_CHILD | WS_VISIBLE | ES_LEFT | 
                                                             WS_VSCROLL | ES_MULTILINE | WS_EX_STATICEDGE,
                                                            10, 282, L_width, 200,hWnd, (HMENU)8, (HINSTANCE)hInst, NULL);

   uart_text = CreateWindowW(TEXT("Edit"), TEXT(""), WS_CHILD | WS_VISIBLE | ES_LEFT |
       WS_VSCROLL | ES_MULTILINE | WS_EX_CLIENTEDGE,
       10, 520, L_width, 220, hWnd, (HMENU)8, (HINSTANCE)hInst, NULL);





   if (!log_text)
   {
       return FALSE;
   }
   
   log_buffer = (WCHAR *)malloc(MAX_LOG_SIZE * sizeof(WCHAR));
   log_ptr = 0;
   if (!log_buffer) {
       return FALSE;
   }


   HFONT hfont = CreateFont(
       -16/*高度*/,
       -8/*宽度*/, 
       0/*不用管*/,
       0/*不用管*/, 
       400 /*一般这个值设为400*/,
       FALSE/*不带斜体*/, 
       FALSE/*不带下划线*/, 
       FALSE/*不带删除线*/,
       DEFAULT_CHARSET,  //这里我们使用默认字符集，还有其他以 _CHARSET 结尾的常量可用
       OUT_CHARACTER_PRECIS, 
       CLIP_CHARACTER_PRECIS,
       DEFAULT_QUALITY,  //默认输出质量
       FF_DONTCARE,  //不指定字体族*/
       L"宋体"  //字体名
   );
   SendMessage(log_text, WM_SETFONT, (WPARAM)hfont, (LPARAM)TRUE);
   SendMessage(uart_text, WM_SETFONT, (WPARAM)hfont, (LPARAM)TRUE);
   SendMessage(reg_dump_window, WM_SETFONT, (WPARAM)hfont, (LPARAM)TRUE);

   hWndDC = GetDC(hWnd);
   vRamBitMap = CreateCompatibleBitmap(hWndDC, screen_rect.right - screen_rect.left, screen_rect.bottom - screen_rect.top);
   hBackbufferDC = CreateCompatibleDC(hWndDC);
   SelectObject(hBackbufferDC, vRamBitMap);

   clear_screen();

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  函数: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  目标: 处理主窗口的消息。
//
//  WM_COMMAND  - 处理应用程序菜单
//  WM_PAINT    - 绘制主窗口
//  WM_DESTROY  - 发送退出消息并返回
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // 分析菜单选择:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            HPEN hPen, bPen;
            HBRUSH screen_brush_w;
            screen_brush_w = CreateSolidBrush(RGB(255,255,255));
            hPen = CreatePen(PS_SOLID, 2, RGB(255,0,0));
            bPen = CreatePen(PS_SOLID,1,RGB(0,0,0));
            SelectObject(hdc, bPen);

            SetTextColor(hdc, RGB(128,0,0));
            TextOut(hdc, 5, 1, L"Registers Dump", lstrlen(L"Registers Dump"));
            TextOut(hdc, 5, 262, L"VM Debug Log", lstrlen(L"VM Debug Log"));
            TextOut(hdc, 5, 500, L"UART Output", lstrlen(L"UART Output"));
            TextOut(hdc, L_width + 30, 1, L"Screen",     lstrlen(L"Screen"));

            Rectangle(hdc, screen_rect.left - 1, screen_rect.top - 1 , screen_rect.right + 1, screen_rect.bottom + 1);

            BitBlt(hWndDC, screen_rect.left, screen_rect.top, screen_rect.right, screen_rect.bottom,
                hBackbufferDC, 0, 0,
                SRCCOPY);

            /*
            for (int x = 0; x < 256;x++) {
                for (int y = 0; y < 128;y++) {
                    SetPixel(hdc, screen_rect.left + x*1.5, screen_rect.top + y*1.5, vram[x][y]);
                }
            }*/
            /*
            FillRect(hdc, &screen_rect, screen_brush_w);
            memset(vram,0,sizeof(vram));
            DeleteObject(bPen);
            DeleteObject(hPen);
            */
            // TODO: 在此处添加使用 hdc 的任何绘图代码...
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// “关于”框的消息处理程序。
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
