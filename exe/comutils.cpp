#include "comutils.h"

bool ComUtils::InvokeMethod(CComPtr<IDispatch> & pDisp, const wchar_t * name, const char * argument) {
    DISPID dispid;
    CComBSTR str(name);
    if (FAILED(pDisp->GetIDsOfNames(IID_NULL, &str, 1, LOCALE_USER_DEFAULT, &dispid)))
        return false;

    CComVariant variant_result;
    EXCEPINFO exceptInfo;

    CComBSTR bstr(argument);
    VARIANTARG variant_arg[1];
    VariantInit(&variant_arg[0]);
    variant_arg[0].vt = VT_BSTR;
    variant_arg[0].bstrVal = bstr;
    DISPPARAMS args = { variant_arg, NULL, 1, 0 };
    if (FAILED(pDisp->Invoke(dispid, IID_NULL, LOCALE_SYSTEM_DEFAULT, DISPATCH_METHOD, &args, &variant_result, &exceptInfo, NULL)))
        return false;
    return true;
}

bool ComUtils::InvokeMethod(CComPtr<IDispatch> & pDisp, const wchar_t * name, int arg1, int arg2) {
    DISPID dispid;
    CComBSTR str(name);
    if (FAILED(pDisp->GetIDsOfNames(IID_NULL, &str, 1, LOCALE_USER_DEFAULT, &dispid)))
        return false;

    CComVariant variant_result;
    EXCEPINFO exceptInfo;

    VARIANTARG variant_arg[2];
    VariantInit(&variant_arg[0]);
    VariantInit(&variant_arg[1]);
    variant_arg[0].vt = VT_INT;
    variant_arg[0].intVal = arg1;
    variant_arg[1].vt = VT_INT;
    variant_arg[1].intVal = arg2;
    DISPPARAMS args = { variant_arg, NULL, 1, 0 };
    if (FAILED(pDisp->Invoke(dispid, IID_NULL, LOCALE_SYSTEM_DEFAULT, DISPATCH_METHOD, &args, &variant_result, &exceptInfo, NULL)))
        return false;
    return true;
}

CComPtr<IDispatch> ComUtils::PropertyGetIDispatch(CComPtr<IDispatch> & pDisp, const wchar_t * name) {
    EXCEPINFO exceptInfo;
    DISPID dispid;

    CComBSTR str(name);
    if (FAILED(pDisp->GetIDsOfNames(IID_NULL, &str, 1, LOCALE_USER_DEFAULT, &dispid)))
        return CComPtr<IDispatch>();

    CComVariant variant_result;
    DISPPARAMS args = { NULL, NULL, 0, 0 };
    if (FAILED(pDisp->Invoke(dispid, IID_NULL, LOCALE_SYSTEM_DEFAULT, DISPATCH_PROPERTYGET, &args, &variant_result, &exceptInfo, NULL)))
        return CComPtr<IDispatch>();
    return variant_result.pdispVal;
}

int ComUtils::PropertyGetInt(CComPtr<IDispatch> & pDisp, const wchar_t * name) {
    EXCEPINFO exceptInfo;
    DISPID dispid;

    CComBSTR str(name);
    if (FAILED(pDisp->GetIDsOfNames(IID_NULL, &str, 1, LOCALE_USER_DEFAULT, &dispid)))
        return -1;

    CComVariant variant_result;
    DISPPARAMS args = { NULL, NULL, 0, 0 };
    if (FAILED(pDisp->Invoke(dispid, IID_NULL, LOCALE_SYSTEM_DEFAULT, DISPATCH_PROPERTYGET, &args, &variant_result, &exceptInfo, NULL)))
        return -1;
    return variant_result.iVal;
}

std::wstring ComUtils::PropertyGetBStr(CComPtr<IDispatch> & pDisp, const wchar_t * name) {
    EXCEPINFO exceptInfo;
    DISPID dispid;

    CComBSTR str(name);
    if (FAILED(pDisp->GetIDsOfNames(IID_NULL, &str, 1, LOCALE_USER_DEFAULT, &dispid)))
        return std::wstring();

    CComVariant variant_result;
    DISPPARAMS args = { NULL, NULL, 0, 0 };
    if (FAILED(pDisp->Invoke(dispid, IID_NULL, LOCALE_SYSTEM_DEFAULT, DISPATCH_PROPERTYGET, &args, &variant_result, &exceptInfo, NULL)))
        return std::wstring();
    return std::wstring(variant_result.bstrVal, SysStringLen(variant_result.bstrVal));
}
