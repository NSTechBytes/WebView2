// Copyright (C) 2025 nstechbytes. All rights reserved.
#include "Plugin.h"
#include "HostObjectRmAPI.h"
#include "../API/RainmeterAPI.h"
#include <WebView2EnvironmentOptions.h>

std::wstring NormalizeUri(const std::wstring& uri)
{
	const std::wstring scheme_sep = L"://";
	auto scheme_pos = uri.find(scheme_sep);
	if (scheme_pos == std::wstring::npos)
		return uri;

	const std::wstring scheme = uri.substr(0, scheme_pos);
	const size_t after_scheme = scheme_pos + scheme_sep.length();

	if (scheme == L"file")
	{
		size_t last_slash = uri.find_last_of(L'/');
		if (last_slash != std::wstring::npos)
		{
			return uri.substr(0, last_slash + 1);
		}
		return uri;
	}

	size_t path_start = uri.find(L'/', after_scheme);
	if (path_start == std::wstring::npos)
	{
		return uri + L"/";
	}

	return uri.substr(0, path_start + 1);
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

	if (measure->initialized)
	{
		RmLog(measure->rm, LOG_ERROR, L"WebView2: Already started");
		return;
	}

	if (measure->isCreationInProgress)
	{
		RmLog(measure->rm, LOG_ERROR, L"WebView2: Initialization already in progress");
		return;
	}

	measure->isCreationInProgress = true;

	// Create user data folder in TEMP directory to avoid permission issues
	wchar_t tempPath[MAX_PATH];
	GetTempPathW(MAX_PATH, tempPath);
	std::wstring userDataFolder = std::wstring(tempPath) + L"RainmeterWebView2";

	// Create the directory if it doesn't exist
	CreateDirectoryW(userDataFolder.c_str(), nullptr);

	// Add environment options
	auto environmentOptions = Microsoft::WRL::Make<CoreWebView2EnvironmentOptions>();

	// Available browser flags: https://learn.microsoft.com/en-us/microsoft-edge/webview2/concepts/webview-features-flags?tabs=win32cpp#available-webview2-browser-flags
	std::wstring browserArgs;
	browserArgs.append(L"--enable-features=");

	environmentOptions->put_AdditionalBrowserArguments(browserArgs.c_str()); // Add Flags

	Microsoft::WRL::ComPtr<ICoreWebView2EnvironmentOptions6> environmentOptions6;
	if (environmentOptions.As(&environmentOptions6) == S_OK)
	{
		environmentOptions6->put_AreBrowserExtensionsEnabled(TRUE); // Enable Extensions
	}

	// Create WebView2 environment with user data folder
	HRESULT hr = CreateCoreWebView2EnvironmentWithOptions(
		nullptr, userDataFolder.c_str(), environmentOptions.Get(),
		Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
			measure,
			&Measure::CreateEnvironmentHandler
		).Get()

	);

	if (FAILED(hr))
	{
		measure->FailWebView(hr, L"WebView2: Failed to start creation process (HRESULT: 0x%08X). Make sure WebView2 Runtime is installed.");
	}
}

// Environment creation callback
HRESULT Measure::CreateEnvironmentHandler(HRESULT result, ICoreWebView2Environment* env)
{
	if (FAILED(result))
	{
		return FailWebView(result, L"WebView2: Failed to create environment");
	}

	webViewEnvironment = env;
	
	// Create WebView2 controller with options.
	auto webViewEnvironment10 = webViewEnvironment.query<ICoreWebView2Environment10>();
	if (webViewEnvironment10)
	{
		wil::com_ptr<ICoreWebView2ControllerOptions> controllerOptions;
		webViewEnvironment10->CreateCoreWebView2ControllerOptions(&controllerOptions);
		
		// OPTIONS
		controllerOptions->put_IsInPrivateModeEnabled(FALSE); // Private/Incognito Mode
		controllerOptions->put_ProfileName(L"Rainmeter"); // Profile Name

		// Get System's language
		wchar_t osLocale[LOCALE_NAME_MAX_LENGTH] = { 0 };
		GetUserDefaultLocaleName(osLocale, LOCALE_NAME_MAX_LENGTH);
		// Set language
		auto controllerOptions2 = controllerOptions.query<ICoreWebView2ControllerOptions2>();
		if (controllerOptions2)
		{
			controllerOptions2->put_ScriptLocale(osLocale); // Script Locale
		}

		// Set Transparent Background
		auto controllerOptions3 = controllerOptions.query<ICoreWebView2ControllerOptions3>();
		if (controllerOptions3)
		{
			COREWEBVIEW2_COLOR transparentColor = { 0, 0, 0, 0 };

			controllerOptions3->put_DefaultBackgroundColor(transparentColor); // Background Color
		}

		// Allow Host Input Processing
		auto controllerOptions4 = controllerOptions.query<ICoreWebView2ControllerOptions4>();
		if (controllerOptions4)
		{
			controllerOptions4->put_AllowHostInputProcessing(FALSE); // Allow Host Input Processing
		}

		// Create Controller With Options.
		webViewEnvironment10->CreateCoreWebView2ControllerWithOptions(
			skinWindow,
			controllerOptions.get(),
			Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
				this,
				&Measure::CreateControllerHandler
			).Get()
		);
	}
	else // Create WebView Controller with no options.
	{ 
		webViewEnvironment->CreateCoreWebView2Controller(
			skinWindow,
			Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
				this,
				&Measure::CreateControllerHandler
			).Get()
		);
	}

	return S_OK;
}

