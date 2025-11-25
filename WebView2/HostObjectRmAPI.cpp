// Copyright (C) 2024 WebView2 Plugin. All rights reserved.

#include "HostObjectRmAPI.h"
#include "Plugin.h"

HostObjectRmAPI::HostObjectRmAPI(Measure* m, wil::com_ptr<ITypeLib> tLib)
    : measure(m), rm(m->rm), skin(m->skin), typeLib(tLib)
{
}

// Basic option reading
STDMETHODIMP HostObjectRmAPI::ReadString(BSTR option, BSTR defaultValue, BSTR* result)
{
    if (!option || !result || !rm)
        return E_INVALIDARG;
    
    LPCWSTR value = RmReadString(rm, option, defaultValue ? defaultValue : L"", TRUE);
    *result = SysAllocString(value ? value : L"");
    return S_OK;
}

STDMETHODIMP HostObjectRmAPI::ReadInt(BSTR option, int defaultValue, int* result)
{
    if (!option || !result || !rm)
        return E_INVALIDARG;
    
    double value = RmReadFormula(rm, option, defaultValue);
    *result = static_cast<int>(value);
    return S_OK;
}

STDMETHODIMP HostObjectRmAPI::ReadDouble(BSTR option, double defaultValue, double* result)
{
    if (!option || !result || !rm)
        return E_INVALIDARG;
    
    *result = RmReadFormula(rm, option, defaultValue);
    return S_OK;
}

STDMETHODIMP HostObjectRmAPI::ReadFormula(BSTR option, double defaultValue, double* result)
{
    if (!option || !result || !rm)
        return E_INVALIDARG;
    
    *result = RmReadFormula(rm, option, defaultValue);
    return S_OK;
}

STDMETHODIMP HostObjectRmAPI::ReadPath(BSTR option, BSTR defaultValue, BSTR* result)
{
    if (!option || !result || !rm)
        return E_INVALIDARG;
    
    LPCWSTR value = RmReadString(rm, option, defaultValue ? defaultValue : L"", TRUE);
    if (value)
    {
        LPCWSTR absolutePath = RmPathToAbsolute(rm, value);
        *result = SysAllocString(absolutePath ? absolutePath : value);
    }
    else
    {
        *result = SysAllocString(L"");
    }
    return S_OK;
}

// Section reading
STDMETHODIMP HostObjectRmAPI::ReadStringFromSection(BSTR section, BSTR option, BSTR defaultValue, BSTR* result)
{
    if (!section || !option || !result || !rm)
        return E_INVALIDARG;
    
    // Note: Rainmeter API doesn't have direct section reading, so we'd need to implement this differently
    // For now, return empty string
    *result = SysAllocString(defaultValue ? defaultValue : L"");
    return S_OK;
}

STDMETHODIMP HostObjectRmAPI::ReadIntFromSection(BSTR section, BSTR option, int defaultValue, int* result)
{
    if (!section || !option || !result || !rm)
        return E_INVALIDARG;
    
    *result = defaultValue;
    return S_OK;
}

STDMETHODIMP HostObjectRmAPI::ReadDoubleFromSection(BSTR section, BSTR option, double defaultValue, double* result)
{
    if (!section || !option || !result || !rm)
        return E_INVALIDARG;
    
    *result = defaultValue;
    return S_OK;
}

STDMETHODIMP HostObjectRmAPI::ReadFormulaFromSection(BSTR section, BSTR option, double defaultValue, double* result)
{
    if (!section || !option || !result || !rm)
        return E_INVALIDARG;
    
    *result = defaultValue;
    return S_OK;
}

// Utility functions
STDMETHODIMP HostObjectRmAPI::ReplaceVariables(BSTR text, BSTR* result)
{
    if (!text || !result || !rm)
        return E_INVALIDARG;
    
    LPCWSTR value = RmReplaceVariables(rm, text);
    *result = SysAllocString(value ? value : L"");
    return S_OK;
}

