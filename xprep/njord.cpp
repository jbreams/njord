/*
 * Copyright 2009 Drew University
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); 
 * you may not use this file except in compliance with the License. 
 * You may obtain a copy of the License at 
 *
 * 		http://www.apache.org/licenses/LICENSE-2.0
 *	
 * Unless required by applicable law or agreed to in writing, software 
 * distributed under the License is distributed on an "AS IS" BASIS, 
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
 * See the License for the specific language governing permissions and 
 * limitations under the License. 
 */

#include "stdafx.h"
#include "njord.h"
#include "Tlhelp32.h"

JSRuntime * rt = NULL;

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
	MessageBoxA(NULL, messageBuf, "nJord Fatal Error", MB_OK | MB_TOPMOST | MB_SETFOREGROUND);
	HeapFree(GetProcessHeap(), 0, messageBuf);
}

JSBool njord_exit(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	WORD exitCode = 0;
	if(argc > 1 && JSVAL_IS_NUMBER(*argv))
	{
		JS_BeginRequest(cx);
		JS_ValueToUint16(cx, *argv, (uint16*)&exitCode);
		JS_EndRequest(cx);
	}
	HANDLE hThreadSnap = INVALID_HANDLE_VALUE;
	THREADENTRY32 te32;

	hThreadSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
	if(hThreadSnap == INVALID_HANDLE_VALUE)
	{
		*rval = JSVAL_FALSE;
		return JS_TRUE;
	}
	te32.dwSize = sizeof(THREADENTRY32);
	if(!Thread32First(hThreadSnap, &te32))
	{
		CloseHandle(hThreadSnap);
		*rval = JSVAL_FALSE;
		return JS_FALSE;
	}
	DWORD targetProcess = GetCurrentProcessId();
	DWORD currentThread = GetCurrentThreadId();
	do
	{
		if(te32.th32OwnerProcessID != targetProcess || te32.th32ThreadID == currentThread)
			continue;
		HANDLE thread = OpenThread(THREAD_TERMINATE, FALSE, te32.th32ThreadID);
		if(thread)
		{
			TerminateThread(thread, exitCode);
			CloseHandle(thread);
		}
	} while(Thread32Next(hThreadSnap, &te32));
	CloseHandle(hThreadSnap);

	JSContext *iter = NULL;
	JS_ContextIterator(rt, &iter);
	while(iter != NULL)
	{
		JSContext * toDestroy = iter;
		JS_ContextIterator(rt, &iter);
		if(toDestroy != cx)
			JS_DestroyContextNoGC(toDestroy); 
	}
	JS_GC(cx);
	JS_DestroyContext(cx);
	JS_DestroyRuntime(rt);
	JS_ShutDown();

	TerminateProcess(GetCurrentProcess(), exitCode);
	return JS_FALSE;
}

JSBool parse_args(JSContext * cx, JSObject * obj, LPWSTR lpCmdLine, LPWSTR * scriptFile)
{
	LPWSTR cmdLineCopy = _wcsdup(lpCmdLine);
	DWORD i = 0;
	BOOL found = FALSE;
	WCHAR backup = L'\0';
	DWORD attribs;
	for(i = 0; i < wcslen(cmdLineCopy) + 1; i++)
	{
		if(cmdLineCopy[i] == L' ' || cmdLineCopy[i] == L'\0')
		{
			backup = cmdLineCopy[i];
			cmdLineCopy[i] = '\0';
			attribs = GetFileAttributes(cmdLineCopy);
			if(attribs != INVALID_FILE_ATTRIBUTES && !(attribs & FILE_ATTRIBUTE_DIRECTORY))
			{
				found = TRUE;
				break;
			}
			else
				cmdLineCopy[i] = backup;
		}
	}
	if(!found)
	{
		free(cmdLineCopy);
		return JS_FALSE;
	}
	*scriptFile = cmdLineCopy;
	JS_BeginRequest(cx);
	JSObject * argv = JS_NewArrayObject(cx, 0, NULL);
	JS_DefineUCProperty(cx, obj, L"argv", wcslen(L"argv"), OBJECT_TO_JSVAL(argv), NULL, NULL, JSPROP_PERMANENT | JSPROP_READONLY | JSPROP_ENUMERATE);
	JS_EndRequest(cx);
	cmdLineCopy += i + 1;
	if(*(cmdLineCopy + 1) == L'\0')
		return JS_TRUE;
	DWORD start = 0;
	DWORD curArg = 0;
	JS_BeginRequest(cx);
	for(i = 0; i < (wcslen(cmdLineCopy) + 1); i++)
	{
		if(cmdLineCopy[i] == L' ' || cmdLineCopy[i] == L'\0')
		{
			JSString * newString = JS_NewUCStringCopyN(cx, (jschar*)(cmdLineCopy + start), i - start);
			JS_DefineElement(cx, argv, curArg++, STRING_TO_JSVAL(newString), NULL, NULL, JSPROP_PERMANENT | JSPROP_READONLY | JSPROP_ENUMERATE);
			start = i + 1;
		}
	}
	JS_EndRequest(cx);
	return JS_TRUE;
}

BOOL SetupSigningFromFile(LPWSTR certPath);

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	if(::_tcslen(lpCmdLine) == 0)
		return 1;

	JSContext * cx = NULL;
	JSObject * global = NULL;

	DWORD runtimeSize = 0x4000000;
	DWORD stackSize = 0x2000;
	HKEY nJordSettingsKey = NULL;
	RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("Software\\nJord"),0, KEY_QUERY_VALUE, &nJordSettingsKey);
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

	if(!StartupSigning())
	{
		MessageBox(NULL, TEXT("There was an error initializing code signing. Cannot continue!"), TEXT("nJord Platform"), MB_OK);
		return GetLastError();
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
	LPWSTR scriptFileName = NULL;
	if(parse_args(cx, global, lpCmdLine, &scriptFileName))
	{
		LPWSTR scriptToRun = LoadFile(scriptFileName);
		if(scriptToRun == NULL)
		{
			MessageBox(NULL, TEXT("Unable to load specified script. Cannot continue."), TEXT("nJord Error"), MB_OK);
			return 3;
		}
		jsval result;
		JS_EvaluateUCScript(cx, global, scriptToRun, ::wcslen(scriptToRun), "nJord", 1, &result);
		HeapFree(GetProcessHeap(), 0, scriptToRun);
	}

	JS_DestroyContext(cx);
	JS_DestroyRuntime(rt);
	JS_ShutDown();
	ShutdownSigning();
//	Cleanup();
	return 0;
}