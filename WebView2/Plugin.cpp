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
autoStart(true), disabled(false), visible(true), initialized(false), clickthrough(false), allowDualControl(true), allowNotifications(false), allowNewWindow(false),
webMessageToken{}
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
	measure->autoStart = RmReadInt(rm, L"AutoStart", 1) >= 1;
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

auto allowDualControlScript = LR"JS(
		if (!window.__rmAllowDualControlInjected) {
			window.__rmAllowDualControlInjected = true;

			let rm_AllowDualControl=false,
				rm_AllowDualControlOn=false,
				rm_AllowDualControlClientX=0,
				rm_AllowDualControlClientY=0;

			function rm_SetAllowDualControl(v){
				rm_AllowDualControl=!!v;
				if(!rm_AllowDualControl) rm_AllowDualControlOn=false;
			}

			document.body.onpointerdown = e => {
				if(!rm_AllowDualControl) return;
				if(e.button===0 && e.ctrlKey){
					e.preventDefault();
					e.stopImmediatePropagation();
					rm_AllowDualControlOn=true;
					rm_AllowDualControlClientX=e.clientX;
					rm_AllowDualControlClientY=e.clientY;
					try{ document.body.setPointerCapture(e.pointerId); }catch{}
				}
			};

			document.body.onpointermove = e => {
				if(!rm_AllowDualControl || !rm_AllowDualControlOn) return;
				e.preventDefault();
				RainmeterAPI.Bang(
					'[!Move '+
					Math.round(e.screenX-RainmeterAPI.ReadFormula('X',0)-rm_AllowDualControlClientX*window.devicePixelRatio)+' '+
					Math.round(e.screenY-RainmeterAPI.ReadFormula('Y',0)-rm_AllowDualControlClientY*window.devicePixelRatio)+']'
				);
			};

			document.body.onpointerup = e => {
				if(!rm_AllowDualControl) return;
				if(e.button===0){
					e.preventDefault();
					rm_AllowDualControlOn=false;
					try{ document.body.releasePointerCapture(e.pointerId); }catch{}
				}
			};

			document.body.oncontextmenu = e => {
				if(!rm_AllowDualControl) return;
				if(e.button===2 && e.ctrlKey){
					e.preventDefault();
					RainmeterAPI.Bang('[!SkinMenu]');
				}
			};
		}
	)JS";

// Inject AllowDualControl script into the WebView
void InjectAllowDualControl(Measure* measure)
{
	if (!measure->webView) return;
	// Inject script to capture page load events for drag/move and context menu
	HRESULT hr = measure->webView->ExecuteScript(allowDualControlScript,
		Callback<ICoreWebView2ExecuteScriptCompletedHandler>(
			[measure](HRESULT errorCode, LPCWSTR resultObjectAsJson) -> HRESULT
			{
				return S_OK;
			}
		).Get()
	);
	if (SUCCEEDED(hr))
	{
		measure->isAllowDualControlInjected = true;
		UpdateAllowDualControl(measure);
	}
}

// Update AllowDualControl state in the WebView
void UpdateAllowDualControl(Measure* measure)
{
	if (!measure->webView) return;

	measure->webView->ExecuteScript(
			measure->allowDualControl
			? L"rm_SetAllowDualControl(true);"
			: L"rm_SetAllowDualControl(false);",
			Callback<ICoreWebView2ExecuteScriptCompletedHandler>(
				[](HRESULT, LPCWSTR) -> HRESULT { return S_OK; }
			).Get()
		);
}

// Inject AllowDualControl script into the WebView frame
void InjectAllowDualControlFrame(Measure* measure, Frames* frame)
{
	if (!frame || !frame->name) return;
	
	// Inject script to capture page load events for drag/move and context menu
	HRESULT hr = frame->name->ExecuteScript(allowDualControlScript,
		Callback<ICoreWebView2ExecuteScriptCompletedHandler>(
			[](HRESULT, LPCWSTR) -> HRESULT { return S_OK; }
		).Get()
	);

	if (SUCCEEDED(hr))
	{
		frame->injected = true;
		UpdateAllowDualControlFrame(measure, frame);
	}
}

// Update AllowDualControl state in the WebView frame
void UpdateAllowDualControlFrame(Measure* measure, Frames* frame)
{
	if (!frame || !frame->name || !frame->injected) return;

	frame->name->ExecuteScript(
		measure->allowDualControl
		? L"rm_SetAllowDualControl(true);"
		: L"rm_SetAllowDualControl(false);",
		Callback<ICoreWebView2ExecuteScriptCompletedHandler>(
			[](HRESULT, LPCWSTR) -> HRESULT { return S_OK; }
		).Get()
	);
}

