// Copyright (C) 2025 nstechbytes. All rights reserved.
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

    if (measure->initialized)
    {
        return;
    }
    
    if (measure->isCreationInProgress)
    {
        return;
    }

    measure->isCreationInProgress = true;
    
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
        if (measure->skin && wcslen(measure->onWebViewFailAction.c_str()) > 0)
        {
            RmExecute(measure->skin, measure->onWebViewFailAction.c_str());
        }
        measure->isCreationInProgress = false;
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
        if (skin && wcslen(onWebViewFailAction.c_str()) > 0)
        {
            RmExecute(skin, onWebViewFailAction.c_str());
        }
        isCreationInProgress = false;
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
        if (skin && wcslen(onWebViewFailAction.c_str()) > 0)
        {
            RmExecute(skin, onWebViewFailAction.c_str());
        }
        isCreationInProgress = false;
        return result;
    }
    
    if (controller == nullptr)
    {
        if (rm)
            RmLog(rm, LOG_ERROR, L"WebView2: Controller is null");
        if (skin && wcslen(onWebViewFailAction.c_str()) > 0)
        {
            RmExecute(skin, onWebViewFailAction.c_str());
        }
        isCreationInProgress = false;
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
    webView->AddHostObjectToScript(L"RainmeterAPI", &variant);
    variant.pdispVal->Release();
    
    // Add script to make RainmeterAPI available globally
    webView->AddScriptToExecuteOnDocumentCreated(
        L"window.RainmeterAPI = chrome.webview.hostObjects.sync.RainmeterAPI",
        nullptr
    );

    // Add SourceChanged event to detect changes in URL
    webView->add_SourceChanged(
        Callback<ICoreWebView2SourceChangedEventHandler>(
            [this](ICoreWebView2* sender, ICoreWebView2SourceChangedEventArgs* args) -> HRESULT
            {
                wil::unique_cotaskmem_string updatedUri;

                if (SUCCEEDED(sender->get_Source(&updatedUri)) && updatedUri.get() != nullptr)
                {
                    std::wstring newUrl = updatedUri.get();

                    if (currentUrl != newUrl)
                    {
                        // URL changed 
                        isFirstLoad = true;
                        currentUrl = newUrl;       
                    }
                    else
                    {
                        // URL did not change
                        isFirstLoad = false;
                    }
                }
                return S_OK;
            }
        ).Get(),
        nullptr
    );

	// Add NavigationStarting event to call action when navigation starts
    webView->add_NavigationStarting(
    Callback<ICoreWebView2NavigationStartingEventHandler>(
        [this](ICoreWebView2* sender, ICoreWebView2NavigationStartingEventArgs* args) -> HRESULT
        {
            if (wcslen(onPageLoadStartAction.c_str()) > 0)
            {
                if (skin)
                    RmExecute(skin, onPageLoadStartAction.c_str());
            }
            return S_OK;
        }
    ).Get(),
    nullptr
    );

	// Add ContentLoading event to call action when page starts loading
    webView->add_ContentLoading(
        Callback<ICoreWebView2ContentLoadingEventHandler>(
            [this](ICoreWebView2* sender, ICoreWebView2ContentLoadingEventArgs* args) -> HRESULT
            {
                if (wcslen(onPageLoadingAction.c_str()) > 0)
                {
                    if (skin)
                        RmExecute(skin, onPageLoadingAction.c_str());
                }
                return S_OK;
            }
        ).Get(),
        nullptr
    );

	// Add NavigationCompleted event to call OnInitialize after page loads and handle load actions
    webView->add_NavigationCompleted(
        Callback<ICoreWebView2NavigationCompletedEventHandler>(
            [this](ICoreWebView2* sender, ICoreWebView2NavigationCompletedEventArgs* args) -> HRESULT
            {
                isAllowDualControlInjected = false;

                // Inject script to capture page load events for drag/move and context menu
                if (allowDualControl)
                {
                    InjectAllowDualControl(this);
                }
               
                // Call JavaScript OnInitialize callback if it exists and capture return value
                webView->ExecuteScript(
                    L"(function() { if (typeof window.OnInitialize === 'function') { var result = window.OnInitialize(); return result !== undefined ? String(result) : ''; } return ''; })();",
                    Callback<ICoreWebView2ExecuteScriptCompletedHandler>(
                        [this](HRESULT errorCode, LPCWSTR resultObjectAsJson) -> HRESULT
                        {
                            if (SUCCEEDED(errorCode) && resultObjectAsJson)
                            {
                                // Remove quotes from JSON string result
                                std::wstring result = resultObjectAsJson;
                                if (result.length() >= 2 && result.front() == L'"' && result.back() == L'"')
                                {
                                    result = result.substr(1, result.length() - 2);
                                }

                                // Store the callback result
                                if (!result.empty() && result != L"null")
                                {
                                    callbackResult = result;
                                }
                            }
                            return S_OK;
                        }
                    ).Get()
                );

				if (isFirstLoad) // First load
                {
                    if (wcslen(onPageFirstLoadAction.c_str()) > 0)
                    {
                        if (skin)
                            RmExecute(skin, onPageFirstLoadAction.c_str());
                    }
                    isFirstLoad = false;
                }
				else // Page reload
                {
                    if (wcslen(onPageReloadAction.c_str()) > 0)
                    {
                        if (skin)
                            RmExecute(skin, onPageReloadAction.c_str());
                    }
                }
				// Common action after any page load
                if (wcslen(onPageLoadFinishAction.c_str()) > 0)
                {
                    if (skin)
                        RmExecute(skin, onPageLoadFinishAction.c_str());
                }
                return S_OK;
            }
        ).Get(),
        nullptr
    );

    // Navigate to URL
    if (!url.empty())
    {
        webView->Navigate(url.c_str());
    }
    
    initialized = true;
    
    isCreationInProgress = false;

    if (rm)
        RmLog(rm, LOG_NOTICE, L"WebView2: Initialized successfully with COM Host Objects");
    
    if (wcslen(onWebViewLoadAction.c_str()) > 0)
    {
        RmExecute(skin, onWebViewLoadAction.c_str());
    }

    // Apply initial clickthrough state
    UpdateClickthrough(this);
    
    return S_OK;
}
