#include <windows.h>
#define JS_THREADSAFE
#define XP_WIN
#include <jsapi.h>
#include <jsstr.h>
#include <difxapi.h>

JSBool install_inf(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	LPWSTR infPath;
	DWORD flags = 0;
	BOOL needsReboot;
	JSBool preInstall = JS_FALSE;

	JS_BeginRequest(cx);
	if(!JS_ConvertArguments(cx, argc, argv, "W/ u b", &infPath, &flags, &preInstall))
	{
		JS_ReportError(cx, "Error parsing arguments in DriverPackageInstall");
		JS_EndRequest(cx);
		return JS_FALSE;
	}

	DWORD errorCode = ERROR_SUCCESS;
	JS_YieldRequest(cx);
	if(preInstall)
		errorCode = DriverPackagePreinstall(infPath, flags);
	else
		errorCode = DriverPackageInstall(infPath, flags, NULL, &needsReboot);
	*rval = errorCode == ERROR_SUCCESS ? JSVAL_TRUE : JSVAL_FALSE;
	if(errorCode != ERROR_SUCCESS)
		SetLastError(errorCode);
	JS_EndRequest(cx);
	return JS_TRUE;
}

JSBool uninstall_inf(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	LPWSTR infPath;
	DWORD flags = 0;
	BOOL needsReboot;
	JSBool preInstall = JS_FALSE;

	JS_BeginRequest(cx);
	if(!JS_ConvertArguments(cx, argc, argv, "W/ u b", &infPath, &flags, &preInstall))
	{
		JS_ReportError(cx, "Error parsing arguments in DriverPackageInstall");
		JS_EndRequest(cx);
		return JS_FALSE;
	}

	JS_YieldRequest(cx);
	DWORD errorCode = DriverPackageUninstall(infPath, flags, NULL, &needsReboot);
	*rval = errorCode == ERROR_SUCCESS ? JSVAL_TRUE : JSVAL_FALSE;
	if(errorCode != ERROR_SUCCESS)
		SetLastError(errorCode);
	JS_EndRequest(cx);
	return JS_TRUE;
}

JSBool get_inf_path(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	LPWSTR infPath;
	JS_BeginRequest(cx);
	if(!JS_ConvertArguments(cx, argc, argv, "W", &infPath))
	{
		JS_ReportError(cx, "Error parsing arguments in DriverPackageGetPath");
		JS_EndRequest(cx);
		return JS_FALSE;
	}

	DWORD bufSize = 0;
	if(DriverPackageGetPath(infPath, NULL, &bufSize) != ERROR_INSUFFICIENT_BUFFER)
	{
		*rval = JSVAL_FALSE;
		JS_EndRequest(cx);
		return JS_TRUE;
	}
	LPWSTR buffer = (LPWSTR)JS_malloc(cx, sizeof(WCHAR) * bufSize);
	JS_YieldRequest(cx);
	DWORD errorCode = DriverPackageGetPath(infPath, buffer, &bufSize);
	if(errorCode != ERROR_SUCCESS)
	{
		JS_free(cx, buffer);
		*rval = JSVAL_FALSE;
		SetLastError(errorCode);
		JS_EndRequest(cx);
		return JS_TRUE;
	}

	JSString * retStr = JS_NewUCString(cx, (jschar*)buffer, bufSize);
	*rval = STRING_TO_JSVAL(retStr);
	JS_EndRequest(cx);
	return JS_TRUE;
}

#ifdef __cplusplus
extern "C" {
#endif

BOOL __declspec(dllexport) InitExports(JSContext * cx, JSObject * global)
{
	struct JSConstDoubleSpec difx_consts[] = {
		{ DRIVER_PACKAGE_FORCE, "DRIVER_PACKAGE_FORCE", 0, 0 },
		{ DRIVER_PACKAGE_LEGACY_MODE, "DRIVER_PACKAGE_LEGACY_MODE", 0, 0 },
		{ DRIVER_PACKAGE_ONLY_IF_DEVICE_PRESENT, "DRIVER_PACKAGE_ONLY_IF_DEVICE_PRESENT", 0, 0 },
		{ DRIVER_PACKAGE_REPAIR, "DRIVER_PACKAGE_REPAIR", 0, 0 },
		{ DRIVER_PACKAGE_SILENT, "DRIVER_PACKAGE_SILENT", 0, 0 },
		{ DRIVER_PACKAGE_DELETE_FILES, "DRIVER_PACKAGE_DELETE_FILES", 0, 0 },
		{ 0 }
	};

	JSFunctionSpec difx_funcs[] = {
		{ "DriverPackageInstall", install_inf, 2, 0, 0 },
		{ "DriverPackageGetPath", get_inf_path, 1, 0, 0 },
		{ "DriverPackageUninstall", uninstall_inf, 2, 0, 0 },
		{ 0 },
	};

	JS_DefineFunctions(cx, global, difx_funcs);
	JS_DefineConstDoubles(cx, global, difx_consts);
	return TRUE;
}

BOOL __declspec(dllexport) CleanupExports(JSContext * cx, JSObject * global)
{

	return TRUE;
}

#ifdef __cplusplus
}
#endif