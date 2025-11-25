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
            webViewWindow(nullptr), measureName(nullptr),
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
    // Proper cleanup sequence to prevent crashes
    
    // 1. Remove event handlers first
    if (webView && webMessageToken.value != 0)
    {
        webView->remove_WebMessageReceived(webMessageToken);
        webMessageToken = {};
    }
    
    // 2. Close and release WebView2 controller
    if (webViewController)
    {
        webViewController->Close();
        webViewController.reset(); // Explicit release
    }
    
    // 3. Release WebView COM pointer
    if (webView)
    {
        webView.reset(); // Explicit release
    }
    
    // 4. Small delay to allow async cleanup
    Sleep(50);
    
    // 5. Destroy window last
    if (webViewWindow && IsWindow(webViewWindow))
    {
        DestroyWindow(webViewWindow);
        webViewWindow = nullptr;
    }
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
    LPCWSTR urlOption = RmReadString(rm, L"Url", L"");
    if (urlOption && wcslen(urlOption) > 0)
    {
        std::wstring urlStr = urlOption;
        
        // Convert file paths to file:/// URLs
        if (urlStr.find(L"://") == std::wstring::npos)
        {
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
            
            // Add file:/// prefix
            urlStr = L"file:///" + urlStr;
            
            measure->url = urlStr;
        }
        else
        {
            measure->url = urlStr;
        }
    }
    
    // Read dimensions
    measure->width = RmReadInt(rm, L"Width", 800);
    measure->height = RmReadInt(rm, L"Height", 600);
    measure->x = RmReadInt(rm, L"X", 0);
    measure->y = RmReadInt(rm, L"Y", 0);
    
    // Read visibility
    measure->visible = RmReadInt(rm, L"Visible", 1) != 0;
    
    // Create WebView2 if not already created
    if (!measure->initialized)
    {
        CreateWebView2(measure);
    }
    else
    {
        // Update existing WebView
        if (measure->webView && !measure->url.empty())
        {
            measure->webView->Navigate(measure->url.c_str());
        }
        
        // Update window position and size
        if (measure->webViewWindow)
        {
            SetWindowPos(
                measure->webViewWindow,
                nullptr,
                measure->x, measure->y,
                measure->width, measure->height,
                SWP_NOZORDER | SWP_NOACTIVATE
            );
            
            ShowWindow(measure->webViewWindow, measure->visible ? SW_SHOW : SW_HIDE);
            
            // Update WebView2 controller visibility
            if (measure->webViewController)
            {
                measure->webViewController->put_IsVisible(measure->visible ? TRUE : FALSE);
            }
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
    else if (_wcsicmp(action.c_str(), L"Show") == 0)
    {
        if (measure->webViewWindow)
        {
            ShowWindow(measure->webViewWindow, SW_SHOW);
            measure->visible = true;
            
            // Also make WebView2 controller visible
            if (measure->webViewController)
            {
                measure->webViewController->put_IsVisible(TRUE);
            }
        }
    }
    else if (_wcsicmp(action.c_str(), L"Hide") == 0)
    {
        if (measure->webViewWindow)
        {
            ShowWindow(measure->webViewWindow, SW_HIDE);
            measure->visible = false;
            
            // Also hide WebView2 controller
            if (measure->webViewController)
            {
                measure->webViewController->put_IsVisible(FALSE);
            }
        }
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
}

PLUGIN_EXPORT void Finalize(void* data)
{
    Measure* measure = (Measure*)data;
    delete measure;
}
