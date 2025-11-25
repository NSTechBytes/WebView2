#pragma once

#include <Windows.h>
#include <string>
#include <wrl.h>
#include <wil/com.h>
#include <WebView2.h>

using namespace Microsoft::WRL;

// Global TypeLib for COM objects
extern wil::com_ptr<ITypeLib> g_typeLib;

// Measure structure containing WebView2 state
struct Measure
{
    void* rm;
    void* skin;
    HWND skinWindow;
    HWND webViewWindow;
    LPCWSTR measureName;
    
    std::wstring url;
    int width;
    int height;
    int x;
    int y;
    bool visible;
    bool initialized;
    
    wil::com_ptr<ICoreWebView2Controller> webViewController;
    wil::com_ptr<ICoreWebView2> webView;
    EventRegistrationToken webMessageToken;
    
    Measure();
    ~Measure();
};

// WebView2 functions
void CreateWebView2(Measure* measure);
void RegisterWebViewWindowClass();
LRESULT CALLBACK WebViewWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
