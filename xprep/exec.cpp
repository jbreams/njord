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
	BYTE left = 2;
	while(left > 0)
	{
		DWORD toRead = 0;
		PeekNamedPipe(pri->pipeToRead, NULL,0, NULL, &toRead, NULL);
		if(toRead > 0)
		{
			if(pri->used + toRead >= pri->bufferSize)
			{
				pri->bufferSize += (toRead >1024 ? toRead + 256 : 1024);
				pri->bufferToUse = (LPBYTE)HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, pri->bufferToUse, pri->bufferSize);
			}
			if(ReadFile(pri->pipeToRead, (LPVOID)pri->bufferToUse, toRead, &toRead, NULL))
				pri->used += toRead;
		}
		
		if(WaitForSingleObject(pri->stopEvent, 0) == WAIT_OBJECT_0 && left > 1)
		{
			left--;
			continue;
		}
		if(left == 1)
			left--;
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
	HANDLE stdOutRead = NULL, stdOutWrite = NULL, stdErrorRead = NULL, stdErrorWrite = NULL, threadHandles[3];
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
		si.dwFlags |= STARTF_USESTDHANDLES;
		threadHandles[0] = stdOutRead;
		threadHandles[1] = stdErrorRead;
	}

	if(jsHide == TRUE)
	{
		si.dwFlags |= STARTF_USESHOWWINDOW;
		si.wShowWindow = SW_HIDE;
	}

	if(jsBatch == TRUE)
		appName = TEXT("cmd.exe");

	if(!CreateProcessW(appName, (LPWSTR)JS_GetStringChars(jsCmdLine), NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi))
	{
		*rval = JS_FALSE;
		goto functionEnd;
	}

	if(jsWait == FALSE)
	{
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
		*rval = JS_TRUE;
		return JS_TRUE;
	}

	if(jsCapture == TRUE)
	{
		outReader = (struct PipeReaderInfo*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(struct PipeReaderInfo));
		errReader = (struct PipeReaderInfo*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(struct PipeReaderInfo));
		outReader->stopEvent = pi.hProcess;
		errReader->stopEvent = pi.hProcess;
		outReader->pipeToRead = stdOutRead;
		errReader->pipeToRead = stdErrorRead;
		threadHandles[0] = CreateThread(sa, 0, ExecPipeReader, outReader, 0, NULL);
		threadHandles[1] = CreateThread(sa, 0, ExecPipeReader, errReader, 0, NULL);
		threadHandles[2] = pi.hProcess;
		WaitForMultipleObjects(2, threadHandles, TRUE, INFINITE);
	}
	else
		WaitForSingleObject(pi.hProcess, INFINITE);

	JSString * stdOutString = NULL, * stdErrString = NULL;
	if(jsCapture == TRUE)
	{
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
		CloseHandle(threadHandles[0]);
		CloseHandle(threadHandles[1]);
		HeapFree(GetProcessHeap(), 0, sa);
		HeapFree(GetProcessHeap(), 0, outReader->bufferToUse);
		HeapFree(GetProcessHeap(), 0, errReader->bufferToUse);
		HeapFree(GetProcessHeap(), 0, outReader);
		HeapFree(GetProcessHeap(), 0, errReader);
	}
	return JS_TRUE;
}

JSBool njord_control_service(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	JSString * serviceName;
	DWORD control = 0;

	if(!JS_ConvertArguments(cx, argc, argv, "S /u", &serviceName, &control))
	{
		JS_ReportError(cx, "Error parsing arguments in control_service");
		return JS_FALSE;
	}

	SC_HANDLE scDBHandle = OpenSCManager(NULL, NULL, GENERIC_READ | GENERIC_EXECUTE);
	if(scDBHandle == NULL)
	{
		*rval = JS_FALSE;
		return JS_TRUE;
	}

	SC_HANDLE scHandle = OpenService(scDBHandle, (LPWSTR)JS_GetStringChars(serviceName), GENERIC_EXECUTE);
	if(scHandle == NULL)
	{
		*rval = JS_FALSE;
		return JS_TRUE;
	}

	SERVICE_STATUS outStatus;
	BOOL status = FALSE;
	if(control == 0)
		status = (jsval)StartService(scHandle, 0, NULL);
	else
		status = (jsval)ControlService(scHandle, control, &outStatus);
	*rval = status == TRUE ? JSVAL_TRUE : JSVAL_FALSE;
	CloseServiceHandle(scHandle);
	CloseServiceHandle(scDBHandle);
	return JS_TRUE;
}

struct JSConstDoubleSpec serviceConsts[] = {
	{ 0, "SERVICE_CONTROL_START", 0, 0 },
	{ SERVICE_CONTROL_CONTINUE, "SERVICE_CONTROL_CONTINUE", 0, 0 },
	{ SERVICE_CONTROL_PAUSE, "SERVICE_CONTROL_PAUSE", 0, 0 },
	{ SERVICE_CONTROL_STOP, "SERVICE_CONTROL_STOP", 0, 0 },
	{ 0 },
};

JSBool InitExec(JSContext * cx, JSObject * global)
{
	JS_DefineConstDoubles(cx, global, serviceConsts);

	JSFunctionSpec execMethods[] = {
		{"Exec", xprep_js_exec, 1, 0, 0},
		{"ControlService", njord_control_service, 1, 0, 0 },
		{0, 0, 0, 0, 0 }
	};

	return JS_DefineFunctions(cx, global, execMethods);
}