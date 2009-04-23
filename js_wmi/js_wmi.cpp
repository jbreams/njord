// js_wmi.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "js_wmi.h"
#include "jsdate.h"
#include <comdef.h>
#include <wbemidl.h>
#pragma comment(lib, "wbemuuid.lib")

IWbemLocator * pLoc = NULL;

void wmi_services_cleanup(JSContext * cx, JSObject * obj);
JSClass wmiServices = {	"WbemServices",  /* name */
    JSCLASS_HAS_PRIVATE,  /* flags */
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, wmi_services_cleanup,
    JSCLASS_NO_OPTIONAL_MEMBERS
};
JSObject * wmiServicesProto = NULL;

void wmi_enum_cleanup(JSContext * cx, JSObject * obj);
JSClass wmiEnum = {	"WbemEnumerator",  /* name */
    JSCLASS_HAS_PRIVATE,  /* flags */
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, wmi_enum_cleanup,
    JSCLASS_NO_OPTIONAL_MEMBERS
};
JSObject * wmiEnumProto = NULL;

void wmi_class_cleanup(JSContext * cx, JSObject * obj);
JSBool wmi_enum_props(JSContext * cx, JSObject * obj);
JSBool wmi_getter(JSContext * cx, JSObject * obj, jsval idval, jsval * vp);
JSClass wmiClass = { "WbemClass",  /* name */
    JSCLASS_HAS_PRIVATE,  /* flags */
    JS_PropertyStub, JS_PropertyStub, wmi_getter, JS_PropertyStub,
    wmi_enum_props, JS_ResolveStub, JS_ConvertStub, wmi_class_cleanup,
    JSCLASS_NO_OPTIONAL_MEMBERS
};
JSObject * wmiClassProto;

JSBool wmi_connect(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	LPWSTR resource = NULL, user = NULL, password = NULL, locale = NULL;
	JS_BeginRequest(cx);
	if(!JS_ConvertArguments(cx, argc, argv, "W/ W W W", &resource, &user, &password, &locale))
	{
		JS_ReportError(cx, "Error parsing arguments in wmi_connect");
		JS_EndRequest(cx);
		return JS_FALSE;
	}
	
	JS_YieldRequest(cx);
	IWbemServices * pSvc = NULL;
	HRESULT result = pLoc->ConnectServer(_bstr_t(resource), (user != NULL ? (BSTR)_bstr_t(user) : 
		NULL), password == NULL ? (BSTR)_bstr_t(password) : NULL, (locale != NULL ? (BSTR)_bstr_t(locale) : NULL), 
		0, NULL, NULL, &pSvc);
	if(result != WBEM_S_NO_ERROR)
	{
		SetLastError(result);
		*rval = JSVAL_FALSE;
		JS_EndRequest(cx);
		return JS_TRUE;
	}

	if(CoSetProxyBlanket(pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE) != ERROR_SUCCESS)
	{
		JS_ReportError(cx, "Error setting security parameters for WMI query.");
		JS_EndRequest(cx);
		return FALSE;
	}

	JSObject * retSvc = JS_NewObject(cx, &wmiServices, wmiServicesProto, obj);
	*rval = OBJECT_TO_JSVAL(retSvc);
	JS_SetPrivate(cx, retSvc, pSvc);
	JS_EndRequest(cx);
	return JS_TRUE;
}

JSBool wmi_open_class(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	LPWSTR strClass = NULL;
	LONG flags = WBEM_FLAG_FORWARD_ONLY;
	BOOL fetchInstance = TRUE;
	IEnumWbemClassObject * enumObj;
	IWbemServices * pSvc = (IWbemServices*)JS_GetPrivate(cx, obj);

	JS_BeginRequest(cx);
	if(!JS_ConvertArguments(cx, argc, argv, "W/ b u", &strClass, &fetchInstance, &flags))
	{
		JS_ReportError(cx, "Error during argument processing in wmi_open_class");
		JS_EndRequest(cx);
		return JS_FALSE;
	}

	HRESULT result = 0;
	JS_YieldRequest(cx);
	if(fetchInstance)
		result = pSvc->CreateInstanceEnum(_bstr_t(strClass), flags, NULL, &enumObj);
	else
		result = pSvc->CreateClassEnum(_bstr_t(strClass), flags, NULL, &enumObj);
	if(result != WBEM_S_NO_ERROR)
	{
		SetLastError(result);
		*rval = JSVAL_FALSE;
		JS_EndRequest(cx);
		return JS_TRUE;
	}

	JSObject * retObj = JS_NewObject(cx, &wmiEnum, wmiEnumProto, obj);
	*rval = OBJECT_TO_JSVAL(retObj);
	JS_SetPrivate(cx, retObj, enumObj);
	JS_EndRequest(cx);
	return JS_TRUE;
}

