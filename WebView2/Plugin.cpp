// Copyright (C) 2025 nstechbytes. All rights reserved.
#include "Plugin.h"
#include "../API/RainmeterAPI.h"

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")

// Global COM initialization tracking
static bool g_comInitialized = false;

// Global TypeLib for COM objects
wil::com_ptr<ITypeLib> g_typeLib;

// DllMain to load TypeLib from embedded resources
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        // Extract TypeLib from embedded resource and load it
        wchar_t tempPath[MAX_PATH];
        GetTempPath(MAX_PATH, tempPath);
        wcscat_s(tempPath, L"WebView2.tlb");

        // Read embedded resource: ID = 1, Type = TYPELIB
        HRSRC hResInfo = FindResource(hinstDLL, MAKEINTRESOURCE(1), L"TYPELIB");
        if (hResInfo)
        {
            HGLOBAL hRes = LoadResource(hinstDLL, hResInfo);
            if (hRes)  // Check if LoadResource succeeded
            {
                LPVOID memRes = LockResource(hRes);
                DWORD sizeRes = SizeofResource(hinstDLL, hResInfo);

                HANDLE hFile = CreateFile(tempPath, GENERIC_WRITE, 0, NULL,
                                           CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
                if (hFile != INVALID_HANDLE_VALUE)
                {
                    DWORD written;
                    WriteFile(hFile, memRes, sizeRes, &written, NULL);
                    CloseHandle(hFile);

                    // Load the TypeLib
                    LoadTypeLib(tempPath, &g_typeLib);
                }
            }
        }
    }
    return TRUE;
}

// Measure constructor
Measure::Measure() : rm(nullptr), skin(nullptr), skinWindow(nullptr), 
            measureName(nullptr),
            width(800), height(600), x(0), y(0), zoomFactor(1.0),
            visible(true), initialized(false), clickthrough(false), allowDualControl(true), webMessageToken{}
{
    // Initialize COM for this thread if not already done
    if (!g_comInitialized)
    {
        HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        if (SUCCEEDED(hr) || hr == RPC_E_CHANGED_MODE)
        {
            g_comInitialized = true;
        }
    }
}

// Measure destructor
Measure::~Measure()
{
}

// Rainmeter Plugin Exports
PLUGIN_EXPORT void Initialize(void** data, void* rm)
{
    Measure* measure = new Measure;
    *data = measure;
    
    measure->rm = rm;
    measure->skin = RmGetSkin(rm);
    measure->skinWindow = RmGetSkinWindow(rm);
    measure->measureName = RmGetMeasureName(rm);
}

// Helper to update clickthrough state
void UpdateClickthrough(Measure* measure)
{
    if (!measure->skinWindow) return;

    // Find the WebView2 window (child of skin window)
    // We iterate through children to find the one that matches our bounds
    HWND child = GetWindow(measure->skinWindow, GW_CHILD);
    while (child)
    {
        // Check if this is likely our window
        // For simplicity, we assume the first child or check bounds if needed
        // Since we can't easily map controller to HWND, we'll try to apply to all children 
        // that look like WebView windows (or just the first one if we assume 1 per skin for now)
        
        // Better approach: Check if the window rect matches our measure bounds
        RECT rect;
        GetWindowRect(child, &rect);
        
        // Convert to client coordinates of parent
        POINT pt = { rect.left, rect.top };
        ScreenToClient(measure->skinWindow, &pt);
        
        // Allow some tolerance or just apply to all children?
        // Applying to all children might be safer for "Clickthrough" if there are multiple WebViews
        // and we want them all to respect their settings.
        // But if we have multiple measures, we want to target ONLY ours.
        
        // For now, let's just apply to the child window found.
        // EnableWindow(FALSE) makes it ignore mouse input (Clickthrough=1)
        // EnableWindow(TRUE) makes it accept mouse input (Clickthrough=0)
        EnableWindow(child, !measure->clickthrough);

        // If enabling clickthrough (disabling input), ensure it loses focus
        if (measure->clickthrough)
        {
            HWND focusedWindow = GetFocus();
            if (focusedWindow && (focusedWindow == child || IsChild(child, focusedWindow)))
            {
                SetFocus(nullptr);
            }
        }
        
        child = GetWindow(child, GW_HWNDNEXT);
    }
}

