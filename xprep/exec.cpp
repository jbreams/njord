#include "stdafx.h"
#include "njord.h"
/*
 Exec(cmdLine, wait, hide, batch, capture)
*/

struct PipeReaderInfo {
	HANDLE pipeToRead;
	LPBYTE bufferToUse;
	DWORD bufferSize;
	DWORD used;
	HANDLE stopEvent;
};

DWORD APIENTRY ExecPipeReader(LPVOID param)
{
	struct PipeReaderInfo * pri = (struct PipeReaderInfo*)param;
	pri->bufferToUse = (LPBYTE)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 4096);
	pri->bufferSize = 4096;
	pri->used = 0;
	while(WaitForSingleObject(pri->stopEvent, 0) == WAIT_TIMEOUT)
	{
		DWORD toRead = 0;
		PeekNamedPipe(pri->pipeToRead, NULL,0, NULL, &toRead, NULL);
		if(pri->used + toRead >= pri->bufferSize)
		{
			pri->bufferSize += 1024;
			pri->bufferToUse = (LPBYTE)HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, pri->bufferToUse, pri->bufferSize);
		}
		if(ReadFile(pri->pipeToRead, (LPVOID)pri->bufferToUse, toRead, &toRead, NULL))
			pri->used += toRead;
	}
	return 0;
}

JSBool xprep_js_exec(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	JSString * jsCmdLine = NULL;
	JSBool jsBatch = FALSE, jsWait = TRUE, jsHide = TRUE, jsCapture = TRUE;

	if(!JS_ConvertArguments(cx, argc, argv, "S /b b b b", &jsCmdLine, &jsWait, &jsHide, &jsBatch, &jsCapture))
	{
		JS_ReportError(cx, "Error parsing arguments in js_exec");
		return JS_FALSE;
	}

	if(JS_GetStringLength(jsCmdLine) == 0)
	{
		JS_ReportError(cx, "exec called with a blank commandline.");
		return JS_FALSE;
	}

	LPWSTR appName = NULL;
	HANDLE stdOutRead = NULL, stdOutWrite = NULL, stdErrorRead = NULL, stdErrorWrite = NULL;
	PROCESS_INFORMATION pi;
	STARTUPINFO si;
	SECURITY_ATTRIBUTES * sa = NULL;
	struct PipeReaderInfo * outReader = NULL, *errReader = NULL;

	memset(&si, 0, sizeof(si));
	memset(&pi, 0, sizeof(pi));

	if(jsCapture == TRUE && jsWait == TRUE)
	{
		sa = (SECURITY_ATTRIBUTES*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(SECURITY_ATTRIBUTES));
		sa->bInheritHandle = TRUE;
		sa->nLength = sizeof(*sa);

		CreatePipe(&stdOutRead, &stdOutWrite, sa, 0);
		CreatePipe(&stdErrorRead, &stdErrorWrite, sa, 0);
		si.hStdError = stdErrorWrite;
		si.hStdOutput = stdOutWrite;
		si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
		si.dwFlags |= STARTF_USESTDHANDLES;

		outReader = (struct PipeReaderInfo*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(struct PipeReaderInfo));
		errReader = (struct PipeReaderInfo*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(struct PipeReaderInfo));
		outReader->stopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		errReader->stopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	}

	if(jsHide == TRUE)
	{
		si.dwFlags |= STARTF_USESHOWWINDOW;
		si.wShowWindow = SW_HIDE;
	}

	if(jsBatch == TRUE)
		appName = TEXT("cmd.exe");


	if(!CreateProcessW(appName, (LPWSTR)JS_GetStringChars(jsCmdLine), sa, NULL, TRUE, 0, NULL, NULL, &si, &pi))
	{
		*rval = JS_FALSE;
		goto functionEnd;
	}

	if(jsWait == TRUE)
	{
		WaitForSingleObject(pi.hProcess, INFINITE);
		HANDLE th1 = CreateThread(NULL, 0, ExecPipeReader, outReader, 0, NULL);
		HANDLE th2 = CreateThread(NULL, 0, ExecPipeReader, errReader, 0, NULL);
		CloseHandle(th1);
		CloseHandle(th2);
	}
	else
	{
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
		*rval = JS_TRUE;
		return JS_TRUE;
	}

	JSString * stdOutString = NULL, * stdErrString = NULL;
	if(jsCapture == TRUE)
	{
		SetEvent(outReader->stopEvent);
		SetEvent(errReader->stopEvent);
		if(IsTextUnicode(outReader->bufferToUse, outReader->used, NULL))
		{
			stdOutString = JS_NewUCStringCopyN(cx, (jschar*)outReader->bufferToUse, outReader->used);
			stdErrString = JS_NewUCStringCopyN(cx, (jschar*)errReader->bufferToUse, errReader->used);
		}
		else
		{
			stdOutString = JS_NewStringCopyN(cx, (char*)outReader->bufferToUse, outReader->used);
			stdErrString = JS_NewStringCopyN(cx, (char*)errReader->bufferToUse, errReader->used);
		}
	}
	
	DWORD exitCode = STILL_ACTIVE;
	GetExitCodeProcess(pi.hProcess, &exitCode);

	JSObject * retObj = JS_NewObject(cx, NULL, NULL, obj);
	*rval = OBJECT_TO_JSVAL(retObj);

	jsval outStringVal = JSVAL_NULL, errStringVal = JSVAL_NULL, retCodeVal;
	JS_NewNumberValue(cx, exitCode, &retCodeVal);
	JS_DefineProperty(cx, retObj, "exitCode", retCodeVal, NULL, NULL, JSPROP_READONLY | JSPROP_PERMANENT | JSPROP_ENUMERATE);
	if(jsCapture == TRUE && stdOutString && stdErrString)
	{
		outStringVal = STRING_TO_JSVAL(stdOutString);
		errStringVal = STRING_TO_JSVAL(stdErrString);
	}
	JS_DefineProperty(cx, retObj, "stdOut", outStringVal, NULL, NULL, JSPROP_READONLY | JSPROP_PERMANENT | JSPROP_ENUMERATE);
	JS_DefineProperty(cx, retObj, "stdErr", errStringVal, NULL, NULL, JSPROP_READONLY | JSPROP_PERMANENT | JSPROP_ENUMERATE);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

functionEnd:
	if(stdOutWrite != NULL)
	{
		CloseHandle(stdOutRead);
		CloseHandle(stdOutWrite);
		CloseHandle(stdErrorRead);
		CloseHandle(stdErrorWrite);
		HeapFree(GetProcessHeap(), 0, sa);

		CloseHandle(outReader->stopEvent);
		CloseHandle(errReader->stopEvent);
		HeapFree(GetProcessHeap(), 0, outReader->bufferToUse);
		HeapFree(GetProcessHeap(), 0, errReader->bufferToUse);
		HeapFree(GetProcessHeap(), 0, outReader);
		HeapFree(GetProcessHeap(), 0, errReader);
	}
	return JS_TRUE;
}

JSBool InitExec(JSContext * cx, JSObject * global)
{
	JSFunctionSpec execMethods[] = {
		{"Exec", xprep_js_exec, 1, 0, 0},
		{0, 0, 0, 0, 0 }
	};

	return JS_DefineFunctions(cx, global, execMethods);
}