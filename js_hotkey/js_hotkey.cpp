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

DWORD threadId = 0;
struct hotKeyRegistration {
	UINT fsModifiers;
	UINT vk;
	int id;
	LPSTR functionName;
	struct hotKeyRegistration * next;
};

DWORD hotkeyThread(LPVOID param)
{
	struct hotKeyRegistration * hotKeyHead = NULL;
	int curId = 0;
	JSContext * cx = JS_NewContext(JS_GetRuntime((JSContext*)param), 0x2000);
	JS_SetGlobalObject(cx, JS_GetGlobalObject((JSContext*)param));

	MSG msg;
	while(GetMessage(&msg, NULL, 0, 0))
	{
		if(msg.message == WM_APP + 1)
		{
			struct hotKeyRegistration * msgToAdd = (struct hotKeyRegistration*)msg.lParam;
			if(RegisterHotKey(NULL, curId, msgToAdd->fsModifiers, msgToAdd->vk))
			{
				msgToAdd->id = curId;
				msgToAdd->next = hotKeyHead;
				hotKeyHead = msgToAdd;
			}
			else
				msgToAdd->id = -2;
		}
		else if(msg.message == WM_APP + 2)
		{
			UINT id = msg.wParam;
			struct hotKeyRegistration * curReg = hotKeyHead, *prevReg = NULL;
			while(curReg != NULL && curReg->id != id)
			{
				prevReg = curReg;
				curReg = curReg->next;
			}
			if(curReg == NULL)
				continue;
			if(prevReg != NULL)
				prevReg->next = curReg->next;
			else
				hotKeyHead = curReg->next;
			UnregisterHotKey(NULL, id);
			HeapFree(GetProcessHeap(), 0, curReg->functionName);
			HeapFree(GetProcessHeap(), 0, curReg);
		}
		else if(msg.message == WM_HOTKEY)
		{
			UINT vk = HIWORD(msg.lParam), fsModifiers = LOWORD(msg.lParam);
			struct hotKeyRegistration * curReg = hotKeyHead;
			while(curReg != NULL && curReg->fsModifiers != fsModifiers && curReg->vk != vk)
				curReg = curReg->next;
			if(curReg != NULL)
			{
				JS_BeginRequest(cx);
				jsval argv[3];
				argv[0] = INT_TO_JSVAL(fsModifiers);
				argv[1] = INT_TO_JSVAL(vk);
				argv[3] = JSVAL_VOID;
				JS_CallFunctionName(cx, JS_GetGlobalObject(cx), curReg->functionName, 2, argv, &argv[2]);
				JS_EndRequest(cx);
			}
		}
	}
	JS_DestroyContext(cx);

	struct hotKeyRegistration * curReg = hotKeyHead, *nextReg = NULL;
	while(curReg != NULL)
	{
		nextReg = curReg->next;
		HeapFree(GetProcessHeap(), 0, curReg->functionName);
		HeapFree(GetProcessHeap(), 0, curReg);
		curReg = nextReg;
	}
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

	struct hotKeyRegistration * newReg = (struct hotKeyRegistration*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(struct hotKeyRegistration));
	newReg->functionName = (LPSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, strlen(functionName) + 1);
	strcpy_s(newReg->functionName, strlen(functionName) + 1, functionName);
	newReg->fsModifiers = flags;
	newReg->vk = vkCode;
	newReg->id = -1;

	if(threadId == 0)
	{
		HANDLE hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)hotkeyThread, (LPVOID)cx, 0, &threadId);
		if(hThread == INVALID_HANDLE_VALUE)
		{
			*rval = JSVAL_FALSE;
			return JS_TRUE;
		}
		CloseHandle(hThread);
	}

	while(!PostThreadMessage(threadId, WM_APP + 1, 0, (LPARAM)newReg))
		Sleep(200);
	while(newReg->id == -1)
		Sleep(200);
	if(newReg->id == -2)
	{
		*rval = JSVAL_FALSE;
		return JS_TRUE;
	}
	*rval = INT_TO_JSVAL(newReg->id);
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
	if(threadId == 0)
	{
		*rval = JSVAL_FALSE;
		return JS_TRUE;
	}

	while(!PostThreadMessage(threadId, WM_APP + 2, (WPARAM)id, 0))
		Sleep(200);
	*rval = JSVAL_TRUE;
	return JS_TRUE;
}

JSBool JSStopHotKeys(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	if(threadId == 0)
	{
		*rval = JSVAL_FALSE;
		return JS_TRUE;
	}
	while(!PostThreadMessage(threadId, WM_QUIT, 0, 0))
		Sleep(200);
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

	JS_BeginRequest(cx);
	JS_DefineConstDoubles(cx, global, hotKeyConsts);
	JS_DefineFunctions(cx, global, hotKeyFunctions);
	JS_EndRequest(cx);
	return TRUE;
}

BOOL __declspec(dllexport) CleanupExports(JSContext * cx, JSObject * global)
{
	if(threadId != 0)
	{
		while(!PostThreadMessage(threadId, WM_QUIT, 0, 0))
			Sleep(200);
	}
	return TRUE;
}
#ifdef __cplusplus
}
#endif