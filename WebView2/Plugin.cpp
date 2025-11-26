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
            width(800), height(600), x(0), y(0), 
            visible(true), initialized(false), webMessageToken{}
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
    bool newVisible = RmReadInt(rm, L"Hidden", 0) == 0;
    
    // Check if URL has changed (requires recreation)
    bool urlChanged = (newUrl != measure->url);
    
    // Check if dimensions or position changed (can be updated dynamically)
    bool dimensionsChanged = (newWidth != measure->width || 
                             newHeight != measure->height || 
                             newX != measure->x || 
                             newY != measure->y);
    
    bool visibilityChanged = (newVisible != measure->visible);
    
    // Update stored values
    measure->url = newUrl;
    measure->width = newWidth;
    measure->height = newHeight;
    measure->x = newX;
    measure->y = newY;
    measure->visible = newVisible;
    
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
    }
}

PLUGIN_EXPORT double Update(void* data)
{
    Measure* measure = (Measure*)data;
    return measure->initialized ? 1.0 : 0.0;
}

PLUGIN_EXPORT LPCWSTR GetString(void* data)
{
    Measure* measure = (Measure*)data;
    static std::wstring result;
    
    if (measure->initialized)
    {
        result = L"WebView2 Initialized";
    }
    else
    {
        result = L"WebView2 Initializing...";
    }
    
    return result.c_str();
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

PLUGIN_EXPORT void Finalize(void* data)
{
    Measure* measure = (Measure*)data;
    delete measure;
}
