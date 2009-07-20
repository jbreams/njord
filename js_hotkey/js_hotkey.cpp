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

#define STATE_ADDED 0
#define STATE_REGISTERED 1
#define STATE_REMOVED 2

struct HotKeyRegistration {
	LPSTR functionName;
	UINT flags;
	UINT vk;
	int id;
	BYTE state;

	struct HotKeyRegistration * next;
} * hotKeyHead;
BOOL registrationsChanged;

JSContext * useCx;
JSObject * useGlobal;

DWORD HotKeyThread(LPVOID param)
{
	SetThreadPriority(GetCurrentThread(), THREAD_MODE_BACKGROUND_BEGIN);
	HANDLE runningEvent = OpenEvent(SYNCHRONIZE, FALSE, TEXT("hotkey_running_event"));
	while(WaitForSingleObject(runningEvent, 0) != WAIT_OBJECT_0)
	{
		if(registrationsChanged == TRUE)
		{
			HANDLE lock = OpenMutex(SYNCHRONIZE, FALSE, TEXT("hotkey_table_mutex"));
			struct HotKeyRegistration * curRegistration = hotKeyHead, *prevReg = NULL;
			while(curRegistration != NULL)
			{
				BOOL advance = TRUE;
				if(curRegistration->state == STATE_ADDED)
				{
					if(curRegistration->next != NULL)
						curRegistration->id = curRegistration->next->id + 1;
					else
						curRegistration->id = 0;
					if(RegisterHotKey(NULL, curRegistration->id, curRegistration->flags, curRegistration->vk))
						curRegistration->state = STATE_REGISTERED;
				}
				else if(curRegistration->state == STATE_REMOVED)
				{
					struct HotKeyRegistration * freer = curRegistration;
					UnregisterHotKey(NULL, curRegistration->id);
					HeapFree(GetProcessHeap(), 0, curRegistration->functionName);
					if(prevReg != NULL)
					{
						prevReg->next = curRegistration->next;
						curRegistration = curRegistration->next;
					}
					else
					{
						hotKeyHead = curRegistration->next;
						curRegistration = hotKeyHead;
					}
					HeapFree(GetProcessHeap(), 0, freer);
					advance = FALSE;
				}
				if(advance == TRUE)
				{
					prevReg = curRegistration;
					curRegistration = curRegistration->next;
				}
			}
			registrationsChanged = FALSE;
			ReleaseMutex(lock);
		}

		MSG msg;
		memset(&msg, 0, sizeof(MSG));
		if(!PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) || msg.message != WM_HOTKEY)
		{
			Sleep(250);
			continue;
		}

		UINT vk = HIWORD(msg.lParam), flags = LOWORD(msg.lParam);
		HANDLE lock = OpenMutex(SYNCHRONIZE, FALSE, TEXT("hotkey_table_mutex"));
		struct HotKeyRegistration * curReg = hotKeyHead;
		while(curReg != NULL)
		{
			if(curReg->vk == vk && curReg->flags == flags)
			{
				jsval vals[2];
				vals[0] = JSVAL_VOID; vals[1] = JSVAL_VOID;
				JS_BeginRequest(useCx);
				JS_NewNumberValue(useCx, flags, &vals[0]);
				JS_NewNumberValue(useCx, vk, &vals[1]);
				
				jsval dontneedit;
				JS_CallFunctionName(useCx, useGlobal, curReg->functionName, 2, vals, &dontneedit);
				JS_EndRequest(useCx);
			}
			curReg = curReg->next;
		}
		ReleaseMutex(lock);
	}

	SetThreadPriority(GetCurrentThread(), THREAD_MODE_BACKGROUND_END);
	HANDLE lock = OpenMutex(SYNCHRONIZE, FALSE, TEXT("hotkey_table_mutex"));
	struct HotKeyRegistration * curReg = hotKeyHead;
	while(curReg != NULL)
	{
		struct HotKeyRegistration * next = curReg->next;
		UnregisterHotKey(NULL, curReg->id);
		HeapFree(GetProcessHeap(), 0, curReg->functionName);
		HeapFree(GetProcessHeap(), 0, curReg);
		curReg = next;
	}
	hotKeyHead = NULL;
	ResetEvent(runningEvent);
	CloseHandle(runningEvent);
	ReleaseMutex(lock);
	return 0;
}