// Inject AllowDualControl script into the WebView
void InjectAllowDualControl(Measure* measure)
{
    if (!measure->webView) return;
    // Inject script to capture page load events for drag/move and context menu
    measure->webView->ExecuteScript(
        L"let rm_AllowDualControl=false,rm_AllowDualControlOn=false,rm_AllowDualControlClientX=0,rm_AllowDualControlClientY=0;function rm_SetAllowDualControl(v){rm_AllowDualControl=!!v;if(!rm_AllowDualControl)rm_AllowDualControlOn=false;}document.body.onpointerdown=e=>{if(!rm_AllowDualControl)return;if(e.button===0&&e.ctrlKey){e.preventDefault();e.stopImmediatePropagation();rm_AllowDualControlOn=true;rm_AllowDualControlClientX=e.clientX;rm_AllowDualControlClientY=e.clientY;try{document.body.setPointerCapture(e.pointerId);}catch{}}};document.body.onpointermove=e=>{if(!rm_AllowDualControl||!rm_AllowDualControlOn)return;e.preventDefault();RainmeterAPI.Bang('[!Move '+(e.screenX-RainmeterAPI.ReadFormula('X',0)-rm_AllowDualControlClientX)+' '+(e.screenY-RainmeterAPI.ReadFormula('Y',0)-rm_AllowDualControlClientY)+']');};document.body.onpointerup=e=>{if(!rm_AllowDualControl)return;if(e.button===0){e.preventDefault();rm_AllowDualControlOn=false;try{document.body.releasePointerCapture(e.pointerId);}catch{}}};document.body.oncontextmenu=e=>{if(!rm_AllowDualControl)return;if(e.button===2&&e.ctrlKey){e.preventDefault();RainmeterAPI.Bang('[!SkinMenu]');}};",
        Callback<ICoreWebView2ExecuteScriptCompletedHandler>(
            [measure](HRESULT errorCode, LPCWSTR resultObjectAsJson) -> HRESULT
            {
                return S_OK;
            }
        ).Get()
    );
	measure->isAllowDualControlInjected = true;
    UpdateAllowDualControl(measure);
}

// Update AllowDualControl state in the WebView
void UpdateAllowDualControl(Measure* measure)
{
    if (!measure->webView) return;

    if (measure->allowDualControl)
    {
        measure->webView->ExecuteScript(
            L"rm_SetAllowDualControl(true);",
            Callback<ICoreWebView2ExecuteScriptCompletedHandler>(
                [measure](HRESULT errorCode, LPCWSTR resultObjectAsJson) -> HRESULT
                {
                    return S_OK;
                }
            ).Get()
        );
    }
    else {
        measure->webView->ExecuteScript(
            L"rm_SetAllowDualControl(false);",
            Callback<ICoreWebView2ExecuteScriptCompletedHandler>(
                [measure](HRESULT errorCode, LPCWSTR resultObjectAsJson) -> HRESULT
                {
                    return S_OK;
                }
            ).Get()
        );
    }
}

