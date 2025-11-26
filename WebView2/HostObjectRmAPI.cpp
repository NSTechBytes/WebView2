// Copyright (C) 2025 nstechbytes. All rights reserved.

#include "HostObjectRmAPI.h"
#include "Plugin.h"

HostObjectRmAPI::HostObjectRmAPI(Measure* m, wil::com_ptr<ITypeLib> tLib)
    : measure(m), rm(m->rm), skin(m->skin), typeLib(tLib)
{
}

// Basic option reading
STDMETHODIMP HostObjectRmAPI::ReadString(BSTR option, VARIANT defaultValue, BSTR* result)
{
    if (!option || !result || !rm)
        return E_INVALIDARG;
    
    // Handle optional default value
    LPCWSTR defValue = L"";
    if (defaultValue.vt == VT_BSTR && defaultValue.bstrVal != nullptr)
    {
        defValue = defaultValue.bstrVal;
    }
    else if (defaultValue.vt != VT_ERROR && defaultValue.vt != VT_EMPTY)
    {
        // Try to convert to string if it's another type
        VARIANT converted;
        VariantInit(&converted);
        if (SUCCEEDED(VariantChangeType(&converted, &defaultValue, 0, VT_BSTR)))
        {
            defValue = converted.bstrVal;
            LPCWSTR value = RmReadString(rm, option, defValue, TRUE);
            *result = SysAllocString(value ? value : L"");
            VariantClear(&converted);
            return S_OK;
        }
    }
    
    LPCWSTR value = RmReadString(rm, option, defValue, TRUE);
    *result = SysAllocString(value ? value : L"");
    return S_OK;
}

STDMETHODIMP HostObjectRmAPI::ReadInt(BSTR option, VARIANT defaultValue, int* result)
{
    if (!option || !result || !rm)
        return E_INVALIDARG;
    
    // Handle optional default value
    int defValue = 0;
    if (defaultValue.vt == VT_I4)
    {
        defValue = defaultValue.lVal;
    }
    else if (defaultValue.vt == VT_I2)
    {
        defValue = defaultValue.iVal;
    }
    else if (defaultValue.vt != VT_ERROR && defaultValue.vt != VT_EMPTY)
    {
        // Try to convert to int
        VARIANT converted;
        VariantInit(&converted);
        if (SUCCEEDED(VariantChangeType(&converted, &defaultValue, 0, VT_I4)))
        {
            defValue = converted.lVal;
        }
        VariantClear(&converted);
    }
    
    double value = RmReadFormula(rm, option, defValue);
    *result = static_cast<int>(value);
    return S_OK;
}

STDMETHODIMP HostObjectRmAPI::ReadDouble(BSTR option, VARIANT defaultValue, double* result)
{
    if (!option || !result || !rm)
        return E_INVALIDARG;
    
    // Handle optional default value
    double defValue = 0.0;
    if (defaultValue.vt == VT_R8)
    {
        defValue = defaultValue.dblVal;
    }
    else if (defaultValue.vt == VT_R4)
    {
        defValue = defaultValue.fltVal;
    }
    else if (defaultValue.vt == VT_I4)
    {
        defValue = static_cast<double>(defaultValue.lVal);
    }
    else if (defaultValue.vt != VT_ERROR && defaultValue.vt != VT_EMPTY)
    {
        // Try to convert to double
        VARIANT converted;
        VariantInit(&converted);
        if (SUCCEEDED(VariantChangeType(&converted, &defaultValue, 0, VT_R8)))
        {
            defValue = converted.dblVal;
        }
        VariantClear(&converted);
    }
    
    *result = RmReadFormula(rm, option, defValue);
    return S_OK;
}

STDMETHODIMP HostObjectRmAPI::ReadFormula(BSTR option, VARIANT defaultValue, double* result)
{
    if (!option || !result || !rm)
        return E_INVALIDARG;
    
    // Handle optional default value
    double defValue = 0.0;
    if (defaultValue.vt == VT_R8)
    {
        defValue = defaultValue.dblVal;
    }
    else if (defaultValue.vt == VT_R4)
    {
        defValue = defaultValue.fltVal;
    }
    else if (defaultValue.vt == VT_I4)
    {
        defValue = static_cast<double>(defaultValue.lVal);
    }
    else if (defaultValue.vt != VT_ERROR && defaultValue.vt != VT_EMPTY)
    {
        // Try to convert to double
        VARIANT converted;
        VariantInit(&converted);
        if (SUCCEEDED(VariantChangeType(&converted, &defaultValue, 0, VT_R8)))
        {
            defValue = converted.dblVal;
        }
        VariantClear(&converted);
    }
    
    *result = RmReadFormula(rm, option, defValue);
    return S_OK;
}

