#include "Plugin.h"
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

// Inject JavaScript bridge code
void InjectJavaScriptBridge(Measure* measure)
{
    if (!measure || !measure->webView)
        return;
    
    // JavaScript bridge code
    const wchar_t* bridgeScript = LR"(
(function() {
    // Create Rainmeter API object
    window.rm = {
        _messageId: 0,
        _pendingCalls: {},
        
        // Internal: Send message to C++
        _sendMessage: function(method, args) {
            return new Promise((resolve, reject) => {
                const id = ++this._messageId;
                this._pendingCalls[id] = { resolve, reject };
                
                const message = {
                    type: 'rainmeter_api',
                    method: method,
                    args: args || [],
                    id: id
                };
                
                window.chrome.webview.postMessage(JSON.stringify(message));
                
                // Timeout after 5 seconds
                setTimeout(() => {
                    if (this._pendingCalls[id]) {
                        delete this._pendingCalls[id];
                        reject(new Error('Request timeout'));
                    }
                }, 5000);
            });
        },
        
        // Internal: Handle response from C++
        _handleResponse: function(response) {
            const call = this._pendingCalls[response.id];
            if (call) {
                delete this._pendingCalls[response.id];
                if (response.success) {
                    call.resolve(response.result);
                } else {
                    call.reject(new Error(response.error || 'Unknown error'));
                }
            }
        },
        
        // Read string option
        ReadString: function(option, defaultValue) {
            return this._sendMessage('ReadString', [option, defaultValue || '']);
        },
        
        // Read integer option
        ReadInt: function(option, defaultValue) {
            return this._sendMessage('ReadInt', [option, defaultValue || 0]);
        },
        
        // Read double option
        ReadDouble: function(option, defaultValue) {
            return this._sendMessage('ReadDouble', [option, defaultValue || 0.0]);
        },
        
        // Read formula option
        ReadFormula: function(option, defaultValue) {
            return this._sendMessage('ReadFormula', [option, defaultValue || 0.0]);
        },
        
        // Read path option
        ReadPath: function(option, defaultValue) {
            return this._sendMessage('ReadPath', [option, defaultValue || '']);
        },
        
        // Read string from section
        ReadStringFromSection: function(section, option, defaultValue) {
            return this._sendMessage('ReadStringFromSection', [section, option, defaultValue || '']);
        },
        
        // Read int from section
        ReadIntFromSection: function(section, option, defaultValue) {
            return this._sendMessage('ReadIntFromSection', [section, option, defaultValue || 0]);
        },
        
        // Read double from section
        ReadDoubleFromSection: function(section, option, defaultValue) {
            return this._sendMessage('ReadDoubleFromSection', [section, option, defaultValue || 0.0]);
        },
        
        // Read formula from section
        ReadFormulaFromSection: function(section, option, defaultValue) {
            return this._sendMessage('ReadFormulaFromSection', [section, option, defaultValue || 0.0]);
        },
        
        // Replace variables
        ReplaceVariables: function(text) {
            return this._sendMessage('ReplaceVariables', [text]);
        },
        
        // Path to absolute
        PathToAbsolute: function(path) {
            return this._sendMessage('PathToAbsolute', [path]);
        },
        
        // Execute bang (synchronous, no return value)
        Execute: function(command) {
            this._sendMessage('Execute', [command]);
        },
        
        // Log message
        Log: function(message, level) {
            this._sendMessage('Log', [message, level || 'Notice']);
        },
        
        // Get measure name
        get MeasureName() {
            return this._sendMessage('GetMeasureName', []);
        },
        
        // Get skin name
        get SkinName() {
            return this._sendMessage('GetSkinName', []);
        },
        
        // Get skin window handle
        get SkinWindowHandle() {
            return this._sendMessage('GetSkinWindowHandle', []);
        },
        
        // Get settings file
        get SettingsFile() {
            return this._sendMessage('GetSettingsFile', []);
        }
    };
    
    // Listen for responses from C++
    window.chrome.webview.addEventListener('message', function(event) {
        try {
            const response = JSON.parse(event.data);
            if (response.type === 'rainmeter_response') {
                window.rm._handleResponse(response);
            }
        } catch (e) {
            console.error('Failed to parse message from Rainmeter:', e);
        }
    });
    
    console.log('Rainmeter API bridge initialized');
})();
)";
    
    measure->webView->AddScriptToExecuteOnDocumentCreated(bridgeScript, nullptr);
}