PLUGIN_EXPORT void Reload(void* data, void* rm, double* maxValue)
{
    Measure* measure = (Measure*)data;
    
    // Read URL
    std::wstring newUrl;
    LPCWSTR urlOption = RmReadString(rm, L"Url", L"");
    if (urlOption && wcslen(urlOption) > 0)
    {
        std::wstring urlStr = urlOption;
        
        // Check if it's a web URL (http://, https://, etc.)
        if (urlStr.find(L"://") != std::wstring::npos)
        {
            // Already has a protocol - use as-is
            // This handles: http://, https://, file:///, etc.
            newUrl = urlStr;
        }
        else
        {
            // No protocol found - treat as file path
            // Check if it's a relative path or absolute path
            if (urlStr[0] != L'/' && (urlStr.length() < 2 || urlStr[1] != L':'))
            {
                // Relative path - make it absolute using skin path
                LPCWSTR absolutePath = RmPathToAbsolute(rm, urlStr.c_str());
                if (absolutePath)
                {
                    urlStr = absolutePath;
                }
            }
            
            // Convert backslashes to forward slashes
            for (size_t i = 0; i < urlStr.length(); i++)
            {
                if (urlStr[i] == L'\\') urlStr[i] = L'/';
            }
            
            // Add file:/// prefix if not already present
            if (urlStr.find(L"file:///") != 0)
            {
                urlStr = L"file:///" + urlStr;
            }
            
            newUrl = urlStr;
        }
    }
    
    // Read dimensions and visibility
    int newWidth = RmReadInt(rm, L"W", 800);
    int newHeight = RmReadInt(rm, L"H", 600);
    int newX = RmReadInt(rm, L"X", 0);
    int newY = RmReadInt(rm, L"Y", 0);
	double newZoomFactor = RmReadFormula(rm, L"ZoomFactor", 1.0);
    bool newVisible = RmReadInt(rm, L"Hidden", 0) <= 0;
   	bool newClickthrough = RmReadInt(rm, L"Clickthrough", 0) >= 1;
	
	// Read AllowDualControl for Yincognito's script injection
    bool newAllowDualControl = RmReadInt(rm, L"AllowDualControl", 1) >= 1;

	// Read OnWebViewLoadAction
	std::wstring newOnWebViewLoadAction;
	LPCWSTR onWebViewLoadOption = RmReadString(rm, L"OnWebViewLoadAction", L"", FALSE);
	if (onWebViewLoadOption && wcslen(onWebViewLoadOption) > 0)
	{
    	newOnWebViewLoadAction = onWebViewLoadOption;
	}

	// Read OnWebViewFailAction
	std::wstring newOnWebViewFailAction;
	LPCWSTR onWebViewFailOption = RmReadString(rm, L"OnWebViewFailAction", L"", FALSE);
	if (onWebViewFailOption && wcslen(onWebViewFailOption) > 0)
	{
    	newOnWebViewFailAction = onWebViewFailOption;
	}

	// Read OnPageLoadStartAction
	std::wstring newOnPageLoadStartAction;
	LPCWSTR onPageLoadStartOption = RmReadString(rm, L"OnPageLoadStartAction", L"", FALSE);
	if (onPageLoadStartOption && wcslen(onPageLoadStartOption) > 0)
	{
    	newOnPageLoadStartAction = onPageLoadStartOption;
	}

	// Read OnPageLoadingAction
	std::wstring newOnPageLoadingAction;
	LPCWSTR onPageLoadingOption = RmReadString(rm, L"OnPageLoadingAction", L"", FALSE);
	if (onPageLoadingOption && wcslen(onPageLoadingOption) > 0)
	{
    	newOnPageLoadingAction = onPageLoadingOption;
	}

	// Read OnPageLoadFinishAction
	std::wstring newOnPageLoadFinishAction;
	LPCWSTR onPageLoadFinishOption = RmReadString(rm, L"OnPageLoadFinishAction", L"", FALSE);
	if (onPageLoadFinishOption && wcslen(onPageLoadFinishOption) > 0)
	{
    	newOnPageLoadFinishAction = onPageLoadFinishOption;
	}

	// Read OnPageFirstLoadAction
	std::wstring newOnPageFirstLoadAction;
	LPCWSTR onPageFirstLoadOption = RmReadString(rm, L"OnPageFirstLoadAction", L"", FALSE);
	if (onPageFirstLoadOption && wcslen(onPageFirstLoadOption) > 0)
	{
    	newOnPageFirstLoadAction = onPageFirstLoadOption;
	}

	// Read OnPageReloadAction
	std::wstring newOnPageReloadAction;
	LPCWSTR onPageReloadOption = RmReadString(rm, L"OnPageReloadAction", L"", FALSE);
	if (onPageReloadOption && wcslen(onPageReloadOption) > 0)
	{
    	newOnPageReloadAction = onPageReloadOption;
	}
	
    // Check if URL has changed (requires recreation)
    bool urlChanged = (newUrl != measure->url);

    // Check if dimensions or position changed (can be updated dynamically)
    bool dimensionsChanged = (newWidth != measure->width || 
                             newHeight != measure->height || 
                             newX != measure->x || 
                             newY != measure->y);
    
    bool visibilityChanged = (newVisible != measure->visible);
    bool clickthroughChanged = (newClickthrough != measure->clickthrough);
	bool allowDualControlChanged = (newAllowDualControl != measure->allowDualControl);
	bool zoomFactorChanged = (newZoomFactor != measure->zoomFactor);
	
    // Update stored values
    measure->url = newUrl;
    measure->width = newWidth;
    measure->height = newHeight;
    measure->x = newX;
    measure->y = newY;
	measure->zoomFactor = newZoomFactor;
    measure->visible = newVisible;
    measure->clickthrough = newClickthrough;
	measure->allowDualControl = newAllowDualControl;
    measure->onWebViewLoadAction = newOnWebViewLoadAction;
    measure->onWebViewFailAction = newOnWebViewFailAction;
    measure->onPageLoadStartAction = newOnPageLoadStartAction;
    measure->onPageLoadingAction = newOnPageLoadingAction;
    measure->onPageLoadFinishAction = newOnPageLoadFinishAction;
    measure->onPageFirstLoadAction = newOnPageFirstLoadAction;
    measure->onPageReloadAction = newOnPageReloadAction;

    // Only create WebView2 if not initialized OR if URL changed
    if (!measure->initialized || urlChanged)
    {
        if (urlChanged && measure->initialized)
        {
            // URL changed - navigate to new URL instead of recreating
            if (measure->webView && !newUrl.empty())
            {
                measure->webView->Navigate(newUrl.c_str());
            }
        }
        else
        {
            // First initialization - create WebView2
            if (measure->isCreationInProgress)
            {
                // Avoid re-entrancy if creation is already in progress
                return;
            }
            CreateWebView2(measure);
        }
    }
    else
    {
        // WebView2 already exists - update properties dynamically
        if (dimensionsChanged && measure->webViewController)
        {
            RECT bounds;
            GetClientRect(measure->skinWindow, &bounds);
            bounds.left = measure->x;
            bounds.top = measure->y;
            bounds.right = measure->x + measure->width;
            bounds.bottom = measure->y + measure->height;
            measure->webViewController->put_Bounds(bounds);
        }
        
        if (visibilityChanged && measure->webViewController)
        {
            measure->webViewController->put_IsVisible(measure->visible ? TRUE : FALSE);
        }
		
        if (zoomFactorChanged && measure->webViewController)
        {
            measure->webViewController->put_ZoomFactor(measure->zoomFactor);
		}
        
        if (clickthroughChanged)
        {
            UpdateClickthrough(measure);
        }

        if (allowDualControlChanged)
        {
            if (!measure->isAllowDualControlInjected)
            {
                InjectAllowDualControl(measure);
            }
			else
            UpdateAllowDualControl(measure);
        }
    }
}

