#include "stdafx.h"
#include "njord.h"


struct JSConstDoubleSpec win32MessageBoxTypes[] = {
	{ MB_ABORTRETRYIGNORE, "MB_ABORTRETRYIGNORE", 0, 0 },
	{ MB_CANCELTRYCONTINUE, "MB_CANCELTRYCONTINUE", 0, 0 },
	{ MB_HELP, "MB_HELP", 0, 0 },
	{ MB_OK, "MB_OK", 0, 0 },
	{ MB_OKCANCEL, "MB_OKCANCEL", 0, 0 },
	{ MB_RETRYCANCEL, "MB_RETRYCANCEL", 0, 0 },
	{ MB_YESNO, "MB_YESNO", 0, 0 },
	{ MB_YESNOCANCEL, "MB_YESNOCANCEL", 0, 0 },
	{ MB_ICONEXCLAMATION, "MB_ICONEXCLAMATION", 0, 0 },
	{ MB_ICONWARNING, "MB_ICONWARNING", 0, 0 },
	{ MB_ICONINFORMATION, "MB_ICONINFORMATION", 0, 0 },
	{ MB_ICONASTERISK, "MB_ICONASTERISK", 0, 0 },
	{ MB_ICONQUESTION, "MB_ICONQUESTION", 0, 0 },
	{ MB_ICONSTOP, "MB_ICONSTOP", 0, 0 },
	{ MB_ICONERROR, "MB_ICONERROR", 0, 0 },
	{ MB_ICONHAND, "MB_ICONHAND", 0, 0 },
};

JSBool win32_messagebox(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval *rval)
{
	JSString * caption = NULL, *title = NULL;
	UINT type = MB_OK;

	if(!JS_ConvertArguments(cx, argc, argv, "S /S c", &caption, &title, &type))
	{
		JS_ReportError(cx, "Unable to parse arguments for win32_messagebox");
		return JS_FALSE;
	}

	DWORD errorCode = 0;
	if(title != NULL)
		errorCode = MessageBoxW(NULL, JS_GetStringChars(caption), JS_GetStringChars(title), type);
	else
		errorCode = MessageBoxW(NULL, JS_GetStringChars(caption), TEXT("njord"), type);
	return JS_NewNumberValue(cx, errorCode, rval);
}

JSBool win32_getlasterror(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	return JS_NewNumberValue(cx, GetLastError(), rval);
}

JSBool win32_getlasterrormsg(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	LPWSTR buffer;
	DWORD errorCode = GetLastError();
	if(argc > 0)
		JS_ValueToECMAUint32(cx, argv[0], (uint32*)&errorCode);
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, errorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&buffer, 0, NULL);
	JSString * retStr = JS_NewUCStringCopyZ(cx, (jschar*)buffer);
	LocalFree(buffer);
	*rval = STRING_TO_JSVAL(retStr);
	return JS_TRUE;
}

JSBool win32_setenv(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	JSString * name, * value;
	JSBool local = JS_TRUE, addition = JS_TRUE;

	if(!JS_ConvertArguments(cx, argc, argv, "S S /b b", &name, &value, &addition, &local))
	{
		JS_ReportError(cx, "Error parsing arguments in win32_setenv");
		return JS_FALSE;
	}

	if(local)
	{
		*rval = (JSBool)SetEnvironmentVariable((LPWSTR)JS_GetStringChars(name), (LPWSTR)JS_GetStringChars(value));
		return JS_TRUE;
	}

	HKEY envKey;
	LPWSTR namec = (LPWSTR)JS_GetStringChars(name), valuec = (LPWSTR)JS_GetStringChars(value);
	DWORD type, size, statusCode;
	statusCode = RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment"), 0, KEY_SET_VALUE | KEY_QUERY_VALUE, &envKey);
	if(envKey == NULL)
	{
		SetLastError(statusCode);
		*rval = JS_FALSE;
		return JS_TRUE;
	}
	if(addition == TRUE && RegQueryValueEx(envKey, namec, NULL, &type, NULL, &size) == ERROR_SUCCESS)
	{
		LPWSTR regValue = (LPWSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size + sizeof(WCHAR));
		RegQueryValueEx(envKey, namec, NULL, NULL, (LPBYTE)regValue, &size);
		LPWSTR outputValue = (LPWSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size + wcslen(valuec) + sizeof(WCHAR));
		size = wsprintf(outputValue, TEXT("%s;%s"), regValue, valuec) + 1;
		statusCode = RegSetValueExW(envKey, namec, 0, type, (LPBYTE)outputValue, size);
		HeapFree(GetProcessHeap(), 0, regValue);
		HeapFree(GetProcessHeap(), 0, outputValue);
	}
	else
	{
		if(wcschr(valuec, TEXT('%')) == NULL)
			type = REG_SZ;
		else
			type = REG_EXPAND_SZ;

		statusCode = RegSetValueExW(envKey, namec, 0, type, (BYTE*)valuec, wcslen(valuec) + 1);
	}
	RegCloseKey(envKey);
	DWORD messageResult = 0;
	SendMessageTimeout(HWND_BROADCAST, WM_SETTINGCHANGE, NULL, (LPARAM)(TEXT("Environment")), SMTO_NORMAL, 0, &messageResult);

	return JS_NewNumberValue(cx, statusCode, rval);
}

JSBool win32_getenv(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	if(argc < 1)
	{
		JS_ReportError(cx, "getenv requires the name of the variable to be passed");
		return JS_FALSE;
	}
	JSString * name = JS_ValueToString(cx, argv[0]);
	DWORD size = GetEnvironmentVariable((LPWSTR)JS_GetStringChars(name), NULL, 0);
	if(size == 0)
	{
		*rval = JS_FALSE;
		return JS_TRUE;
	}
	LPWSTR valueBuffer = (LPWSTR)JS_malloc(cx, (size + 2) * sizeof(WCHAR));
	size = GetEnvironmentVariable((LPWSTR)JS_GetStringChars(name), valueBuffer, size + 1);
	if(size == 0)
	{
		JS_free(cx, valueBuffer);
		*rval = JS_FALSE;
		return JS_TRUE;
	}
	JSString * value = JS_NewUCString(cx, valueBuffer, size + 1);
	*rval = STRING_TO_JSVAL(value);
	return JS_TRUE;
}

void InitWin32s(JSContext * cx, JSObject * global)
{
	JS_DefineConstDoubles(cx, global, win32MessageBoxTypes);
	
	struct JSFunctionSpec win32s[] = {
		{ "MessageBox", win32_messagebox, 1, 0 },
		{ "GetLastError", win32_getlasterror, 0, 0 },
		{ "GetLastErrorMessage", win32_getlasterrormsg, 0, 0 },
		{ "SetEnv", win32_setenv, 2, 0 },
		{ "GetEnv", win32_getenv, 2, 0 },
		{ 0 }
	};

	JS_DefineFunctions(cx, global, win32s);
}