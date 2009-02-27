#include "stdafx.h"
#include "xprep.h"

JSBool xprep_js_exec(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval *rval)
{
	JSString *jsCmdLine = NULL, *jsOutput;
	JSBool jsBatch = JS_FALSE, retval = JS_TRUE;
	LPWSTR appName = NULL;
	HANDLE stdOutRead, stdOutWrite, stdIn = GetStdHandle(STD_INPUT_HANDLE);
	PROCESS_INFORMATION pi;
	STARTUPINFO si;
	SECURITY_ATTRIBUTES sa;

	if(!JS_ConvertArguments(cx, argc, argv, "S /b", &jsCmdLine, &jsBatch))
		return JS_FALSE;

	if(JS_GetStringLength(jsCmdLine) == 0)
		return JS_FALSE;

	if(jsBatch == JS_TRUE)
		appName = TEXT("cmd.exe");

	memset(&sa, 0, sizeof(sa));
	sa.bInheritHandle = TRUE;
	sa.nLength = sizeof(sa);

	memset(&si, 0, sizeof(STARTUPINFO));
	memset(&pi, 0, sizeof(PROCESS_INFORMATION));
	si.cb = sizeof(si);

	if(!CreatePipe(&stdOutRead, &stdOutWrite, &sa, 0))
		return JS_FALSE;
	si.hStdOutput = stdOutWrite;
	si.hStdError = stdOutWrite;
	si.hStdInput = stdIn;
	si.dwFlags |= STARTF_USESTDHANDLES;

	if(!CreateProcessW(appName, (LPWSTR)JS_GetStringChars(jsCmdLine), &sa, NULL, TRUE, 0, NULL, NULL, &si, &pi))
	{
		retval = JS_FALSE;
		goto functionEnd;
	}

	DWORD used = 0, totalSize = 4096, exitCode = STILL_ACTIVE;
	LPSTR outputBuffer = (LPSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(char) * totalSize);
	do
	{
		GetExitCodeProcess(pi.hProcess, &exitCode);
		DWORD dwRead = 0;
		ReadFile(stdOutRead, outputBuffer + used, totalSize - used, &dwRead, NULL);
		used += dwRead;
		if(totalSize - used < 256)
		{
			totalSize += 1024;
			HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, outputBuffer, totalSize);
		}
		WaitForSingleObject(pi.hProcess, 5000);
	} while(exitCode == STILL_ACTIVE);
	jsOutput = JS_NewStringCopyZ(cx, outputBuffer);	
	HeapFree(GetProcessHeap(), 0, outputBuffer);

	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	*rval = STRING_TO_JSVAL(jsOutput);
functionEnd:
	CloseHandle(stdOutRead);
	CloseHandle(stdOutWrite);
	return retval;
}

JSBool xprep_js_exec_no(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	JSString * cmdLine;
	JSBool batch = JS_FALSE, hide = TRUE, wait = TRUE;
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	
	if(!JS_ConvertArguments(cx, argc, argv, "S /b b b", &cmdLine, &wait, &hide, &batch))
		return JS_FALSE;

	memset(&si, 0, sizeof(si));
	memset(&pi, 0, sizeof(pi));
	si.cb = sizeof(si);

	if(hide == TRUE)
	{
		si.dwFlags = STARTF_USESHOWWINDOW;
		si.wShowWindow = SW_HIDE;
	}

	LPWSTR appName = NULL;
	if(batch == JS_TRUE)
		appName = TEXT("cmd.exe");

	if(!CreateProcessW(appName, (LPWSTR)JS_GetStringChars(cmdLine), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
	{
		JS_ReportError(cx, "Error creating process.");
		return JS_FALSE;
	}
	if(wait == JS_TRUE)
		WaitForSingleObject(pi.hProcess, INFINITE);
	DWORD exitCode = 0;
	if(GetExitCodeProcess(pi.hProcess, &exitCode))
		JS_NewNumberValue(cx, exitCode, rval);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	return JS_TRUE;
}

JSBool InitExec(JSContext * cx, JSObject * global)
{
	JSFunctionSpec execMethods[] = {
		{"Exec", xprep_js_exec, 1, 0, 0},
		{"ExecNoOutput", xprep_js_exec_no, 1, 0, 0},
		{0, 0, 0, 0, 0 }
	};

	return JS_DefineFunctions(cx, global, execMethods);
}