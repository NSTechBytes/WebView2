#include "Plugin.h"
#include "HostObjectRmAPI.h"
#include "../API/RainmeterAPI.h"

// Create WebView2 environment and controller
void CreateWebView2(Measure* measure)
{
    if (!measure || !measure->skinWindow)
    {
        if (measure && measure->rm)
            RmLog(measure->rm, LOG_ERROR, L"WebView2: Invalid measure or skin window");
        return;
    }
    
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
            measure,
            &Measure::CreateEnvironmentHandler
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

// Environment creation callback
HRESULT Measure::CreateEnvironmentHandler(HRESULT result, ICoreWebView2Environment* env)
{
    if (FAILED(result))
    {
        if (rm)
        {
            wchar_t errorMsg[256];
            swprintf_s(errorMsg, L"WebView2: Failed to create environment (HRESULT: 0x%08X)", result);
            RmLog(rm, LOG_ERROR, errorMsg);
        }
        return result;
    }
    
    // Create WebView2 controller using skin window directly
    env->CreateCoreWebView2Controller(
        skinWindow,
        Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
            this,
            &Measure::CreateControllerHandler
        ).Get()
    );
    
    return S_OK;
}

// Controller creation callback
HRESULT Measure::CreateControllerHandler(HRESULT result, ICoreWebView2Controller* controller)
{
    if (FAILED(result))
    {
        if (rm)
        {
            wchar_t errorMsg[256];
            swprintf_s(errorMsg, L"WebView2: Failed to create controller (HRESULT: 0x%08X)", result);
            RmLog(rm, LOG_ERROR, errorMsg);
        }
        return result;
    }
    
    if (controller == nullptr)
    {
        if (rm)
            RmLog(rm, LOG_ERROR, L"WebView2: Controller is null");
        return S_FALSE;
    }
    
    webViewController = controller;
    webViewController->get_CoreWebView2(&webView);
    
    // Set bounds within the skin window
    RECT bounds;
    GetClientRect(skinWindow, &bounds);
    bounds.left = x;
    bounds.top = y;
    if (width > 0)
    {
        bounds.right = x + width;
    }
    if (height > 0)
    {
        bounds.bottom = y + height;
    }
    webViewController->put_Bounds(bounds);
    
    // Set initial visibility
    webViewController->put_IsVisible(visible ? TRUE : FALSE);
    
    // Transparent background
    auto controller2 = webViewController.query<ICoreWebView2Controller2>();
    if (controller2)
    {
        COREWEBVIEW2_COLOR transparentColor = { 0, 0, 0, 0 };
        controller2->put_DefaultBackgroundColor(transparentColor);
    }
    
    // Enable host objects and JavaScript in settings
    wil::com_ptr<ICoreWebView2Settings> settings;
    webView->get_Settings(&settings);
    settings->put_IsScriptEnabled(TRUE);
    settings->put_AreDefaultScriptDialogsEnabled(TRUE);
    settings->put_IsWebMessageEnabled(TRUE);
    settings->put_AreHostObjectsAllowed(TRUE);
    settings->put_AreDevToolsEnabled(TRUE);
    settings->put_AreDefaultContextMenusEnabled(TRUE);
    
    // Create and inject COM Host Object for Rainmeter API
    wil::com_ptr<HostObjectRmAPI> hostObject = 
        Microsoft::WRL::Make<HostObjectRmAPI>(this, g_typeLib);
    
    VARIANT variant = {};
    hostObject.query_to<IDispatch>(&variant.pdispVal);
    variant.vt = VT_DISPATCH;
    webView->AddHostObjectToScript(L"rm", &variant);
    variant.pdispVal->Release();
    
    // Add script to make rm available globally
    webView->AddScriptToExecuteOnDocumentCreated(
        L"window.rm = chrome.webview.hostObjects.sync.rm",
        nullptr
    );
    
    // Navigate to URL
    if (!url.empty())
    {
        webView->Navigate(url.c_str());
    }
    
    initialized = true;
    
    if (rm)
        RmLog(rm, LOG_NOTICE, L"WebView2: Initialized successfully with COM Host Objects");
    
    return S_OK;
}