PLUGIN_EXPORT void Reload(void* data, void* rm, double* /*maxValue*/)
{
	Measure* measure = static_cast<Measure*>(data);

	// Disabled check
	measure->disabled = RmReadInt(rm, L"Disabled", 0) >= 1;
	if (measure->disabled)
	{
		return;
	}

	// Read basic configuration
	const int    newWidth = RmReadInt(rm, L"W", 800);
	const int    newHeight = RmReadInt(rm, L"H", 600);
	const int    newX = RmReadInt(rm, L"X", 0);
	const int    newY = RmReadInt(rm, L"Y", 0);
	const double newZoomFactor = RmReadFormula(rm, L"ZoomFactor", 1.0);
	const bool newVisible = RmReadInt(rm, L"Hidden", 0) <= 0;
	const bool newClickthrough = RmReadInt(rm, L"Clickthrough", 0) >= 1;
	const bool newAllowDualControl = RmReadInt(rm, L"AllowDualControl", 1) >= 1;
	const bool newAllowNotifications = RmReadInt(rm, L"AllowNotifications", 0) >= 1;
	const bool newAllowNewWindow = RmReadInt(rm, L"AllowNewWindow", 0) >= 1;

	// URL handling
	std::wstring newUrl;
	LPCWSTR urlOption = RmReadString(rm, L"Url", L"");

	if (urlOption && *urlOption)
	{
		std::wstring urlStr = urlOption;

		// Protocol present - use as-is
		if (urlStr.find(L"://") != std::wstring::npos)
		{
			newUrl = urlStr;
		}
		else
		{
			// Relative path - convert to absolute
			if (urlStr[0] != L'/' && (urlStr.length() < 2 || urlStr[1] != L':'))
			{
				if (LPCWSTR absolutePath = RmPathToAbsolute(rm, urlStr.c_str()))
				{
					urlStr = absolutePath;
				}
			}

			// Normalize slashes
			for (wchar_t& ch : urlStr)
			{
				if (ch == L'\\') ch = L'/';
			}

			// Ensure file:///
			if (urlStr.find(L"file:///") != 0)
			{
				urlStr = L"file:///" + urlStr;
			}

			newUrl = urlStr;
		}
	}

	// Action strings
	auto ReadAction = [&](LPCWSTR key) -> std::wstring
		{
			LPCWSTR value = RmReadString(rm, key, L"", FALSE);
			return (value && *value) ? value : L"";
		};

	const std::wstring newOnWebViewLoadAction = ReadAction(L"OnWebViewLoadAction");
	const std::wstring newOnWebViewFailAction = ReadAction(L"OnWebViewFailAction");
	const std::wstring newOnWebViewStopAction = ReadAction(L"OnWebViewStopAction");
	const std::wstring newOnStateChangeAction = ReadAction(L"OnStateChangeAction");
	const std::wstring newOnUrlChangeAction = ReadAction(L"OnUrlChangeAction");
	const std::wstring newOnPageLoadStartAction = ReadAction(L"OnPageLoadStartAction");
	const std::wstring newOnPageLoadingAction = ReadAction(L"OnPageLoadingAction");
	const std::wstring newOnPageDOMLoadAction = ReadAction(L"OnPageDOMLoadAction");
	const std::wstring newOnPageLoadFinishAction = ReadAction(L"OnPageLoadFinishAction");
	const std::wstring newOnPageFirstLoadAction = ReadAction(L"OnPageFirstLoadAction");
	const std::wstring newOnPageReloadAction = ReadAction(L"OnPageReloadAction");

	// Change detection
	const bool urlChanged = (newUrl != measure->url);
	const bool dimensionsChanged = (newWidth != measure->width ||
									newHeight != measure->height ||
									newX != measure->x ||
									newY != measure->y);
	const bool visibilityChanged = (newVisible != measure->visible);
	const bool clickthroughChanged = (newClickthrough != measure->clickthrough);
	const bool allowDualControlChanged = (newAllowDualControl != measure->allowDualControl);
	const bool zoomFactorChanged = (newZoomFactor != measure->zoomFactor);

	// Options
	measure->url = newUrl;
	measure->width = newWidth;
	measure->height = newHeight;
	measure->x = newX;
	measure->y = newY;
	measure->zoomFactor = newZoomFactor;
	measure->visible = newVisible;
	measure->clickthrough = newClickthrough;
	measure->allowDualControl = newAllowDualControl;
	measure->allowNotifications = newAllowNotifications;
	measure->allowNewWindow = newAllowNewWindow;

	// Actions
	measure->onWebViewLoadAction = newOnWebViewLoadAction;
	measure->onWebViewFailAction = newOnWebViewFailAction;
	measure->onWebViewStopAction = newOnWebViewStopAction;
	measure->onStateChangeAction = newOnStateChangeAction;
	measure->onUrlChangeAction = newOnUrlChangeAction;
	measure->onPageLoadStartAction = newOnPageLoadStartAction;
	measure->onPageLoadingAction = newOnPageLoadingAction;
	measure->onPageDOMLoadAction = newOnPageDOMLoadAction;
	measure->onPageLoadFinishAction = newOnPageLoadFinishAction;
	measure->onPageFirstLoadAction = newOnPageFirstLoadAction;
	measure->onPageReloadAction = newOnPageReloadAction;

	// Initialization
	if (!measure->initialized && measure->autoStart && !measure->disabled)
	{
		if (measure->isCreationInProgress)
		{
			return;
		}

		CreateWebView2(measure);
		measure->autoStart = false;
		return;
	}

	// Dynamic updates
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

	if (!allowDualControlChanged)
		return;

	// Main document
	if (!measure->isAllowDualControlInjected)
	{
		InjectAllowDualControl(measure);
	}
	else
	{
		UpdateAllowDualControl(measure);
	}

	if (measure->webViewFrames.empty())
		return;
	// Frames
	for (Frames& frame : measure->webViewFrames)
	{
		if (!frame.name)
			continue;

		if (!frame.injected)
		{
			InjectAllowDualControlFrame(measure, &frame);
		}
		else
		{
			UpdateAllowDualControlFrame(measure, &frame);
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
							// measure->callbackResult = result;
						}
					}
					return S_OK;
				}
			).Get()
		);
	}

	return measure->state;
}