// Controller creation callback
HRESULT Measure::CreateControllerHandler(HRESULT result, ICoreWebView2Controller* controller)
{
	if (FAILED(result))
	{
		return FailWebView(result, L"WebView2: Failed to create controller");
	}

	if (!controller)
	{
		return FailWebView(S_FALSE, L"WebView2: Controller is null");
	}

	// WebView is initializing
	SetStateAndNotify(0);

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

	// Set initial zoom factor
	webViewController->put_ZoomFactor(zoomFactor);

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

	wil::com_ptr<ICoreWebView2_4> webView4;
	webView->QueryInterface(IID_PPV_ARGS(&webView4));
	if (webView4) {
		webView4->add_FrameCreated(
			Microsoft::WRL::Callback<ICoreWebView2FrameCreatedEventHandler>(
				[this](ICoreWebView2* sender, ICoreWebView2FrameCreatedEventArgs* args) -> HRESULT
				{
					wil::com_ptr<ICoreWebView2Frame> frame;
					RETURN_IF_FAILED(args->get_Frame(&frame));

					wil::com_ptr<ICoreWebView2Frame2> frame2;
					RETURN_IF_FAILED(frame->QueryInterface(IID_PPV_ARGS(&frame2)));

					if (frame2) {
						// Add host object
						wil::com_ptr<HostObjectRmAPI> hostObject =
							Microsoft::WRL::Make<HostObjectRmAPI>(this, g_typeLib);

						wil::unique_variant hostObjectVariant;
						hostObject.query_to<IDispatch>(&hostObjectVariant.pdispVal);
						hostObjectVariant.vt = VT_DISPATCH;

						std::wstring origin = NormalizeUri(currentUrl);
						LPCWSTR origins = L"*"; // all-origins

						frame2->AddHostObjectToScriptWithOrigins(L"RainmeterAPI", &hostObjectVariant, 1, &origins);

						this->webViewFrames.push_back(Frames{ frame2, false });
						Frames* frameState = &this->webViewFrames.back();

						// Inject frame ancestor to nested frames to allow framing websites. (Requires http-server).
						frame2->add_NavigationStarting(
							Microsoft::WRL::Callback<ICoreWebView2FrameNavigationStartingEventHandler>(
								[origin](ICoreWebView2Frame* sender, ICoreWebView2NavigationStartingEventArgs* args) -> HRESULT
								{
									wil::com_ptr<ICoreWebView2NavigationStartingEventArgs2> navigationStartArgs;
									if (SUCCEEDED(args->QueryInterface(IID_PPV_ARGS(&navigationStartArgs))))
									{
										navigationStartArgs->put_AdditionalAllowedFrameAncestors(origin.c_str());
									}
									return S_OK;

								}
							).Get(), nullptr
						);

						// Inject AllowDualControl script to frames.
						frame2->add_DOMContentLoaded(
							Callback<ICoreWebView2FrameDOMContentLoadedEventHandler>(
								[this, frameState](ICoreWebView2Frame* sender, ICoreWebView2DOMContentLoadedEventArgs* args) -> HRESULT {

									frameState->injected = false;

									if (this->allowDualControl)
									{
										InjectAllowDualControlFrame(this, frameState);
									}
									return S_OK;
								}
							).Get(), nullptr
						);
					}

					return S_OK;
				}).Get(), nullptr
		);
	}

	// Add script to make RainmeterAPI available globally
	webView->AddScriptToExecuteOnDocumentCreated(
		L"window.RainmeterAPI = chrome.webview.hostObjects.sync.RainmeterAPI;",
		//L"chrome.webview.hostObjects.options.log = console.log.bind(console);" // This enables console debug mode.
		nullptr
	);

	// Inject frame ancestor to allow framing websites. (Requires http-server).
	webView->add_FrameNavigationStarting(
		Microsoft::WRL::Callback<ICoreWebView2NavigationStartingEventHandler>(
			[this](ICoreWebView2* sender, ICoreWebView2NavigationStartingEventArgs* args) -> HRESULT
			{
				std::wstring nUri = NormalizeUri(currentUrl);

				wil::com_ptr<ICoreWebView2NavigationStartingEventArgs2> navigationStartArgs;
				if (SUCCEEDED(args->QueryInterface(IID_PPV_ARGS(&navigationStartArgs))))
				{
					navigationStartArgs->put_AdditionalAllowedFrameAncestors(nUri.c_str());
				}
				return S_OK;
			}
		).Get(),nullptr
	);

	// Avoid browser from opening links on different windows and block not user requested popups
	webView->add_NewWindowRequested(
		Callback<ICoreWebView2NewWindowRequestedEventHandler>(
			[this](ICoreWebView2* sender, ICoreWebView2NewWindowRequestedEventArgs* args) -> HRESULT
			{
				BOOL isUserInitiated = FALSE;
				args->get_IsUserInitiated(&isUserInitiated);

				// Block scripted popup
				if (!isUserInitiated)
				{
					args->put_Handled(TRUE);
					return S_OK;
				}

				// Open in same window
				if (!allowNewWindow)
				{
					wil::unique_cotaskmem_string uri;
					if (SUCCEEDED(args->get_Uri(&uri)) && uri)
					{
						sender->Navigate(uri.get());
					}
					args->put_Handled(TRUE);
				}
				else // Open in new window
				{
					args->put_Handled(FALSE);
				}
				return S_OK;
			}
		).Get(), nullptr
	);

	// Handle permissions
	webView->add_PermissionRequested(
		Callback<ICoreWebView2PermissionRequestedEventHandler>(
			[this](ICoreWebView2* sender, ICoreWebView2PermissionRequestedEventArgs* args) -> HRESULT
			{
				COREWEBVIEW2_PERMISSION_KIND kind;
				args->get_PermissionKind(&kind);
				// Allow notifications
				if (kind == COREWEBVIEW2_PERMISSION_KIND_NOTIFICATIONS)
				{
					if (allowNotifications) // Allow
					{
						args->put_State(COREWEBVIEW2_PERMISSION_STATE_ALLOW);
					}
					else // Deny
					{
						args->put_State(COREWEBVIEW2_PERMISSION_STATE_DENY);
					}
				}

				wil::com_ptr<ICoreWebView2PermissionRequestedEventArgs3> args3;
				if (SUCCEEDED(args->QueryInterface(IID_PPV_ARGS(&args3))))
				{
					args3->put_SavesInProfile(FALSE);
				}
				return S_OK;
			}
		).Get(), nullptr
	);

	// Add NavigationStarting event to call action when navigation starts
	webView->add_NavigationStarting(
		Callback<ICoreWebView2NavigationStartingEventHandler>(
			[this](ICoreWebView2* sender, ICoreWebView2NavigationStartingEventArgs* args) -> HRESULT
			{
				// Navigation is starting
				SetStateAndNotify(100);

				if (wcslen(onPageLoadStartAction.c_str()) > 0)
				{
					if (skin)
						RmExecute(skin, onPageLoadStartAction.c_str());
				}
				return S_OK;
			}
		).Get(),nullptr
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
						callbackResult = currentUrl;
						if (wcslen(onUrlChangeAction.c_str()) > 0)
						{
							if (skin)
								RmExecute(skin, onUrlChangeAction.c_str());
						}
					}
					else
					{
						// URL did not change
						isFirstLoad = false;
					}
				}
				return S_OK;
			}
		).Get(),nullptr
	);

	// Add ContentLoading event to call action when page starts loading
	webView->add_ContentLoading(
		Callback<ICoreWebView2ContentLoadingEventHandler>(
			[this](ICoreWebView2* sender, ICoreWebView2ContentLoadingEventArgs* args) -> HRESULT
			{
				// Navigation is loading
				SetStateAndNotify(200);

				if (wcslen(onPageLoadingAction.c_str()) > 0)
				{
					if (skin)
						RmExecute(skin, onPageLoadingAction.c_str());
				}
				return S_OK;
			}
		).Get(),nullptr
	);

	// Add DOMContentLoaded event to inject AllowDualControl
	wil::com_ptr<ICoreWebView2_2> webView2;
	webView->QueryInterface(IID_PPV_ARGS(&webView2));
	if (webView2) {
		webView2->add_DOMContentLoaded(
			Callback< ICoreWebView2DOMContentLoadedEventHandler>(
				[this](ICoreWebView2* sender, ICoreWebView2DOMContentLoadedEventArgs* args) -> HRESULT
				{
					// DOM content is loaded
					SetStateAndNotify(300);

					// Inject script to capture page load events for drag/move and context menu
					isAllowDualControlInjected = false;
					if (allowDualControl)
					{
						InjectAllowDualControl(this);
					}

					if (wcslen(onPageDOMLoadAction.c_str()) > 0)
					{
						if (skin)
							RmExecute(skin, onPageDOMLoadAction.c_str());
					}
					return S_OK;
				}
			).Get(), nullptr
		);
	}

	// Add NavigationCompleted event to call OnInitialize after page loads and handle load actions
	webView->add_NavigationCompleted(
		Callback<ICoreWebView2NavigationCompletedEventHandler>(
			[this](ICoreWebView2* sender, ICoreWebView2NavigationCompletedEventArgs* args) -> HRESULT
			{
				// Navigation is complete
				SetStateAndNotify(400);

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
									//callbackResult = result;
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
		).Get(), nullptr
	);

	initialized = true;
	if (rm)
		RmLog(rm, LOG_NOTICE, L"WebView2: Initialized successfully with COM Host Objects");

	// WebView is initialized
	SetStateAndNotify(1);

	if (wcslen(onWebViewLoadAction.c_str()) > 0)
	{
		RmExecute(skin, onWebViewLoadAction.c_str());
	}

	isCreationInProgress = false;

	// Navigate to URL
	webView->Navigate(url.c_str());

	// Apply initial clickthrough state
	UpdateClickthrough(this);

	return S_OK;
}

