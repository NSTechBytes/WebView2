#include "Plugin.h"
#include "HostObjectRmAPI.h"
#include "../API/RainmeterAPI.h"
#include <vector>
#include <sstream>

// Window class registration flag
static bool g_windowClassRegistered = false;

// Window procedure for WebView2 host window
LRESULT CALLBACK WebViewWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_SIZE:
        {
            // Get measure from window user data
            Measure* measure = reinterpret_cast<Measure*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
            if (measure && measure->webViewController)
            {
                RECT bounds;
                GetClientRect(hwnd, &bounds);
                measure->webViewController->put_Bounds(bounds);
            }
            return 0;
        }
        
        case WM_DESTROY:
            return 0;
            
        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}

// Register window class for WebView2 host
void RegisterWebViewWindowClass()
{
    if (g_windowClassRegistered)
        return;
    
    WNDCLASSEX wc = { 0 };
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = WebViewWindowProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = L"RainmeterWebView2Host";
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    
    if (RegisterClassEx(&wc))
    {
        g_windowClassRegistered = true;
    }
}


// Create WebView2 environment and controller
void CreateWebView2(Measure* measure)
{
    if (!measure || !measure->skinWindow)
    {
        if (measure && measure->rm)
            RmLog(measure->rm, LOG_ERROR, L"WebView2: Invalid measure or skin window");
        return;
    }
    
    // Register window class
    RegisterWebViewWindowClass();
    
    // Create host window as child of skin window
    measure->webViewWindow = CreateWindowEx(
        0,
        L"RainmeterWebView2Host",
        L"WebView2",
        WS_CHILD | (measure->visible ? WS_VISIBLE : 0),
        measure->x, measure->y,
        measure->width, measure->height,
        measure->skinWindow,
        nullptr,
        GetModuleHandle(nullptr),
        nullptr
    );
    
    if (!measure->webViewWindow)
    {
        if (measure->rm)
            RmLog(measure->rm, LOG_ERROR, L"WebView2: Failed to create host window");
        return;
    }
    
    // Store measure pointer in window
    SetWindowLongPtr(measure->webViewWindow, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(measure));
    
    // Create user data folder in TEMP directory to avoid permission issues
    wchar_t tempPath[MAX_PATH];
    GetTempPathW(MAX_PATH, tempPath);
    std::wstring userDataFolder = std::wstring(tempPath) + L"RainmeterWebView2";
    
    // Create the directory if it doesn't exist
    CreateDirectoryW(userDataFolder.c_str(), nullptr);
    
    // Create WebView2 environment with user data folder
    HRESULT hr = CreateCoreWebView2EnvironmentWithOptions(
        nullptr, userDataFolder.c_str(), nullptr,
        Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
            [measure](HRESULT result, ICoreWebView2Environment* env) -> HRESULT
            {
                if (FAILED(result))
                {
                    if (measure->rm)
                    {
                        wchar_t errorMsg[256];
                        swprintf_s(errorMsg, L"WebView2: Failed to create environment (HRESULT: 0x%08X)", result);
                        RmLog(measure->rm, LOG_ERROR, errorMsg);
                    }
                    return result;
                }
                
                // Create WebView2 controller
                env->CreateCoreWebView2Controller(
                    measure->webViewWindow,
                    Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                        [measure](HRESULT result, ICoreWebView2Controller* controller) -> HRESULT
                        {
                            if (FAILED(result))
                            {
                                if (measure->rm)
                                {
                                    wchar_t errorMsg[256];
                                    swprintf_s(errorMsg, L"WebView2: Failed to create controller (HRESULT: 0x%08X)", result);
                                    RmLog(measure->rm, LOG_ERROR, errorMsg);
                                }
                                return result;
                            }
                            
                            measure->webViewController = controller;
                            measure->webViewController->get_CoreWebView2(&measure->webView);
                            
                            // Set bounds
                            RECT bounds;
                            GetClientRect(measure->webViewWindow, &bounds);
                            measure->webViewController->put_Bounds(bounds);
                            
                            // Set initial visibility
                            measure->webViewController->put_IsVisible(measure->visible ? TRUE : FALSE);
                            
                            // Enable host objects in settings
                            wil::com_ptr<ICoreWebView2Settings> settings;
                            measure->webView->get_Settings(&settings);
                            settings->put_AreHostObjectsAllowed(TRUE);
                            
                            // Create and inject COM Host Object for Rainmeter API
                            wil::com_ptr<HostObjectRmAPI> hostObject = 
                                Microsoft::WRL::Make<HostObjectRmAPI>(measure, g_typeLib);
                            
                            VARIANT variant = {};
                            hostObject.query_to<IDispatch>(&variant.pdispVal);
                            variant.vt = VT_DISPATCH;
                            measure->webView->AddHostObjectToScript(L"rm", &variant);
                            variant.pdispVal->Release();
                            
                            // Add script to make rm available globally
                            measure->webView->AddScriptToExecuteOnDocumentCreated(
                                L"window.rm = chrome.webview.hostObjects.sync.rm",
                                nullptr
                            );
                            
                            // Navigate to URL
                            if (!measure->url.empty())
                            {
                                measure->webView->Navigate(measure->url.c_str());
                            }
                            
                            measure->initialized = true;
                            
                            if (measure->rm)
                                RmLog(measure->rm, LOG_NOTICE, L"WebView2: Initialized successfully with COM Host Objects");
                            
                            return S_OK;
                        }
                    ).Get()
                );
                
                return S_OK;
            }
        ).Get()
    );
    
    if (FAILED(hr))
    {
        if (measure->rm)
        {
            wchar_t errorMsg[512];
            swprintf_s(errorMsg, L"WebView2: Failed to start creation process (HRESULT: 0x%08X). Make sure WebView2 Runtime is installed.", hr);
            RmLog(measure->rm, LOG_ERROR, errorMsg);
        }
    }
}
