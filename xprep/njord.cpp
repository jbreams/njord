// xprep.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "njord.h"

JSRuntime * rt = NULL;
JSContext * cx = NULL;
JSObject * global = NULL;

JSClass global_class = {
	"global", JSCLASS_GLOBAL_FLAGS,
	JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
	JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub
};

DWORD branches = 0;
JSBool BranchCallback(JSContext * cx, JSScript * script)
{
	if(branches++ > 5000)
	{
		JS_MaybeGC(cx);
		branches = 0;
	}
	return JS_TRUE;
}

void ErrorReporter(JSContext *cx, const char *message, JSErrorReport *report)
{
	printf(message);
}

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	if(::_tcslen(lpCmdLine) == 0)
		return 1;

	rt = JS_NewRuntime(0x4000000);
	if(rt == NULL)
		return 2;
	cx = JS_NewContext(rt, 0x1000);
	if(cx == NULL)
		return 2;

	global = JS_NewObject(cx, &global_class, NULL, NULL);
	if(global == NULL)
		return 2;

	JS_SetBranchCallback(cx, BranchCallback);
	JS_SetErrorReporter(cx, ErrorReporter);
	if(JS_InitStandardClasses(cx, global) == JS_FALSE)
		return 2;


	InitNativeLoad(cx, global);
	InitExec(cx, global);
	InitFile(cx, global);
	InitWin32s(cx, global);

	LPWSTR scriptToRun = LoadFile(lpCmdLine);
	if(scriptToRun == NULL)
		return 3;
	jsval result;
	JS_EvaluateUCScript(cx, global, scriptToRun, ::wcslen(scriptToRun), "njord", 1, &result);

	JS_DestroyContext(cx);
	JS_DestroyRuntime(rt);
	JS_ShutDown();
	return 0;
}