PLUGIN_EXPORT double Update(void* data)
{
    Measure* measure = (Measure*)data;
    
    // Call JavaScript OnUpdate callback if WebView is initialized
    if (measure->initialized && measure->webView)
    {
        measure->webView->ExecuteScript(
            L"(function() { if (typeof window.OnUpdate === 'function') { var result = window.OnUpdate(); return result !== undefined ? String(result) : ''; } return ''; })();",
            Callback<ICoreWebView2ExecuteScriptCompletedHandler>(
                [measure](HRESULT errorCode, LPCWSTR resultObjectAsJson) -> HRESULT
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
                            measure->callbackResult = result;
                        }
                    }
                    return S_OK;
                }
            ).Get()
        );
    }
    
    return measure->initialized ? 1.0 : 0.0;
}

PLUGIN_EXPORT LPCWSTR GetString(void* data)
{
    Measure* measure = (Measure*)data;
    
    // Return the callback result if available, otherwise return "0"
    if (!measure->callbackResult.empty())
    {
        return measure->callbackResult.c_str();
    }
    
    return L"0";
}

PLUGIN_EXPORT void ExecuteBang(void* data, LPCWSTR args)
{
    Measure* measure = (Measure*)data;
    
    if (!measure || !measure->webView)
        return;
    
    std::wstring command = args;
    
    // Parse command
    size_t spacePos = command.find(L' ');
    std::wstring action = (spacePos != std::wstring::npos) ? 
                          command.substr(0, spacePos) : command;
    std::wstring param = (spacePos != std::wstring::npos) ? 
                         command.substr(spacePos + 1) : L"";
    
    if (_wcsicmp(action.c_str(), L"Navigate") == 0)
    {
        if (!param.empty())
        {
            measure->webView->Navigate(param.c_str());
        }
    }
    else if (_wcsicmp(action.c_str(), L"Reload") == 0)
    {
        measure->webView->Reload();
    }
    else if (_wcsicmp(action.c_str(), L"GoBack") == 0)
    {
        measure->webView->GoBack();
    }
    else if (_wcsicmp(action.c_str(), L"GoForward") == 0)
    {
        measure->webView->GoForward();
    }
    else if (_wcsicmp(action.c_str(), L"ExecuteScript") == 0)
    {
        if (!param.empty())
        {
            measure->webView->ExecuteScript(
                param.c_str(),
                Callback<ICoreWebView2ExecuteScriptCompletedHandler>(
                    [](HRESULT errorCode, LPCWSTR resultObjectAsJson) -> HRESULT
                    {
                        return S_OK;
                    }
                ).Get()
            );
        }
    }
    else if (_wcsicmp(action.c_str(), L"OpenDevTools") == 0)
    {
        measure->webView->OpenDevToolsWindow();
    }
}