JSBool wmi_exec_query(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	LPWSTR strClass = NULL;
	LONG flags = WBEM_FLAG_FORWARD_ONLY;
	IEnumWbemClassObject * enumObj;
	IWbemServices * pSvc = (IWbemServices*)JS_GetPrivate(cx, obj);
	
	JS_BeginRequest(cx);
	if(!JS_ConvertArguments(cx, argc, argv, "W/ u", &strClass, &flags))
	{
		JS_ReportError(cx, "Error during argument processing in wmi_open_class");
		JS_EndRequest(cx);
		return JS_FALSE;
	}

	JS_YieldRequest(cx);
	HRESULT result = pSvc->ExecQuery(_bstr_t(TEXT("WQL")), _bstr_t(strClass), flags, NULL, &enumObj);
	if(result != WBEM_S_NO_ERROR)
	{
		SetLastError(result);
		*rval = JSVAL_FALSE;
		JS_EndRequest(cx);
		return JS_TRUE;
	}

	JSObject * retObj = JS_NewObject(cx, &wmiEnum, wmiEnumProto, obj);
	*rval = OBJECT_TO_JSVAL(retObj);
	JS_SetPrivate(cx, retObj, enumObj);
	JS_EndRequest(cx);
	return JS_TRUE;
}

void wmi_services_cleanup(JSContext * cx, JSObject * obj)
{
	IWbemServices * pSvc = (IWbemServices*)JS_GetPrivate(cx, obj);
	if(pSvc)
		pSvc->Release();
}
JSBool wmi_next_enum(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	IEnumWbemClassObject * enumObj = (IEnumWbemClassObject*)JS_GetPrivate(cx, obj);
	IWbemClassObject * classObj;
	ULONG objCount = 0;
	HRESULT result = enumObj->Next(INFINITE, 1, &classObj, &objCount);
	if(result != WBEM_S_NO_ERROR || objCount == 0)
	{
		*rval = JSVAL_FALSE;
		return JS_TRUE;
	}

	JS_BeginRequest(cx);
	JSObject * retObj = JS_NewObject(cx, &wmiClass, wmiClassProto, obj);
	JS_SetPrivate(cx, retObj, classObj);
	*rval = OBJECT_TO_JSVAL(retObj);
	JS_EndRequest(cx);
	return JS_TRUE;
}

JSBool wmi_reset_enum(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	IEnumWbemClassObject * enumObj = (IEnumWbemClassObject*)JS_GetPrivate(cx, obj);
	enumObj->Reset();
	return JS_TRUE;
}

void wmi_enum_cleanup(JSContext * cx, JSObject * obj)
{
	IEnumWbemClassObject * enumObj = (IEnumWbemClassObject*)JS_GetPrivate(cx, obj);
	if(enumObj)
		enumObj->Release();
}

JSBool wmi_enum_props(JSContext * cx, JSObject * obj)
{
	IWbemClassObject * classObj = (IWbemClassObject*)JS_GetPrivate(cx, obj);
	if(classObj == NULL)
		return JS_FALSE;
	classObj->BeginEnumeration(0);
	_bstr_t propName;
	JS_BeginRequest(cx);
	while(classObj->Next(0, propName.GetAddress(), NULL, NULL, NULL) == WBEM_S_NO_ERROR)
		JS_DefineUCProperty(cx, obj, (jschar*)(LPWSTR)propName, propName.length(), JSVAL_VOID, wmi_getter, NULL, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT | JSPROP_SHARED);
	JS_EndRequest(cx);
	classObj->EndEnumeration();
	return JS_TRUE;
}

