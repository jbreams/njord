#include "stdafx.h"
#include "njord.h"
/*
 Exec(cmdLine, wait, hide, batch, capture)
*/

struct PipeReaderInfo {
	JSContext * cx;
	HANDLE pipeToRead;
	LPBYTE bufferToUse;
	DWORD bufferSize;
	DWORD used;
	DWORD toRead;
};

DWORD exec_reader_thread(LPVOID param)
{
	struct PipeReaderInfo * r = (struct PipeReaderInfo*)param;
	DWORD read = 0;
	while(ReadFile(r->pipeToRead, r->bufferToUse + r->used, r->bufferSize - r->used, &read, NULL))
	{
		r->used += read;
		if(r->bufferSize - r->used < 512)
		{
			r->bufferSize += 512;
			LPVOID reallocOk = JS_realloc(r->cx, r->bufferToUse, r->bufferSize);
			if(!reallocOk)
				return 1;
			r->bufferToUse = (LPBYTE)reallocOk;				
			memset(r->bufferToUse + r->used, 0, r->bufferSize - r->used);
		}
		read = 0;
	}
	return 0;
}

HANDLE InitReader(struct PipeReaderInfo * p, HANDLE pipeToRead, JSContext * cx)
{
	memset(p, 0, sizeof(struct PipeReaderInfo));
	p->cx = cx;
	p->pipeToRead = pipeToRead;
	p->bufferToUse = (LPBYTE)JS_malloc(cx, 4096);
	memset(p->bufferToUse, 0, 4096);
	p->bufferSize = 4096;
	HANDLE retVal = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)exec_reader_thread, (LPVOID)p, 0, NULL);
	if(retVal != NULL)
		SetThreadPriority(retVal, THREAD_PRIORITY_LOWEST);
	return retVal;
}

jsval ParseReaderOutput(struct PipeReaderInfo * p, JSContext * cx)
{
	if(*p->bufferToUse == 0)
	{
		JS_free(cx, p->bufferToUse);
		return JS_GetEmptyStringValue(cx);
	}
	if(p->bufferSize - p->used > 512)
		p->bufferToUse = (LPBYTE)JS_realloc(cx, p->bufferToUse, p->used + sizeof(jschar));
	JSString * str = NULL;
	if(IsTextUnicode(p->bufferToUse, p->used, NULL))
		str = JS_NewUCString(cx, (jschar*)p->bufferToUse, p->used / sizeof(jschar));
	else
		str = JS_NewString(cx, (char*)p->bufferToUse, p->used);
	if(str == NULL)
		return JSVAL_NULL;
	return STRING_TO_JSVAL(str);
}