STDMETHODIMP HostObjectRmAPI::PathToAbsolute(BSTR path, BSTR* result)
{
    if (!path || !result || !rm)
        return E_INVALIDARG;
    
    LPCWSTR value = RmPathToAbsolute(rm, path);
    *result = SysAllocString(value ? value : L"");
    return S_OK;
}

STDMETHODIMP HostObjectRmAPI::Execute(BSTR command)
{
    if (!command || !skin)
        return E_INVALIDARG;
    
    RmExecute(skin, command);
    return S_OK;
}

STDMETHODIMP HostObjectRmAPI::Log(BSTR message, BSTR level)
{
    if (!message || !rm)
        return E_INVALIDARG;
    
    int logLevel = LOG_NOTICE;
    if (level)
    {
        std::wstring levelStr(level);
        if (levelStr == L"Error") logLevel = LOG_ERROR;
        else if (levelStr == L"Warning") logLevel = LOG_WARNING;
        else if (levelStr == L"Debug") logLevel = LOG_DEBUG;
        else if (levelStr == L"Notice") logLevel = LOG_NOTICE;
    }
    
    RmLog(rm, logLevel, message);
    return S_OK;
}

// Properties
STDMETHODIMP HostObjectRmAPI::get_MeasureName(BSTR* result)
{
    if (!result || !measure)
        return E_INVALIDARG;
    
    *result = SysAllocString(measure->measureName ? measure->measureName : L"");
    return S_OK;
}

STDMETHODIMP HostObjectRmAPI::get_SkinName(BSTR* result)
{
    if (!result || !rm)
        return E_INVALIDARG;
    
    LPCWSTR skinName = RmGetSkinName(rm);
    *result = SysAllocString(skinName ? skinName : L"");
    return S_OK;
}

STDMETHODIMP HostObjectRmAPI::get_SkinWindowHandle(BSTR* result)
{
    if (!result || !measure)
        return E_INVALIDARG;
    
    std::wstring handle = std::to_wstring(reinterpret_cast<UINT_PTR>(measure->skinWindow));
    *result = SysAllocString(handle.c_str());
    return S_OK;
}

STDMETHODIMP HostObjectRmAPI::get_SettingsFile(BSTR* result)
{
    if (!result)
        return E_INVALIDARG;
    
    LPCWSTR settingsFile = RmGetSettingsFile();
    *result = SysAllocString(settingsFile ? settingsFile : L"");
    return S_OK;
}

// IDispatch implementation
STDMETHODIMP HostObjectRmAPI::GetTypeInfoCount(UINT* pctinfo)
{
    if (!pctinfo)
        return E_INVALIDARG;
    
    *pctinfo = 1;
    return S_OK;
}

STDMETHODIMP HostObjectRmAPI::GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo** ppTInfo)
{
    if (!ppTInfo)
        return E_INVALIDARG;
    
    if (iTInfo != 0 || !typeLib)
        return TYPE_E_ELEMENTNOTFOUND;
    
    return typeLib->GetTypeInfoOfGuid(__uuidof(IHostObjectRmAPI), ppTInfo);
}

STDMETHODIMP HostObjectRmAPI::GetIDsOfNames(REFIID riid, LPOLESTR* rgszNames,
                                             UINT cNames, LCID lcid, DISPID* rgDispId)
{
    wil::com_ptr<ITypeInfo> typeInfo;
    HRESULT hr = GetTypeInfo(0, lcid, &typeInfo);
    if (FAILED(hr))
        return hr;
    
    return typeInfo->GetIDsOfNames(rgszNames, cNames, rgDispId);
}

STDMETHODIMP HostObjectRmAPI::Invoke(DISPID dispIdMember, REFIID riid, LCID lcid,
                                      WORD wFlags, DISPPARAMS* pDispParams,
                                      VARIANT* pVarResult, EXCEPINFO* pExcepInfo,
                                      UINT* puArgErr)
{
    wil::com_ptr<ITypeInfo> typeInfo;
    HRESULT hr = GetTypeInfo(0, lcid, &typeInfo);
    if (FAILED(hr))
        return hr;
    
    return typeInfo->Invoke(this, dispIdMember, wFlags, pDispParams,
                            pVarResult, pExcepInfo, puArgErr);
}
