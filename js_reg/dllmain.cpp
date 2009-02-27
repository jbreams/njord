// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"
#include "jsapi.h"

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

extern struct JSConstDoubleSpec regConsts[];
JSBool xprep_js_set_value(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval *rval);
JSBool xprep_js_delete_value(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval);
JSBool xprep_js_create_key(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval);
JSBool xprep_js_delete_key(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval);
JSBool xprep_js_enum_keys(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval);
JSBool xprep_js_enum_values(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval);
JSBool xprep_js_query_value(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval);
#ifdef __cplusplus
extern "C" {
#endif
BOOL __declspec(dllexport) InitExports(JSContext * cx, JSObject * global)
{

	JS_DefineConstDoubles(cx, global, (JSConstDoubleSpec*)regConsts);
	JS_DefineFunction(cx, global, "RegSetValue", xprep_js_set_value, 5, 0);
	JS_DefineFunction(cx, global, "RegDeleteValue", xprep_js_delete_value, 3, 0);
	JS_DefineFunction(cx, global, "RegCreateKey", xprep_js_create_key, 2, 0);
	JS_DefineFunction(cx, global, "RegDeleteKey", xprep_js_delete_key, 2, 0);
	JS_DefineFunction(cx, global, "RegEnumKeys", xprep_js_enum_keys, 2, 0);
	JS_DefineFunction(cx, global, "RegEnumValues", xprep_js_enum_values, 2, 0);
	JS_DefineFunction(cx, global, "RegQueryValue", xprep_js_query_value, 3, 0);
	return TRUE;
}

BOOL __declspec(dllexport) CleanupExports(JSContext * cx, JSObject * global)
{
	return TRUE;
}
#ifdef __cplusplus
}
#endif