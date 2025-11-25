#pragma once

#include <Windows.h>
#include <string>
#include <wrl.h>
#include <wil/com.h>
#include <WebView2.h>

using namespace Microsoft::WRL;

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
    
    Measure();
    ~Measure();
};

// WebView2 functions
void CreateWebView2(Measure* measure);
void RegisterWebViewWindowClass();
void InjectJavaScriptBridge(Measure* measure);
void HandleWebMessage(Measure* measure, LPCWSTR message);
LRESULT CALLBACK WebViewWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