JSBool xprep_js_exec(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	JSString * jsCmdLine = NULL;
	JSBool jsBatch = FALSE, jsWait = TRUE, jsHide = TRUE, jsCapture = TRUE;

	JS_BeginRequest(cx);
	if(!JS_ConvertArguments(cx, argc, argv, "S /b b b b", &jsCmdLine, &jsWait, &jsHide, &jsBatch, &jsCapture))
	{
		JS_ReportError(cx, "Error parsing arguments in js_exec");
		JS_EndRequest(cx);
		return JS_FALSE;
	}

	if(JS_GetStringLength(jsCmdLine) == 0)
	{
		JS_ReportError(cx, "exec called with a blank commandline.");
		JS_EndRequest(cx);
		return JS_FALSE;
	}

	LPWSTR appName = NULL;
	HANDLE stdOutRead = NULL, stdOutWrite = NULL, stdErrorRead = NULL, stdErrorWrite = NULL;
	PROCESS_INFORMATION pi;
	STARTUPINFO si;
	struct PipeReaderInfo readers[2];

	memset(&si, 0, sizeof(si));
	memset(&pi, 0, sizeof(pi));

	HANDLE waitHandles[2];
	memset(waitHandles, 0, sizeof(HANDLE) * 2);

	if(jsCapture == TRUE && jsWait == TRUE)
	{
		SECURITY_ATTRIBUTES sa;
		sa.bInheritHandle = TRUE;
		sa.nLength = sizeof(SECURITY_ATTRIBUTES);
		sa.lpSecurityDescriptor = NULL;
		CreatePipe(&stdOutRead, &stdOutWrite, &sa, 0);
		CreatePipe(&stdErrorRead, &stdErrorWrite, &sa, 0);
		si.hStdError = stdErrorWrite;
		si.hStdOutput = stdOutWrite;
		si.dwFlags |= STARTF_USESTDHANDLES;

		waitHandles[0] = InitReader(&readers[0], stdOutRead, cx);
		waitHandles[1] = InitReader(&readers[1], stdErrorRead, cx);
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
		*rval = JSVAL_FALSE;
		if(jsCapture)
			goto functionEnd;
		else
		{
			return JS_TRUE;
			JS_EndRequest(cx);
		}
	}

	if(jsWait == FALSE)
	{
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
		*rval = JSVAL_TRUE;
		JS_EndRequest(cx);
		return JS_TRUE;
	}

	jsrefcount rCount = JS_SuspendRequest(cx);
	if(WaitForSingleObject(pi.hProcess, INFINITE) != WAIT_OBJECT_0)
	{
		JS_ResumeRequest(cx, rCount);
		*rval = JSVAL_FALSE;
		if(!jsCapture)
		{
			JS_EndRequest(cx);
			return JS_TRUE;
		}
		else
			goto functionEnd;
	}
	JS_ResumeRequest(cx, rCount);

	if(!jsCapture)
	{
		DWORD exitCode;
		GetExitCodeProcess(pi.hProcess, &exitCode);
		JS_NewNumberValue(cx, exitCode, rval);
		JS_EndRequest(cx);
		return JS_TRUE;
	}

	CloseHandle(stdOutWrite);
	CloseHandle(stdErrorWrite);
	stdOutWrite = NULL;
	WaitForMultipleObjects(2, waitHandles, TRUE, INFINITE);
	CloseHandle(waitHandles[0]);
	CloseHandle(waitHandles[1]);

	DWORD exitCode = STILL_ACTIVE;
	GetExitCodeProcess(pi.hProcess, &exitCode);

	JSObject * retObj = JS_NewObject(cx, NULL, NULL, obj);
	*rval = OBJECT_TO_JSVAL(retObj);

	jsval retCodeVal;
	JS_NewNumberValue(cx, exitCode, &retCodeVal);
	JS_DefineProperty(cx, retObj, "exitCode", retCodeVal, NULL, NULL, JSPROP_READONLY | JSPROP_PERMANENT | JSPROP_ENUMERATE);
	JS_DefineProperty(cx, retObj, "stdOut", ParseReaderOutput(&readers[0], cx), NULL, NULL, JSPROP_READONLY | JSPROP_PERMANENT | JSPROP_ENUMERATE);
	JS_DefineProperty(cx, retObj, "stdErr", ParseReaderOutput(&readers[1], cx), NULL, NULL, JSPROP_READONLY | JSPROP_PERMANENT | JSPROP_ENUMERATE);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

functionEnd:
	JS_EndRequest(cx);
	if(stdOutWrite != NULL)
	{
		CloseHandle(stdOutWrite);
		CloseHandle(stdErrorWrite);
	}
	CloseHandle(stdOutRead);
	CloseHandle(stdErrorRead);
	return JS_TRUE;
}

JSBool njord_control_service(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	JSString * serviceName;
	DWORD control = 0;
	JSBool async = JS_FALSE;

	if(!JS_ConvertArguments(cx, argc, argv, "S /u b", &serviceName, &control, &async))
	{
		JS_ReportError(cx, "Error parsing arguments in control_service");
		return JS_FALSE;
	}

	SC_HANDLE scDBHandle = OpenSCManager(NULL, NULL, GENERIC_READ | GENERIC_EXECUTE);
	if(scDBHandle == NULL)
	{
		*rval = JSVAL_FALSE;
		return JS_TRUE;
	}

	SC_HANDLE scHandle = OpenService(scDBHandle, (LPWSTR)JS_GetStringChars(serviceName), GENERIC_EXECUTE | SERVICE_QUERY_STATUS);
	if(scHandle == NULL)
	{
		*rval = JSVAL_FALSE;
		return JS_TRUE;
	}

	DWORD expectedState;
	switch(control)
	{
	case SERVICE_CONTROL_CONTINUE:
	case 0:
		expectedState = SERVICE_RUNNING;
		break;
	case SERVICE_CONTROL_PAUSE:
		expectedState = SERVICE_PAUSED;
		break;
	case SERVICE_CONTROL_STOP:
		expectedState = SERVICE_STOPPED;
		break;
	}
	SERVICE_STATUS curStatus;
	QueryServiceStatus(scHandle, &curStatus);
	if(curStatus.dwCurrentState == expectedState)
	{
		*rval = JSVAL_TRUE;
		CloseServiceHandle(scHandle);
		CloseServiceHandle(scDBHandle);
		return JS_TRUE;
	}

	BOOL status = FALSE;
	if(control == 0)
		status = (jsval)StartService(scHandle, 0, NULL);
	else
		status = (jsval)ControlService(scHandle, control, &curStatus);
	if(!status)
	{
		*rval = JSVAL_FALSE;
		return JS_TRUE;
	}
	else
		*rval = JSVAL_TRUE;
	if(async == JS_TRUE)
	{
		CloseServiceHandle(scHandle);
		CloseServiceHandle(scDBHandle);
		return JS_TRUE;
	}

	DWORD oldCheckPoint, startTickCount = GetTickCount();
	while(curStatus.dwCurrentState != expectedState)
	{
		oldCheckPoint = curStatus.dwCheckPoint;
		DWORD waitTime = curStatus.dwWaitHint / 10;
		if(waitTime < 1000)
			waitTime = 1000;
		else if(waitTime > 10000)
			waitTime = 10000;
		Sleep(waitTime);

		QueryServiceStatus(scHandle, &curStatus);
		if(curStatus.dwCheckPoint > oldCheckPoint)
		{
			startTickCount = GetTickCount();
			oldCheckPoint = curStatus.dwCheckPoint;
		}
		else
		{
			if(GetTickCount() - startTickCount > curStatus.dwWaitHint)
				break;
		}
	}

	QueryServiceStatus(scHandle, &curStatus);
	if(curStatus.dwCurrentState == expectedState)
		*rval = JSVAL_TRUE;
	else
	{
		*rval = JSVAL_FALSE;
		if(curStatus.dwWin32ExitCode == ERROR_SERVICE_SPECIFIC_ERROR)
			SetLastError(curStatus.dwServiceSpecificExitCode);
		else
			SetLastError(curStatus.dwWin32ExitCode);
	}
	CloseServiceHandle(scHandle);
	CloseServiceHandle(scDBHandle);
	return JS_TRUE;
}

