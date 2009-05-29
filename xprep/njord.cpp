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
DWORD branchLimit = 5000;
JSBool BranchCallback(JSContext * cx, JSScript * script)
{
	if(branches++ > branchLimit)
	{
		JS_MaybeGC(cx);
		branches = 0;
	}
	return JS_TRUE;
}

void ErrorReporter(JSContext *cx, const char *message, JSErrorReport *report)
{
	LPSTR messageBuf = (LPSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 4096);
	sprintf_s(messageBuf, 4096, "Fatal error in nJord script!\nError message (%u): %s\nOffending source line (%u): %s\nError token: %s\n",
		report->errorNumber, message, report->lineno, report->linebuf, report->tokenptr);
	MessageBoxA(NULL, messageBuf, "nJord Fatal Error", MB_OK);
	HeapFree(GetProcessHeap(), 0, messageBuf);
}

JSBool njord_exit(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	*rval = *argv;
	return JS_FALSE;
}

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	if(::_tcslen(lpCmdLine) == 0)
		return 1;

	DWORD runtimeSize = 0x4000000;
	DWORD stackSize = 0x2000;
	HKEY nJordSettingsKey = NULL;
	RegOpenKey(HKEY_LOCAL_MACHINE, TEXT("Software\\ReamsTronics\\nJord"), &nJordSettingsKey);
	if(nJordSettingsKey)
	{
		DWORD value;
		DWORD size = sizeof(DWORD);
		if(RegQueryValueEx(nJordSettingsKey, TEXT("RuntimeSize"), NULL, NULL, (LPBYTE)&value, &size) == ERROR_SUCCESS)
			runtimeSize = value;
		if(RegQueryValueEx(nJordSettingsKey, TEXT("StackSize"), NULL, NULL, (LPBYTE)&value, &size) == ERROR_SUCCESS)
			stackSize = value;
		if(RegQueryValueEx(nJordSettingsKey, TEXT("BranchesBeforeGC"), NULL, NULL, (LPBYTE)&value, &size) == ERROR_SUCCESS)
			branchLimit = value;

		TCHAR libPath[MAX_PATH];
		size = MAX_PATH * sizeof(TCHAR);
		if(RegQueryValueEx(nJordSettingsKey, TEXT("LibPath"), NULL, NULL, (LPBYTE)libPath, &size) == ERROR_SUCCESS)
			SetDllDirectory(libPath);
		size = MAX_PATH * sizeof(TCHAR);
		if(RegQueryValueEx(nJordSettingsKey, TEXT("WorkingPath"), NULL, NULL, (LPBYTE)libPath, &size) == ERROR_SUCCESS)
			SetCurrentDirectory(libPath);
		RegCloseKey(nJordSettingsKey);
	}

	rt = JS_NewRuntime(runtimeSize);
	if(rt == NULL)
	{
		MessageBox(NULL, TEXT("Error initializing JS runtime."), TEXT("nJord Error"), MB_OK);
		return 2;
	}
	cx = JS_NewContext(rt, stackSize);
	if(cx == NULL)
	{
		MessageBox(NULL, TEXT("Error creating JS context."), TEXT("nJord Error"), MB_OK);
		return 2;
	}

	JS_SetOptions(cx, JSOPTION_VAROBJFIX);
	global = JS_NewObject(cx, &global_class, NULL, NULL);
	if(global == NULL)
	{
		MessageBox(NULL, TEXT("Error creating global JS object."), TEXT("nJord Error"), MB_OK);
		return 2;
	}

	JS_SetBranchCallback(cx, BranchCallback);
	JS_SetErrorReporter(cx, ErrorReporter);
	if(JS_InitStandardClasses(cx, global) == JS_FALSE)
	{
		MessageBox(NULL, TEXT("Error initializing standard JS classes."), TEXT("nJord Error"), MB_OK);
		return 2;
	}

	JS_DefineFunction(cx, global, "Exit", njord_exit, 1, 0);
	InitNativeLoad(cx, global);
	InitExec(cx, global);
	InitFile(cx, global);
	InitWin32s(cx, global);
	InitFindFile(cx, global);
	InitRegistry(cx, global);

	LPWSTR scriptToRun = LoadFile(lpCmdLine);
	if(scriptToRun == NULL)
	{
		MessageBox(NULL, TEXT("Unable to load specified script. Cannot continue."), TEXT("nJord Error"), MB_OK);
		return 3;
	}
	jsval result;
	JS_EvaluateUCScript(cx, global, scriptToRun, ::wcslen(scriptToRun), "njord", 1, &result);
	HeapFree(GetProcessHeap(), 0, scriptToRun);

	JS_DestroyContext(cx);
	JS_DestroyRuntime(rt);
	JS_ShutDown();
//	Cleanup();
	return 0;
}