#include "stdafx.h"
#include <commctrl.h>
#include "resource.h"
#pragma comment(lib, "comctl32.lib")

void finalizeUI(JSContext * cx, JSObject * uiObj)
{
	HWND myWnd = (HWND)JS_GetPrivate(cx, uiObj);
	if(myWnd != NULL)
		SendMessage(myWnd, WM_DESTROY, 0, 0);
}
JSClass xDriverUI = {	"xDriverUI",  /* name */
    JSCLASS_HAS_PRIVATE,  /* flags */
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, finalizeUI,
    JSCLASS_NO_OPTIONAL_MEMBERS
};
JSObject * xDriverUIProto = NULL;

extern HMODULE myModule;

BOOL xDriver_DlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
	case WM_INITDIALOG:
		return 1;
	case WM_DESTROY:
		PostQuitMessage(0);
	}
	return 0;
}

DWORD xDriverUIThread(LPVOID param)
{
	HWND * myWindow = (HWND*)param;
	INITCOMMONCONTROLSEX icce;
	icce.dwSize = sizeof(icce);
	icce.dwICC = ICC_PROGRESS_CLASS;
	BOOL ok = InitCommonControlsEx(&icce);
	if(!ok)
	{
		MessageBox(NULL, TEXT("Error initialzing common controls. Cannot continue."), TEXT("xDriver"), MB_OK);
		return 1;
	}

	*myWindow = CreateDialogW(myModule, MAKEINTRESOURCE(IDD_DIALOG), NULL, (DLGPROC)xDriver_DlgProc);
	if(*myWindow == NULL)
	{
		MessageBox(NULL, TEXT("Error creating window. Cannot continue."), TEXT("xDriver"), MB_OK);
		return 1;
	}
	DWORD errorCode = GetLastError();
	ShowWindow(*myWindow, SW_SHOW);
	MSG msg;
	while(GetMessage(&msg, NULL, 0, 0))
	{
		if(!IsDialogMessage(*myWindow, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	return 0;
}

JSBool create_ui(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	HWND myWnd = NULL;
	HANDLE uiThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)xDriverUIThread, &myWnd, 0, NULL);
	CloseHandle(uiThread);
	while(myWnd == NULL)
		Sleep(5);

	JS_BeginRequest(cx);
	JSObject * retObj = JS_NewObject(cx, &xDriverUI, xDriverUIProto, obj);
	*rval = OBJECT_TO_JSVAL(retObj);
	JS_SetPrivate(cx, retObj, myWnd);
	JS_EndRequest(cx);
	return JS_TRUE;
}

JSBool set_static_text(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	LPWSTR message;
	JS_BeginRequest(cx);
	if(!JS_ConvertArguments(cx, argc, argv, "W", &message))
	{
		JS_ReportError(cx, "Error parsing arguments in SetStaticText");
		JS_EndRequest(cx);
		return JS_FALSE;
	}
	JS_EndRequest(cx);

	HWND myWnd = (HWND)JS_GetPrivate(cx, obj);
	SetDlgItemText(myWnd, IDC_STATICTEXT, message);
	return JS_TRUE;
}

JSBool set_progress(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	WORD pos;
	BOOL max = FALSE;
	JS_BeginRequest(cx);
	JS_ConvertArguments(cx, argc, argv, "c /b", &pos, &max);
	JS_EndRequest(cx);

	HWND myWnd = (HWND)JS_GetPrivate(cx, obj);
	if(max)
		SendDlgItemMessage(myWnd, IDC_PROGRESS, PBM_SETRANGE, 0, MAKELPARAM(0, pos));
	else
		SendDlgItemMessage(myWnd, IDC_PROGRESS, PBM_SETPOS, pos, 0);
	return JS_TRUE;
}

JSBool show_or_hide(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	int cmd = SW_SHOW;
	JS_BeginRequest(cx);
	if(argc > 0 && JSVAL_IS_BOOLEAN(*argv) && !JSVAL_TO_BOOLEAN(*argv))
		cmd = SW_HIDE;
	HWND myWnd = (HWND)JS_GetPrivate(cx, obj);
	*rval = ShowWindow(myWnd, cmd) ? JSVAL_TRUE : JSVAL_FALSE;
	JS_EndRequest(cx);
	return JS_TRUE;
}

void InitProgressDlg(JSContext * cx, JSObject * obj)
{
	JSFunctionSpec ui_funcs[] = {
		{ "SetStaticText", set_static_text, 1, 0, 0 },
		{ "SetProgressBar", set_progress, 2, 0, 0 },
		{ "Show", show_or_hide, 1, 0, 0 },
		{ 0 }
	};

	xDriverUIProto = JS_InitClass(cx, obj, NULL, &xDriverUI, NULL, 0, NULL, ui_funcs, NULL, NULL);
	JS_DefineFunction(cx, obj, "CreateProgressDlg", (JSNative)create_ui, 0, 0);
}