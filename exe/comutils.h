#pragma once

#include <string>
#include <atlbase.h>

class ComUtils {
public:
    static bool InvokeMethod(CComPtr<IDispatch> & pDisp, const wchar_t * name, const char * argument); 
    static bool InvokeMethod(CComPtr<IDispatch> & pDisp, const wchar_t * name, int arg1, int arg2);
    static CComPtr<IDispatch> PropertyGetIDispatch(CComPtr<IDispatch> & pDisp, const wchar_t * name);
    static int PropertyGetInt(CComPtr<IDispatch> & pDisp, const wchar_t * name);
    static std::wstring PropertyGetBStr(CComPtr<IDispatch> & pDisp, const wchar_t * name);
};