STDMETHODIMP HostObjectRmAPI::ReadPath(BSTR option, VARIANT defaultValue, BSTR* result)
{
    if (!option || !result || !rm)
        return E_INVALIDARG;
    
    // Handle optional default value
    LPCWSTR defValue = L"";
    if (defaultValue.vt == VT_BSTR && defaultValue.bstrVal != nullptr)
    {
        defValue = defaultValue.bstrVal;
    }
    else if (defaultValue.vt != VT_ERROR && defaultValue.vt != VT_EMPTY)
    {
        // Try to convert to string
        VARIANT converted;
        VariantInit(&converted);
        if (SUCCEEDED(VariantChangeType(&converted, &defaultValue, 0, VT_BSTR)))
        {
            defValue = converted.bstrVal;
            LPCWSTR value = RmReadString(rm, option, defValue, TRUE);
            if (value)
            {
                LPCWSTR absolutePath = RmPathToAbsolute(rm, value);
                *result = SysAllocString(absolutePath ? absolutePath : value);
            }
            else
            {
                *result = SysAllocString(L"");
            }
            VariantClear(&converted);
            return S_OK;
        }
    }
    
    LPCWSTR value = RmReadString(rm, option, defValue, TRUE);
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
STDMETHODIMP HostObjectRmAPI::ReadStringFromSection(BSTR section, BSTR option, VARIANT defaultValue, BSTR* result)
{
    if (!section || !option || !result || !rm)
        return E_INVALIDARG;
    
    // Handle optional default value
    LPCWSTR defValue = L"";
    if (defaultValue.vt == VT_BSTR && defaultValue.bstrVal != nullptr)
    {
        defValue = defaultValue.bstrVal;
    }
    
    LPCWSTR value = RmReadStringFromSection(rm, section, option, defValue, TRUE);
    *result = SysAllocString(value ? value : L"");
    
    return S_OK;
}

STDMETHODIMP HostObjectRmAPI::ReadIntFromSection(BSTR section, BSTR option, VARIANT defaultValue, int* result)
{
    if (!section || !option || !result || !rm)
        return E_INVALIDARG;
    
    // Handle optional default value
    int defValue = 0;
    if (defaultValue.vt == VT_I4) defValue = defaultValue.lVal;
    else if (defaultValue.vt == VT_I2) defValue = defaultValue.iVal;
    
    double value = RmReadFormulaFromSection(rm, section, option, (double)defValue);
    *result = (int)value;
    
    return S_OK;
}

STDMETHODIMP HostObjectRmAPI::ReadDoubleFromSection(BSTR section, BSTR option, VARIANT defaultValue, double* result)
{
    if (!section || !option || !result || !rm)
        return E_INVALIDARG;
    
    // Handle optional default value
    double defValue = 0.0;
    if (defaultValue.vt == VT_R8) defValue = defaultValue.dblVal;
    else if (defaultValue.vt == VT_R4) defValue = defaultValue.fltVal;
    else if (defaultValue.vt == VT_I4) defValue = static_cast<double>(defaultValue.lVal);
    
    *result = RmReadFormulaFromSection(rm, section, option, defValue);
    return S_OK;
}

STDMETHODIMP HostObjectRmAPI::ReadFormulaFromSection(BSTR section, BSTR option, VARIANT defaultValue, double* result)
{
    if (!section || !option || !result || !rm)
        return E_INVALIDARG;
    
    // Handle optional default value
    double defValue = 0.0;
    if (defaultValue.vt == VT_R8)
    {
        defValue = defaultValue.dblVal;
    }
    else if (defaultValue.vt == VT_R4)
    {
        defValue = defaultValue.fltVal;
    }
    else if (defaultValue.vt == VT_I4)
    {
        defValue = static_cast<double>(defaultValue.lVal);
    }
    
    *result = defValue;
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

STDMETHODIMP HostObjectRmAPI::GetVariable(BSTR variableName, BSTR* result)
{
    if (!variableName || !result || !rm)
        return E_INVALIDARG;
    
    // Wrap variable name with # syntax
    std::wstring wrappedVar = L"#" + std::wstring(variableName) + L"#";
    
    LPCWSTR value = RmReplaceVariables(rm, wrappedVar.c_str());
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

STDMETHODIMP HostObjectRmAPI::Bang(BSTR command)
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
