#pragma once

#include "resource.h"

LPWSTR LoadFile(LPTSTR fileName);

JSBool InitNativeLoad(JSContext * cx, JSObject * global);
JSBool InitExec(JSContext * cx, JSObject * global);
JSBool InitFile(JSContext * cx, JSObject * global);
void InitWin32s(JSContext * cx, JSObject * global);