JSBool njord_query_service_status(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	JSString * serviceName;
	JSBool justState = JS_TRUE;

	if(!JS_ConvertArguments(cx, argc, argv, "S /b", &serviceName, &justState))
	{
		JS_ReportError(cx, "Error parsing arguments in control_service");
		return JS_FALSE;
	}

	SC_HANDLE scDBHandle = OpenSCManager(NULL, NULL, GENERIC_READ | GENERIC_EXECUTE);
	if(scDBHandle == NULL)
	{
		*rval = JSVAL_FALSE;
		return JS_TRUE;
	}

	SC_HANDLE scHandle = OpenService(scDBHandle, (LPWSTR)JS_GetStringChars(serviceName), GENERIC_EXECUTE | SERVICE_QUERY_STATUS);
	if(scHandle == NULL)
	{
		CloseServiceHandle(scDBHandle);
		*rval = JSVAL_FALSE;
		return JS_TRUE;
	}

	SERVICE_STATUS_PROCESS curStatus;
	DWORD outSize;
	BOOL queryOK = QueryServiceStatusEx(scHandle, SC_STATUS_PROCESS_INFO, (LPBYTE)&curStatus, sizeof(SERVICE_STATUS_PROCESS), &outSize);
	CloseServiceHandle(scHandle);
	CloseServiceHandle(scDBHandle);
	if(!queryOK)
	{
		*rval = JSVAL_FALSE;
		return JS_TRUE;
	}

	if(justState == TRUE)
	{
		JS_NewNumberValue(cx, curStatus.dwCurrentState, rval);
		return JS_TRUE;
	}
	return JS_TRUE;
}

JSBool InitExec(JSContext * cx, JSObject * global)
{
	struct JSConstDoubleSpec serviceConsts[] = {
		{ 0, "SERVICE_CONTROL_START", 0, 0 },
		{ SERVICE_CONTROL_CONTINUE, "SERVICE_CONTROL_CONTINUE", 0, 0 },
		{ SERVICE_CONTROL_PAUSE, "SERVICE_CONTROL_PAUSE", 0, 0 },
		{ SERVICE_CONTROL_STOP, "SERVICE_CONTROL_STOP", 0, 0 },
		{ SERVICE_CONTINUE_PENDING, "SERVICE_CONTINUE_PENDING", 0, 0 },
		{ SERVICE_PAUSE_PENDING, "SERVICE_PAUSE_PENDING", 0, 0 },
		{ SERVICE_PAUSED, "SERVICE_PAUSED", 0, 0 },
		{ SERVICE_RUNNING, "SERVICE_RUNNING", 0, 0 },
		{ SERVICE_START_PENDING, "SERVICE_START_PENDING", 0, 0 },
		{ SERVICE_STOP_PENDING, "SERVICE_STOP_PENDING", 0, 0 },
		{ SERVICE_STOPPED, "SERVICE_STOPPED", 0, 0 },
		{ 0 },
	};
	JS_DefineConstDoubles(cx, global, serviceConsts);

	JSFunctionSpec execMethods[] = {
		{"Exec", xprep_js_exec, 1, 0, 0},
		{"ControlService", njord_control_service, 1, 0, 0 },
		{"QueryServiceStatus", njord_query_service_status, 1, 0, 0 },
		{0, 0, 0, 0, 0 }
	};

	return JS_DefineFunctions(cx, global, execMethods);
}