// Copyright (C) 2024 WebView2 Plugin. All rights reserved.

#pragma once
#include "HostObject_h.h"
#include "../API/RainmeterAPI.h"
#include <wrl.h>
#include <wil/com.h>

struct Measure;

class HostObjectRmAPI : public Microsoft::WRL::RuntimeClass<
    Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>,
    IHostObjectRmAPI, IDispatch>
{
public:
    HostObjectRmAPI(Measure* measure, wil::com_ptr<ITypeLib> typeLib);

    // IHostObjectRmAPI methods - Basic option reading
    STDMETHODIMP ReadString(BSTR option, BSTR defaultValue, BSTR* result) override;
    STDMETHODIMP ReadInt(BSTR option, int defaultValue, int* result) override;
    STDMETHODIMP ReadDouble(BSTR option, double defaultValue, double* result) override;
    STDMETHODIMP ReadFormula(BSTR option, double defaultValue, double* result) override;
    STDMETHODIMP ReadPath(BSTR option, BSTR defaultValue, BSTR* result) override;
    
    // Section reading
    STDMETHODIMP ReadStringFromSection(BSTR section, BSTR option, BSTR defaultValue, BSTR* result) override;
    STDMETHODIMP ReadIntFromSection(BSTR section, BSTR option, int defaultValue, int* result) override;
    STDMETHODIMP ReadDoubleFromSection(BSTR section, BSTR option, double defaultValue, double* result) override;
    STDMETHODIMP ReadFormulaFromSection(BSTR section, BSTR option, double defaultValue, double* result) override;
    
    // Utility functions
    STDMETHODIMP ReplaceVariables(BSTR text, BSTR* result) override;
    STDMETHODIMP PathToAbsolute(BSTR path, BSTR* result) override;
    STDMETHODIMP Execute(BSTR command) override;
    STDMETHODIMP Log(BSTR message, BSTR level) override;
    
    // Properties
    STDMETHODIMP get_MeasureName(BSTR* result) override;
    STDMETHODIMP get_SkinName(BSTR* result) override;
    STDMETHODIMP get_SkinWindowHandle(BSTR* result) override;
    STDMETHODIMP get_SettingsFile(BSTR* result) override;

    // IDispatch methods
    STDMETHODIMP GetTypeInfoCount(UINT* pctinfo) override;
    STDMETHODIMP GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo** ppTInfo) override;
    STDMETHODIMP GetIDsOfNames(REFIID riid, LPOLESTR* rgszNames, UINT cNames, 
                                LCID lcid, DISPID* rgDispId) override;
    STDMETHODIMP Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags,
                        DISPPARAMS* pDispParams, VARIANT* pVarResult,
                        EXCEPINFO* pExcepInfo, UINT* puArgErr) override;

private:
    Measure* measure;
    void* rm;
    void* skin;
    wil::com_ptr<ITypeLib> typeLib;
};