void StopWebView2(Measure* measure)
{
	if (!measure)
		return;
	if (measure->isStopping)
		return;
	if (!measure->initialized && !measure->webView && !measure->webViewController)
	{
		RmLog(measure->rm, LOG_ERROR, L"WebView2: Already stopped");
		return;
	}

	measure->isStopping = true;
	
	measure->isCreationInProgress = false;

	// Hide (prevents flicker)
	if (measure->webViewController)
	{
		measure->webViewController->put_IsVisible(FALSE);
	}

	// Stop navigation
	if (measure->webView)
	{
		measure->webView->Stop();
	}

	// Release
	measure->webView.reset();
	measure->webViewController.reset();
	measure->webViewEnvironment.reset();

	// Reset flags
	measure->initialized = false;
	measure->isFirstLoad = true;

	// Clear url
	measure->currentUrl.clear();
	measure->callbackResult.clear();

	// WebView is stopped
	measure->SetStateAndNotify(-1);

	if (wcslen(measure->onWebViewStopAction.c_str()) > 0)
	{
		if (measure->skin)
			RmExecute(measure->skin, measure->onWebViewStopAction.c_str());
	}
	if (measure->rm)
	{
		RmLog(measure->rm, LOG_NOTICE, L"WebView2: Stopped sucessfully");
	}
	measure->isStopping = false;
}