// Handle web message from JavaScript
void HandleWebMessage(Measure* measure, LPCWSTR message)
{
    if (!measure || !measure->rm || !message)
        return;
    
    // Parse JSON message
    std::wstring msgStr(message);
    
    // Simple JSON parsing (looking for specific fields)
    // Format: {"type":"rainmeter_api","method":"ReadString","args":["Width","800"],"id":1}
    
    size_t typePos = msgStr.find(L"\"type\"");
    size_t methodPos = msgStr.find(L"\"method\"");
    size_t argsPos = msgStr.find(L"\"args\"");
    size_t idPos = msgStr.find(L"\"id\"");
    
    if (typePos == std::wstring::npos || methodPos == std::wstring::npos || idPos == std::wstring::npos)
        return;
    
    // Extract method name
    size_t methodStart = msgStr.find(L"\"", methodPos + 9) + 1;
    size_t methodEnd = msgStr.find(L"\"", methodStart);
    std::wstring method = msgStr.substr(methodStart, methodEnd - methodStart);
    
    // Extract ID
    size_t idStart = msgStr.find(L":", idPos) + 1;
    size_t idEnd = msgStr.find_first_of(L",}", idStart);
    int id = _wtoi(msgStr.substr(idStart, idEnd - idStart).c_str());
    
    // Extract args array
    std::vector<std::wstring> args;
    if (argsPos != std::wstring::npos)
    {
        size_t argsStart = msgStr.find(L"[", argsPos) + 1;
        size_t argsEnd = msgStr.find(L"]", argsStart);
        std::wstring argsStr = msgStr.substr(argsStart, argsEnd - argsStart);
        
        // Parse args (simple string extraction)
        size_t pos = 0;
        while (pos < argsStr.length())
        {
            size_t start = argsStr.find(L"\"", pos);
            if (start == std::wstring::npos) break;
            start++;
            size_t end = argsStr.find(L"\"", start);
            if (end == std::wstring::npos) break;
            args.push_back(argsStr.substr(start, end - start));
            pos = end + 1;
        }
    }
    
    // Process API call and build response
    std::wstring result;
    bool success = true;
    std::wstring error;
    
    try
    {
        if (method == L"ReadString")
        {
            LPCWSTR value = RmReadString(measure->rm, args.size() > 0 ? args[0].c_str() : L"", 
                                         args.size() > 1 ? args[1].c_str() : L"", TRUE);
            result = value ? value : L"";
        }
        else if (method == L"ReadInt")
        {
            int defaultVal = args.size() > 1 ? _wtoi(args[1].c_str()) : 0;
            double value = RmReadFormula(measure->rm, args.size() > 0 ? args[0].c_str() : L"", defaultVal);
            result = std::to_wstring(static_cast<int>(value));
        }
        else if (method == L"ReadDouble")
        {
            double defaultVal = args.size() > 1 ? _wtof(args[1].c_str()) : 0.0;
            double value = RmReadFormula(measure->rm, args.size() > 0 ? args[0].c_str() : L"", defaultVal);
            result = std::to_wstring(value);
        }
        else if (method == L"ReadFormula")
        {
            double defaultVal = args.size() > 1 ? _wtof(args[1].c_str()) : 0.0;
            double value = RmReadFormula(measure->rm, args.size() > 0 ? args[0].c_str() : L"", defaultVal);
            result = std::to_wstring(value);
        }
        else if (method == L"ReadPath")
        {
            LPCWSTR value = RmReadString(measure->rm, args.size() > 0 ? args[0].c_str() : L"", 
                                       args.size() > 1 ? args[1].c_str() : L"", TRUE);
            result = value ? value : L"";
        }
        else if (method == L"ReplaceVariables")
        {
            LPCWSTR value = RmReplaceVariables(measure->rm, args.size() > 0 ? args[0].c_str() : L"");
            result = value ? value : L"";
        }
        else if (method == L"PathToAbsolute")
        {
            LPCWSTR value = RmPathToAbsolute(measure->rm, args.size() > 0 ? args[0].c_str() : L"");
            result = value ? value : L"";
        }
        else if (method == L"Execute")
        {
            RmExecute(measure->skin, args.size() > 0 ? args[0].c_str() : L"");
            result = L"";
        }
        else if (method == L"Log")
        {
            int level = LOG_NOTICE;
            if (args.size() > 1)
            {
                if (args[1] == L"Error") level = LOG_ERROR;
                else if (args[1] == L"Warning") level = LOG_WARNING;
                else if (args[1] == L"Debug") level = LOG_DEBUG;
            }
            RmLog(measure->rm, level, args.size() > 0 ? args[0].c_str() : L"");
            result = L"";
        }
        else if (method == L"GetMeasureName")
        {
            result = measure->measureName ? measure->measureName : L"";
        }
        else if (method == L"GetSkinName")
        {
            LPCWSTR skinName = RmGetSkinName(measure->rm);
            result = skinName ? skinName : L"";
        }
        else if (method == L"GetSkinWindowHandle")
        {
            result = std::to_wstring(reinterpret_cast<UINT_PTR>(measure->skinWindow));
        }
        else if (method == L"GetSettingsFile")
        {
            LPCWSTR settingsFile = RmGetSettingsFile();
            result = settingsFile ? settingsFile : L"";
        }
        else
        {
            success = false;
            error = L"Unknown method: " + method;
        }
    }
    catch (...)
    {
        success = false;
        error = L"Exception occurred while processing request";
    }
    
    // Build JSON response
    std::wostringstream response;
    response << L"{\"type\":\"rainmeter_response\",\"id\":" << id << L",\"success\":" 
             << (success ? L"true" : L"false");
    
    if (success)
    {
        // Escape backslashes and quotes in result for JSON
        std::wstring escapedResult = result;
        size_t pos = 0;
        
        // First escape backslashes
        while ((pos = escapedResult.find(L"\\", pos)) != std::wstring::npos)
        {
            escapedResult.replace(pos, 1, L"\\\\");
            pos += 2;
        }
        
        // Then escape quotes
        pos = 0;
        while ((pos = escapedResult.find(L"\"", pos)) != std::wstring::npos)
        {
            escapedResult.replace(pos, 1, L"\\\"");
            pos += 2;
        }
        
        response << L",\"result\":\"" << escapedResult << L"\"";
    }
    else
    {
        // Escape error message too
        std::wstring escapedError = error;
        size_t pos = 0;
        while ((pos = escapedError.find(L"\\", pos)) != std::wstring::npos)
        {
            escapedError.replace(pos, 1, L"\\\\");
            pos += 2;
        }
        pos = 0;
        while ((pos = escapedError.find(L"\"", pos)) != std::wstring::npos)
        {
            escapedError.replace(pos, 1, L"\\\"");
            pos += 2;
        }
        response << L",\"error\":\"" << escapedError << L"\"";
    }
    
    response << L"}";
    
    // Send response back to JavaScript
    if (measure->webView)
    {
        measure->webView->PostWebMessageAsString(response.str().c_str());
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
                            
                            // Set up web message handler
                            measure->webView->add_WebMessageReceived(
                                Callback<ICoreWebView2WebMessageReceivedEventHandler>(
                                    [measure](ICoreWebView2* sender, ICoreWebView2WebMessageReceivedEventArgs* args) -> HRESULT
                                    {
                                        wil::unique_cotaskmem_string message;
                                        args->TryGetWebMessageAsString(&message);
                                        if (message)
                                        {
                                            HandleWebMessage(measure, message.get());
                                        }
                                        return S_OK;
                                    }
                                ).Get(),
                                nullptr
                            );
                            
                            // Inject JavaScript bridge
                            InjectJavaScriptBridge(measure);
                            
                            // Navigate to URL
                            if (!measure->url.empty())
                            {
                                measure->webView->Navigate(measure->url.c_str());
                            }
                            
                            measure->initialized = true;
                            
                            if (measure->rm)
                                RmLog(measure->rm, LOG_NOTICE, L"WebView2: Initialized successfully with JavaScript bridge");
                            
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
