// Copyright (C) 2025 nstechbytes. All rights reserved.
#pragma once

#include <Windows.h>
#include <string>
#include <map>
#include <wrl.h>
#include <wil/com.h>
#include <WebView2.h>

using namespace Microsoft::WRL;

// Global TypeLib for COM objects
extern wil::com_ptr<ITypeLib> g_typeLib;

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
    bool visible;
    bool initialized;
    bool clickthrough;
    bool isCreationInProgress = false;
    bool isFirstLoad = true;
    bool allowDualControl;
    bool isAllowDualControlInjected = false;

    std::wstring onWebViewLoadAction;
    std::wstring onWebViewFailAction;
    std::wstring onPageFirstLoadAction;
    std::wstring onPageLoadStartAction;
    std::wstring onPageLoadingAction;
    std::wstring onPageLoadFinishAction;
    std::wstring onPageReloadAction;

    wil::com_ptr<ICoreWebView2Controller> webViewController;
    wil::com_ptr<ICoreWebView2> webView;
    EventRegistrationToken webMessageToken;

    std::wstring buffer;  // Buffer for section variable return values
    std::wstring callbackResult;  // Stores return value from OnInitialize/OnUpdate callbacks
    std::map<std::wstring, std::wstring> jsResults; // Cache for CallJS results

    Measure();
    ~Measure();

    // Member callback functions for WebView2 creation
    HRESULT CreateEnvironmentHandler(HRESULT result, ICoreWebView2Environment* env);
    HRESULT CreateControllerHandler(HRESULT result, ICoreWebView2Controller* controller);
};

// WebView2 functions
void CreateWebView2(Measure* measure);
void UpdateClickthrough(Measure* measure);
void InjectAllowDualControl(Measure* measure);
void UpdateAllowDualControl(Measure* measure);