JSBool JSRegisterHotKey(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	char *functionName, * keyCode;
	UINT flags;

	JS_BeginRequest(cx);
	if(!JS_ConvertArguments(cx, argc, argv, "s u s", &keyCode, &flags, &functionName))
	{
		JS_ReportError(cx, "Unable to parse arguments in RegisterHotKeys");
		JS_EndRequest(cx);
		return JS_FALSE;
	}
	JS_EndRequest(cx);

	if(strlen(keyCode) < 1 || flags == 0)
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		*rval = JSVAL_FALSE;
		return JS_TRUE;
	}

	UINT vkCode = 0;
	if(keyCode[0] >= 'a' && keyCode[0] <= 'z')
		keyCode[0] -= 0x20;
	vkCode = (UINT)keyCode[0];

	HANDLE runningEvent = CreateEvent(NULL, TRUE, FALSE, TEXT("hotkey_running_event"));
	struct HotKeyRegistration * newReg = (struct HotKeyRegistration*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(struct HotKeyRegistration));
	newReg->functionName = (LPSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, strlen(functionName) + 1);
	strcpy_s(newReg->functionName, strlen(functionName) + 1, functionName);
	newReg->vk = vkCode;
	newReg->flags = flags;

	HANDLE lock = OpenMutex(SYNCHRONIZE, FALSE, TEXT("hotkey_table_mutex"));
	registrationsChanged = TRUE;
	if(hotKeyHead != NULL)
	{
		newReg->next = hotKeyHead;
		hotKeyHead = newReg;
	}
	else
	{
		hotKeyHead = newReg;
		useCx = cx;
		useGlobal = obj;
		HANDLE hotKeyThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)HotKeyThread, NULL, 0, NULL);
		CloseHandle(hotKeyThread);
	}
	ReleaseMutex(lock);

	*rval = JSVAL_VOID;
	while(*rval == JSVAL_VOID)
	{
		lock = OpenMutex(SYNCHRONIZE, FALSE, TEXT("hotkey_table_mutex"));
		newReg = hotKeyHead;
		while(newReg != NULL)
		{
			int nameMatch = strcmp(newReg->functionName, functionName);
			if(newReg->flags == flags && newReg->vk == vkCode && nameMatch  == 0 && newReg->state == STATE_REGISTERED)
			{
				JS_BeginRequest(cx);
				JS_NewNumberValue(cx, newReg->id, rval);
				JS_EndRequest(cx);
				break;
			}
			newReg = newReg->next;
		}
		ReleaseMutex(lock);
		Sleep(250);
	}
	return JS_TRUE;
}

JSBool JSUnregisterHotKey(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	UINT id;
	JS_BeginRequest(cx);
	if(!JS_ConvertArguments(cx, argc, argv, "c", &id))
	{
		JS_ReportError(cx, "Unable to parse arguments in UnregisterHotKey");
		JS_EndRequest(cx);
		return JS_FALSE;
	}
	JS_EndRequest(cx);

	HANDLE lock = OpenMutex(SYNCHRONIZE, FALSE, TEXT("hotkey_table_mutex"));
	struct HotKeyRegistration * curReg = hotKeyHead;
	while(curReg != NULL && curReg->id != id)
		curReg = curReg->next;
	if(curReg == NULL)
		*rval = JSVAL_FALSE;
	else
	{
		registrationsChanged = TRUE;
		curReg->state = STATE_REMOVED;
		*rval = JSVAL_TRUE;
	}
	ReleaseMutex(lock);
	return JS_TRUE;
}

JSBool JSStopHotKeys(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	HANDLE runningEvent = OpenEvent(SYNCHRONIZE, FALSE, TEXT("hotkey_running_event"));
	SetEvent(runningEvent);
	while(WaitForSingleObject(runningEvent, 0) == WAIT_OBJECT_0)
		Sleep(250);
	CloseHandle(runningEvent);
	*rval = JSVAL_TRUE;
	return JS_TRUE;
}

struct JSConstDoubleSpec hotKeyConsts[] = {
	{ MOD_ALT, "MOD_ALT", 0, 0 },
	{ MOD_CONTROL, "MOD_CONTROL", 0, 0 },
	{ MOD_SHIFT, "MOD_SHIFT", 0, 0 },
	{ MOD_WIN, "MOD_WIN", 0, 0 },
	{ 0 }
};

#ifdef __cplusplus
extern "C" {
#endif
BOOL __declspec(dllexport) InitExports(JSContext * cx, JSObject * global)
{
	JSFunctionSpec hotKeyFunctions [] = {
		{ "RegisterHotKey", JSRegisterHotKey, 3, 0, 0 },
		{ "UnregisterHotKey", JSUnregisterHotKey, 1, 0, 0 },
		{ "StopHotKeys", JSStopHotKeys, 0, 0, 0 },
		{ 0 }
	};

	CreateMutex(NULL, FALSE, TEXT("hotkey_table_mutex"));
	JS_BeginRequest(cx);
	JS_DefineConstDoubles(cx, global, hotKeyConsts);
	JS_DefineFunctions(cx, global, hotKeyFunctions);
	JS_EndRequest(cx);
	return TRUE;
}

BOOL __declspec(dllexport) CleanupExports(JSContext * cx, JSObject * global)
{
	if(hotKeyHead != NULL)
	{
		HANDLE runningEvent = OpenEvent(SYNCHRONIZE, FALSE, TEXT("hotkey_running_event"));
		SetEvent(runningEvent);
		while(WaitForSingleObject(runningEvent, 0) == WAIT_OBJECT_0)
			Sleep(250);
		CloseHandle(runningEvent);
	}
	return TRUE;
}
#ifdef __cplusplus
}
#endif