void RestartWebView2(Measure* measure)
{
	if (!measure || !measure->skinWindow)
		return;

	if (!measure->initialized)
	{
		RmLog(measure->rm, LOG_ERROR, L"WebView2: Not running");
		return;
	}

	// Stop WebView2
	StopWebView2(measure);

	// Start WebView2
	CreateWebView2(measure);
}

void Measure::SetStateAndNotify(int newState)
{
	state = newState;

	if (!onStateChangeAction.empty() && skin)
	{
		RmExecute(skin, onStateChangeAction.c_str());
	}
}

HRESULT Measure::FailWebView(HRESULT hr, const wchar_t* logMessage, bool resetCreationFlag)
{
	if (rm && logMessage)
	{
		wchar_t errorMsg[512];
		if (hr != S_OK)
		{
			swprintf_s(errorMsg, L"%s (HRESULT: 0x%08X)", logMessage, hr);
			RmLog(rm, LOG_ERROR, errorMsg);
		}
		else
		{
			RmLog(rm, LOG_ERROR, logMessage);
		}
	}

	SetStateAndNotify(-2);

	if (!onWebViewFailAction.empty() && skin)
	{
		RmExecute(skin, onWebViewFailAction.c_str());
	}

	if (resetCreationFlag)
	{
		isCreationInProgress = false;
	}

	return hr;
}
