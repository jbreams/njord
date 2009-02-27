#include "stdafx.h"
#include "xprep.h"


struct JSConstDoubleSpec win32MessageBoxTypes[] = {
	{ MB_ABORTRETRYIGNORE, "MB_ABORTRETRYIGNORE", 0, 0 },
	{ MB_CANCELTRYCONTINUE, "MB_CANCELTRYCONTINUE", 0, 0 },
	{ MB_HELP, "MB_HELP", 0, 0 },
	{ MB_OK, "MB_OK", 0, 0 },
	{ MB_OKCANCEL, "MB_OKCANCEL", 0, 0 },
	{ MB_RETRYCANCEL, "MB_RETRYCANCEL", 0, 0 },
	{ MB_YESNO, "MB_YESNO", 0, 0 },
	{ MB_YESNOCANCEL, "MB_YESNOCANCEL", 0, 0 },
	{ MB_ICONEXCLAMATION, "MB_ICONEXCLAMATION", 0, 0 },
	{ MB_ICONWARNING, "MB_ICONWARNING", 0, 0 },
	{ MB_ICONINFORMATION, "MB_ICONINFORMATION", 0, 0 },
	{ MB_ICONASTERISK, "MB_ICONASTERISK", 0, 0 },
	{ MB_ICONQUESTION, "MB_ICONQUESTION", 0, 0 },
	{ MB_ICONSTOP, "MB_ICONSTOP", 0, 0 },
	{ MB_ICONERROR, "MB_ICONERROR", 0, 0 },
	{ MB_ICONHAND, "MB_ICONHAND", 0, 0 },
};

JSBool win32_messagebox(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval *rval)
{
	JSString * caption = NULL, *title = NULL;
	UINT type = MB_OK;

	if(!JS_ConvertArguments(cx, argc, argv, "S /S c", &caption, &title, &type))
	{
		JS_ReportError(cx, "Unable to parse arguments for win32_messagebox");
		return JS_FALSE;
	}

	DWORD errorCode = 0;
	if(title != NULL)
		errorCode = MessageBoxW(NULL, JS_GetStringChars(caption), JS_GetStringChars(title), type);
	else
		errorCode = MessageBoxW(NULL, JS_GetStringChars(caption), TEXT("njord"), type);
	return JS_NewNumberValue(cx, errorCode, rval);
}

void InitWin32s(JSContext * cx, JSObject * global)
{
	JS_DefineConstDoubles(cx, global, win32MessageBoxTypes);
	JS_DefineFunction(cx, global, "MessageBox", win32_messagebox, 1, 0);
}