PLUGIN_EXPORT LPCWSTR GetString(void* data)
{
	Measure* measure = (Measure*)data;

	// Return the callback result if available, otherwise return "0"
	if (!measure->callbackResult.empty())
	{
		return measure->callbackResult.c_str();
	}

	return L"";
}

PLUGIN_EXPORT void ExecuteBang(void* data, LPCWSTR args)
{
	Measure* measure = (Measure*)data;
	if (!measure)
		return;

	if (measure->disabled)
	{
		RmLog(measure->rm, LOG_ERROR, L"WebView2: The measure is disabled");
		return;
	}

	std::wstring command = args;

	// Parse command
	size_t spacePos = command.find(L' ');
	std::wstring action = (spacePos != std::wstring::npos) ?
		command.substr(0, spacePos) : command;
	std::wstring param = (spacePos != std::wstring::npos) ?
		command.substr(spacePos + 1) : L"";

	// WebView Commands
	if (_wcsicmp(action.c_str(), L"WebViewStart") == 0)
	{
		CreateWebView2(measure);
		return;
	}
	if (_wcsicmp(action.c_str(), L"WebViewStop") == 0)
	{
		StopWebView2(measure);
		return;
	}
	if (_wcsicmp(action.c_str(), L"WebViewRestart") == 0)
	{
		RestartWebView2(measure);
		return;
	}

	if (!measure->webView)
	{
		RmLog(measure->rm, LOG_ERROR, L"WebView2: Not running");
		return;
	}

	// Navigation Commands
	if (_wcsicmp(action.c_str(), L"Navigate") == 0)
	{
		measure->webView->Navigate(param.c_str());
	}
	else if (_wcsicmp(action.c_str(), L"Stop") == 0)
	{
		measure->webView->Stop();
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
	else if (_wcsicmp(action.c_str(), L"GoHome") == 0)
	{
		measure->webView->Navigate(measure->url.c_str());
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
			if (measure->webViewFrames.empty())
				return;
			// Frames
			for (Frames& frame : measure->webViewFrames)
			{
				if (!frame.name)
					continue;

				frame.name->ExecuteScript(
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
	else if (_wcsicmp(action.c_str(), L"OpenDevTools") == 0)
	{
		measure->webView->OpenDevToolsWindow();
	}
}

// Generic JavaScript function caller
PLUGIN_EXPORT LPCWSTR CallJS(void* data, const int argc, const WCHAR* argv[])
{
	Measure* measure = (Measure*)data;

	if (measure->disabled)
	{
		return L"";
	}

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

	if (!measure->webViewFrames.empty())
	{
		// Frames
		for (Frames& frame : measure->webViewFrames)
		{
			if (!frame.name)
				continue;

			frame.name->ExecuteScript(
				jsCode.c_str(),
				Callback<ICoreWebView2ExecuteScriptCompletedHandler>(
					[](HRESULT errorCode, LPCWSTR resultObjectAsJson) -> HRESULT
					{
						return S_OK;
					}
				).Get()
			);
		}
	}

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
	Frames* frames = (Frames*)data;
	delete frames;
	delete measure;
}
