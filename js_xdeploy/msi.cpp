#include "stdafx.h"
#include <msi.h>
#pragma comment(lib, "msi.lib")

JSBool msi_set_internal_ui(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	JSObject * ownerObject = NULL;
	DWORD uiLevel;

	JS_BeginRequest(cx);
	if(!JS_ConvertArguments(cx, argc, argv, "u /o", &uiLevel, &ownerObject))
	{
		JS_ReportError(cx, "Error parsing arguments in DriverPackageGetPath");
		JS_EndRequest(cx);
		return JS_FALSE;
	}
	JS_EndRequest(cx);

	HWND ownerWindow = NULL;
	if(ownerObject != NULL)
		ownerWindow = (HWND)JS_GetPrivate(cx, ownerObject);
	MsiSetInternalUI((INSTALLUILEVEL)uiLevel, &ownerWindow);
	return JS_TRUE;
}

JSBool msi_install_product(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	LPWSTR packagePath;
	JSObject * propObj;
	LPWSTR propString;
	JS_BeginRequest(cx);
	if(!JS_ConvertArguments(cx, argc, argv, "W *", &packagePath))
	{
		JS_ReportError(cx, "Error parsing arguments in DriverPackageGetPath");
		JS_EndRequest(cx);
		return JS_FALSE;
	}

	if(JSVAL_IS_STRING(argv[1]))
	{
		JSString * propStringStr = JS_ValueToString(cx, argv[1]);
		propString = (LPWSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (JS_GetStringLength(propStringStr) + 1) * sizeof(WCHAR));
		wcscpy_s(propString, JS_GetStringLength(propStringStr) + 1, (LPWSTR)JS_GetStringChars(propStringStr));
	}
	else if(JSVAL_IS_OBJECT(argv[1]))
	{
		LPWSTR outString = (LPWSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 512 * sizeof(WCHAR));
		DWORD used = 0, size = 512;
		JS_ValueToObject(cx, argv[1], &propObj);
		JSIdArray * ids = JS_Enumerate(cx, propObj);
		for(jsint i = 0; i < ids->length; i++)
		{
			jsval curPropVal;
			JS_IdToValue(cx, ids->vector[i], &curPropVal);
			JSString * curPropName = JS_ValueToString(cx, curPropVal);
			JS_AddRoot(cx, curPropName);
			JS_GetUCProperty(cx, propObj, JS_GetStringChars(curPropName), JS_GetStringLength(curPropName), &curPropVal);
			JSString * curPropValue = JS_ValueToString(cx, curPropVal);

			DWORD CurPropPairLen = JS_GetStringLength(curPropName) + JS_GetStringLength(curPropValue) + 2;
			LPWSTR CurPropPair = (LPWSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(WCHAR) * CurPropPairLen);
			wsprintf(CurPropPair, TEXT("%s=%s "), JS_GetStringChars(curPropName), JS_GetStringChars(curPropValue));
			JS_RemoveRoot(cx, curPropName);
			if(used + wcslen(CurPropPair) > size)
			{
				size += 128;
				outString = (LPWSTR)HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, outString, size * sizeof(WCHAR));
			}
			wcscat_s(outString, size - used, CurPropPair);
			used += wcslen(CurPropPair);
		}
		JS_DestroyIdArray(cx, ids);
		DWORD len = wcslen(outString);
		outString[len - 1] = 0;
	}

	jsrefcount rCount = JS_SuspendRequest(cx);
	UINT errorCode = MsiInstallProductW(packagePath, propString);
	JS_ResumeRequest(cx, rCount);
	HeapFree(GetProcessHeap(), 0, propString);
	if(errorCode == ERROR_SUCCESS)
		*rval = JSVAL_TRUE;
	else
	{
		*rval = JSVAL_FALSE;
		SetLastError(errorCode);
	}
	JS_EndRequest(cx);
	return JS_TRUE;
}

void InitMsi(JSContext * cx, JSObject * obj)
{
	struct JSFunctionSpec msifuncs[] = { 
		{ "MsiInstallProduct", msi_install_product, 2, 0, 0 },
		{ "MsiSetInternalUI", msi_set_internal_ui, 2, 0, 0 },
		{ 0 }
	};

	struct JSConstDoubleSpec msi_consts[] = {
		{ INSTALLUILEVEL_NONE, "INSTALLUILEVEL_NONE", 0, 0 },
		{ INSTALLUILEVEL_BASIC, "INSTALLUILEVEL_BASIC", 0, 0 },
		{ INSTALLUILEVEL_REDUCED, "INSTALLUILEVEL_REDUCED", 0, 0 },
		{ INSTALLUILEVEL_FULL, "INSTALLUILEVEL_FULL", 0, 0 },
		{ 0 }
	};

	JS_DefineFunctions(cx, obj, msifuncs);
	JS_DefineConstDoubles(cx, obj, msi_consts);
}