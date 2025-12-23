// Copyright (C) 2025 nstechbytes. All rights reserved.
#pragma once

#include <Windows.h>
#include <string>
#include <vector>
#include <map>
#include <wrl.h>
#include <wil/com.h>
#include <WebView2.h>
#include <memory>

using namespace Microsoft::WRL;

// Global TypeLib for COM objects
extern wil::com_ptr<ITypeLib> g_typeLib;

struct Frames
{
    wil::com_ptr<ICoreWebView2Frame2> frame;
    bool injected = false;
};

// Measure structure containing WebView2 state
struct Measure
{
    void* rm;
    void* skin;
    HWND skinWindow;
    LPCWSTR measureName;

    std::wstring url;
    std::wstring currentUrl;
    int width;
    int height;
    int x;
    int y;
	double zoomFactor;
    bool disabled;
    bool autoStart = true;
    bool visible;
    bool initialized;
    bool clickthrough;
    bool isFirstLoad = true;
    bool allowDualControl;
    bool isAllowDualControlInjected = false;
    bool allowNotifications;
    bool allowNewWindow;

    bool isCreationInProgress = false;
    bool isStopping = false;

    std::wstring onWebViewLoadAction;
    std::wstring onWebViewFailAction;
    std::wstring onWebViewStopAction;
    std::wstring onStateChangeAction;
    std::wstring onUrlChangeAction;
    std::wstring onPageFirstLoadAction;
    std::wstring onPageLoadStartAction;
    std::wstring onPageLoadingAction;
    std::wstring onPageDOMLoadAction;
    std::wstring onPageLoadFinishAction;
    std::wstring onPageReloadAction;

    wil::com_ptr<ICoreWebView2Environment> webViewEnvironment;
    wil::com_ptr<ICoreWebView2Controller> webViewController;
    wil::com_ptr<ICoreWebView2> webView;
    std::vector<std::shared_ptr<Frames>> Measure::webViewFrames;

    EventRegistrationToken webMessageToken;

    std::wstring buffer;  // Buffer for section variable return values
    std::wstring callbackResult;  // Stores return value from OnInitialize/OnUpdate callbacks
    std::map<std::wstring, std::wstring> jsResults; // Cache for CallJS results
    int state = -1; // Integer number to show the internal state of WebView and Navigation

    Measure();
    ~Measure();

    // Member callback functions for WebView2 creation
    HRESULT CreateEnvironmentHandler(HRESULT result, ICoreWebView2Environment* env);
    HRESULT CreateControllerHandler(HRESULT result, ICoreWebView2Controller* controller);
    void Measure::SetStateAndNotify(int newState);
    HRESULT Measure::FailWebView(HRESULT hr, const wchar_t* logMessage, bool resetCreationFlag = true);
};

// WebView2 functions
void CreateWebView2(Measure* measure);
void StopWebView2(Measure* measure);
void RestartWebView2(Measure* measure);
void UpdateClickthrough(Measure* measure);
void InjectAllowDualControl(Measure* measure);
void UpdateAllowDualControl(Measure* measure);
void InjectAllowDualControlFrame(Measure* measure, Frames* frame);
void UpdateAllowDualControlFrame(Measure* measure, Frames* frame);
