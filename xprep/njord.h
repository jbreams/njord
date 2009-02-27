#pragma once

#include "resource.h"


char* WcharToUtf8(WCHAR* str);
WCHAR* Utf8ToWchar(const char* str);
LPWSTR LoadFile(LPTSTR fileName);

JSBool InitVarStore(JSContext * cx, JSObject * global);
JSBool InitUIFunctions(JSContext * cx, JSObject * global);
JSBool InitNativeLoad(JSContext * cx, JSObject * global);
JSBool InitExec(JSContext * cx, JSObject * global);
JSBool InitFile(JSContext * cx, JSObject * global);
void InitWin32s(JSContext * cx, JSObject * global);