JSBool wmi_getter(JSContext * cx, JSObject * obj, jsval idval, jsval * vp)
{
	JS_BeginRequest(cx);
	IWbemClassObject * classObj = (IWbemClassObject*)JS_GetPrivate(cx, obj);
	JSString * name = JS_ValueToString(cx, idval);
	_variant_t outVar;
	CIMTYPE type;

	HRESULT result = classObj->Get((LPWSTR)JS_GetStringChars(name), 0, outVar.GetAddress(), &type, NULL);
	if(result != WBEM_S_NO_ERROR)
	{
		SetLastError(result);
		*vp = JSVAL_VOID;
		JS_EndRequest(cx);
		return JS_TRUE;
	}

	if(outVar.GetVARIANT().vt == VT_NULL)
	{
		*vp = JSVAL_NULL;
		JS_EndRequest(cx);
		return JS_TRUE;
	}
	switch(type)
	{
	case CIM_SINT8:
		JS_NewNumberValue(cx, (char)outVar, vp);
		break;
	case CIM_UINT8:
		JS_NewNumberValue(cx, (unsigned char)outVar, vp);
		break;
	case CIM_SINT16:
		JS_NewNumberValue(cx, (int)outVar, vp);
		break;
	case CIM_UINT16:
		JS_NewNumberValue(cx, (unsigned int)outVar, vp);
		break;
	case CIM_SINT32:
		JS_NewNumberValue(cx, (long)outVar, vp);
		break;
	case CIM_UINT32:
	case CIM_REAL32:
		JS_NewNumberValue(cx, (unsigned long)outVar, vp);
		break;
	case CIM_SINT64:
		JS_NewNumberValue(cx, (__int64)outVar, vp);
		break;
	case CIM_UINT64:
	case CIM_REAL64:
		JS_NewNumberValue(cx, (unsigned __int64)outVar, vp);
		break;
	case CIM_STRING:
		{
			_bstr_t bstrVal(outVar);
			JSString * retStr = JS_NewUCStringCopyZ(cx, (jschar*)(LPWSTR)bstrVal);
			*vp = STRING_TO_JSVAL(retStr);
		}
		break;
	case CIM_EMPTY:
		*vp = JSVAL_NULL;
		break;
	case CIM_BOOLEAN:
		*vp = ((bool)outVar) ? JSVAL_TRUE : JSVAL_FALSE;
		break;
	case CIM_DATETIME:
		{
			_bstr_t timeStr = (_bstr_t)outVar;
			int year, month, day, hour, minute, second;
			swscanf((wchar_t*)timeStr, TEXT("%4i%2i%2i%2i%2i%2i%*6i%*3i%*1c%*3i"), &year, &month, &day, &hour, &minute, &second);
			JSObject * dateObj = js_NewDateObject(cx, year, month, day, hour, minute, second);
			*vp = OBJECT_TO_JSVAL(dateObj);
		}
		break;
	default:
		*vp = JSVAL_VOID;
		break;
	}
	JS_EndRequest(cx);
	return JS_TRUE;
}

void wmi_class_cleanup(JSContext * cx, JSObject * obj)
{
	IWbemClassObject * classObj = (IWbemClassObject*)JS_GetPrivate(cx, obj);
	if(classObj)
		classObj->Release();
}

#ifdef __cplusplus
extern "C" {
#endif
BOOL __declspec(dllexport) InitExports(JSContext * cx, JSObject * global)
{
	CoInitialize(NULL);
	if(CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID*)&pLoc) != ERROR_SUCCESS)
	{
		JS_ReportError(cx, "Error creating WbemLocator");
		return FALSE;
	}

	JSFunctionSpec wmiServiceFunctions[] = {
		{ "OpenClass", wmi_open_class, 3, 0 },
		{ "ExecQuery", wmi_exec_query, 1, 0 },
		{ 0 }
	};

	JSFunctionSpec wmiEnumFunctions[] = {
		{ "Next", wmi_next_enum, 0, 0 },
		{ "Reset", wmi_reset_enum, 0, 0 },
		{ 0 }
	};

	JS_BeginRequest(cx);
	wmiServicesProto = JS_InitClass(cx, global, NULL, &wmiServices, NULL, 0, NULL, wmiServiceFunctions, NULL, NULL);
	wmiEnumProto = JS_InitClass(cx, global, NULL, &wmiEnum, NULL, 0, NULL, wmiEnumFunctions, NULL, NULL);
	wmiClassProto = JS_InitClass(cx, global, NULL, &wmiClass, NULL, 0, NULL, NULL, NULL, NULL);
	JS_DefineFunction(cx, global, "WbemConnect", wmi_connect, 4, 0);
	JS_EndRequest(cx);
	return TRUE;
}

BOOL __declspec(dllexport) CleanupExports(JSContext * cx, JSObject * global)
{
	pLoc->Release();
	return TRUE;
}
#ifdef __cplusplus
}
#endif