// Generic JavaScript function caller
PLUGIN_EXPORT LPCWSTR CallJS(void* data, const int argc, const WCHAR* argv[])
{
    Measure* measure = (Measure*)data;
    
    if (!measure || !measure->initialized || !measure->webView)
        return L"";
    
    if (argc == 0 || !argv[0])
        return L"";
    
    // Build unique key for this call: functionName|arg1|arg2...
    std::wstring key = argv[0];
    for (int i = 1; i < argc; i++)
    {
        key += L"|";
        key += argv[i];
    }
    
    // Build JavaScript call: functionName(arg1, arg2, ...)
    std::wstring jsCode = L"(function() { try { if (typeof " + std::wstring(argv[0]) + L" === 'function') { var result = " + std::wstring(argv[0]) + L"(";
    
    // Add arguments if provided
    for (int i = 1; i < argc; i++)
    {
        if (i > 1) jsCode += L", ";
        jsCode += L"'" + std::wstring(argv[i]) + L"'";
    }
    
    jsCode += L"); return result !== undefined ? String(result) : ''; } return 'Function not found'; } catch(e) { return 'Error: ' + e.message; } })();";
    
    // Execute asynchronously and update cache
    measure->webView->ExecuteScript(
        jsCode.c_str(),
        Callback<ICoreWebView2ExecuteScriptCompletedHandler>(
            [measure, key](HRESULT errorCode, LPCWSTR resultObjectAsJson) -> HRESULT
            {
                if (SUCCEEDED(errorCode) && resultObjectAsJson)
                {
                    std::wstring result = resultObjectAsJson;
                    if (result.length() >= 2 && result.front() == L'"' && result.back() == L'"')
                    {
                        result = result.substr(1, result.length() - 2);
                    }
                    
                    if (!result.empty() && result != L"null")
                    {
                        // Update cache for this specific call
                        measure->jsResults[key] = result;
                    }
                }
                return S_OK;
            }
        ).Get()
    );
    
    // Return cached result if available, otherwise "0"
    if (measure->jsResults.find(key) != measure->jsResults.end())
    {
        measure->buffer = measure->jsResults[key];
    }
    else
    {
        measure->buffer = L"0";
    }
    
    return measure->buffer.c_str();
}

PLUGIN_EXPORT void Finalize(void* data)
{
    Measure* measure = (Measure*)data;
    delete measure;
}
