#include "stdafx.h"
#include "xprep.h"
#include "embed.h"
#include "varstore.h"

#define UI_CLASS_NAME TEXT("xprep_gecko_ui")
#define WM_SHOWUI WM_APP + 1
#define WM_SHOWURI WM_APP + 2

struct UIDATA {
	LPSTR uiDOM;
	LPSTR uriBase;
	VarStore * localVars;
	VarStore * parsingVars;
	int cx;
	int cy;
};

class UIListener : public MozViewListener
{
public:
    void SetTitle(const char* newTitle);
    PRBool OpenURI(const char* newLocation);

    MozView* OpenWindow(PRUint32 flags);
    void SizeTo(PRUint32 width, PRUint32 height);
    void SetVisibility(PRBool visible);
    void StartModal();
    void ExitModal(nsresult result);
	VarStore * localVars;
};


HWND topWnd;
bool gDoModal;

LRESULT CALLBACK MozViewProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	MozView* mMozView = (MozView*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
	switch(message)	
	{
	case WM_SHOWUI:
		if(mMozView)
		{
			UIDATA * showData = (UIDATA*)lParam;
			UIListener * listener = (UIListener*)mMozView->GetListener();
			listener->localVars = showData->localVars;
			LPSTR uriBase = "file:";
			if(showData->uriBase != NULL)
				uriBase = showData->uriBase;
			LRESULT retVal = (LRESULT)mMozView->LoadData(uriBase, "text/html", (PRUint8*)showData->uiDOM, strlen(showData->uiDOM));
			if(showData->parsingVars != NULL)
				showData->parsingVars->ParseIntoXML(mMozView);
			
			if(showData->cx != 0 && showData->cy != 0)
			{
				RECT rect;
				GetClientRect(hWnd, &rect);
				int dx, dy;
				dx = rect.right - rect.left;
				dy = rect.bottom - rect.top;
				if(dx != showData->cx || dy != showData->cy)
					MoveWindow(hWnd, rect.top, rect.left, showData->cx, showData->cy, TRUE);
			}

			return retVal;
		}
		return 1;
	case WM_SHOWURI:
		if(mMozView)
		{
			UIDATA * showData = (UIDATA*)lParam;
			UIListener * listener = (UIListener*)mMozView->GetListener();
			listener->localVars = showData->localVars;
			LRESULT retVal = (LRESULT)mMozView->LoadURI(showData->uiDOM);
			if(showData->parsingVars != NULL)
				showData->parsingVars->ParseIntoXML(mMozView);
			
			if(showData->cx != 0 && showData->cy != 0)
			{
				RECT rect;
				GetClientRect(hWnd, &rect);
				int dx, dy;
				dx = rect.right - rect.left;
				dy = rect.bottom - rect.top;
				if(dx != showData->cx || dy != showData->cy)
					MoveWindow(hWnd, rect.top, rect.left, showData->cx, showData->cy, TRUE);
			}

			return retVal;
		}
		return 1;
	case WM_CREATE:
		{
			mMozView = new MozView();
			if(mMozView == NULL)
				return FALSE;
			CREATESTRUCT * myCS = (CREATESTRUCT*)lParam;
			mMozView->CreateBrowser(hWnd, myCS->x, myCS->y, myCS->cx, myCS->cy);
			mMozView->SetListener(new UIListener());
			SetWindowLongPtrW(hWnd, GWLP_USERDATA, (LONG_PTR)mMozView);
		}
		return 0;
	case WM_DESTROY:
		if(mMozView)
		{
			mMozView->Stop();
			delete mMozView->GetListener();
			delete mMozView;
		}
		break;
    case WM_CLOSE:
		if(mMozView)
		{
			MozView* parentView = mMozView->GetParentView();
			if(hWnd != topWnd)
				DestroyWindow(hWnd);
			if (parentView && gDoModal)
				::EnableWindow((HWND)parentView->GetParentWindow(), TRUE);
		}
		break;
    case WM_SIZE:
		if (mMozView) {
			RECT rect;
			GetClientRect(hWnd, &rect);
			mMozView->SetPositionAndSize(rect.left, rect.top,
					rect.right - rect.left, rect.bottom - rect.top);
		}
		break;
    case WM_ACTIVATE:
		if (mMozView) {
			switch (wParam) {
			case WA_CLICKACTIVE:
			case WA_ACTIVE:
				mMozView->SetFocus(true);
				break;
			case WA_INACTIVE:
				mMozView->SetFocus(false);
				break;
			default:
				break;
			}
		}
		break;
	case WM_ERASEBKGND:
        // Reduce flicker by not painting the non-visible background
        return 1;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
		break;
	}
	return 0;
}

DWORD UIThread(LPVOID param)
{
	HINSTANCE hInstance = GetModuleHandle(NULL);
	DWORD errorCode;
	WNDCLASS wc;
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = (WNDPROC)MozViewProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_XPREP));
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = UI_CLASS_NAME;
	if(!RegisterClass(&wc))
		return GetLastError();

	HWND uiWnd = CreateWindowW(UI_CLASS_NAME, TEXT("Setup"), WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 800, 600, NULL, NULL, hInstance, NULL);
	HANDLE uiRunningEvent = OpenEvent(SYNCHRONIZE | EVENT_MODIFY_STATE, FALSE, TEXT("xprep_uithread_running"));
	if(uiWnd == NULL || uiRunningEvent == NULL)
	{
		errorCode = GetLastError();
		goto end;
	}
	ShowWindow(uiWnd, SW_SHOW);
	UpdateWindow(uiWnd);
	topWnd = uiWnd;
	MSG msg;

	MozView * topView = NULL;
	UIListener * uiListener = NULL;
	while(uiListener == NULL)
	{
		if(GetMessage(&msg, NULL, 0, 0))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		topView = (MozView*)GetWindowLongPtrW(uiWnd, GWLP_USERDATA);
		if(topView != NULL)
			uiListener = (UIListener*)topView->GetListener();
	}

	SetEvent(uiRunningEvent);
	while(WaitForSingleObject(uiRunningEvent, 0) == WAIT_OBJECT_0)
	{
		if(GetMessage(&msg, NULL, 0, 0))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

end:
	ResetEvent(uiRunningEvent);
	CloseHandle(uiRunningEvent);
	UnregisterClass(UI_CLASS_NAME, hInstance);
	return GetLastError();
}

JSBool initUI(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	JSBool retval = JS_TRUE;
	if(topWnd != NULL)
		return JS_TRUE;
	HANDLE uiRunningEvent = CreateEvent(NULL, TRUE, FALSE, TEXT("xprep_uithread_running"));
	HANDLE threadHandle = NULL;
	if(uiRunningEvent == NULL)
	{
		JS_ReportError(cx, "Error creating UI Running event.");
		retval = JS_FALSE;
		goto end;
	}
	threadHandle = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)UIThread, (LPVOID)GetModuleHandle(NULL), 0, NULL);
	if(threadHandle == NULL)
	{
		JS_ReportError(cx, "Error starting UI thread.");
		retval = JS_FALSE;
		goto end;
	}
	if(WaitForSingleObject(uiRunningEvent, 30000) == WAIT_ABANDONED)
	{
		JS_ReportError(cx, "The UI thread never started, cannot continue.");
		retval = JS_FALSE;
	}

end:
	if(uiRunningEvent != NULL)
		CloseHandle(uiRunningEvent);
	if(threadHandle != NULL)
		CloseHandle(threadHandle);
	return retval;
}

JSBool shutdownUI(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	HANDLE uiRunningEvent = OpenEvent(SYNCHRONIZE | EVENT_MODIFY_STATE, FALSE, TEXT("xprep_uithread_running"));
	if(uiRunningEvent == NULL)
	{
		JS_ReportError(cx, "Unable to open UI running event.");
		return JS_FALSE;
	}
	ResetEvent(uiRunningEvent);
	while(WaitForSingleObject(uiRunningEvent, 500) == WAIT_OBJECT_0)
		Sleep(500);
	CloseHandle(uiRunningEvent);
	topWnd = NULL;
	return JS_TRUE;
}

JSBool showUI(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	JSString * xmlRepStr = NULL, *uriBase = NULL;
	JSBool async = JS_FALSE;
	JSObject * varStoreObj = NULL;

	if(!JS_ConvertArguments(cx, argc, argv, "S /o S b", &xmlRepStr, &varStoreObj, &uriBase, &async))
	{
		JS_ReportError(cx, "Unable to parse arguments in showURI");
		return JS_FALSE;
	}

	LPWSTR domWStr = JS_GetStringChars(xmlRepStr);
	DWORD len = WideCharToMultiByte(CP_UTF8, 0, domWStr, -1, NULL, 0, NULL, NULL);
	UIDATA * dataToPass = (UIDATA*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(UIDATA));
	dataToPass->uiDOM = (LPSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len + 1);
	WideCharToMultiByte(CP_UTF8, 0, domWStr, -1, dataToPass->uiDOM, len + 1, NULL, NULL);

	if(uriBase != NULL)
	{
		LPWSTR uriBaseStr = JS_GetStringChars(uriBase);
		DWORD len2 = WideCharToMultiByte(CP_UTF8, 0, uriBaseStr, -1, NULL, 0, NULL, NULL);
		dataToPass->uriBase = (LPSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len);
		WideCharToMultiByte(CP_UTF8, 0, uriBaseStr, -1, dataToPass->uriBase, len2, NULL, NULL);
	}

	JSObject * localStoreObj = JS_ConstructObject(cx, &varStoreClass, varStoreProto, obj);
	*rval = OBJECT_TO_JSVAL(localStoreObj);
	dataToPass->localVars = (VarStore*)JS_GetPrivate(cx, localStoreObj);

	if(varStoreObj != NULL)
		dataToPass->parsingVars = (VarStore*)JS_GetPrivate(cx, varStoreObj);

	ShowWindow(topWnd, SW_SHOW);
	LRESULT result = SendMessage(topWnd, WM_SHOWUI, 0, (LPARAM)dataToPass);
	
	if(result != 0)
	{
		HeapFree(GetProcessHeap(), 0, dataToPass->uiDOM);
		if(dataToPass->uriBase != NULL)
			HeapFree(GetProcessHeap(), 0, dataToPass->uriBase);
		HeapFree(GetProcessHeap(), 0, dataToPass);
		JS_ReportError(cx, "Error displaying UI: %i", result);
		return JS_FALSE;
	}

	if(async == JS_TRUE)
	{
		*rval = JS_TRUE;
		return JS_TRUE;
	}

	HANDLE waitHandle = CreateEvent(NULL, TRUE, FALSE, TEXT("xprep_uireturn_wait"));
	ResetEvent(waitHandle);
	if(WaitForSingleObject(waitHandle, INFINITE) != WAIT_OBJECT_0)
	{
		JS_ReportError(cx, "The UI timed out.");
		return JS_FALSE;
	}

	HeapFree(GetProcessHeap(), 0, dataToPass->uiDOM);
	if(dataToPass->uriBase != NULL)
		HeapFree(GetProcessHeap(), 0, dataToPass->uriBase);
	HeapFree(GetProcessHeap(), 0, dataToPass);
	return JS_TRUE;
}

JSBool hideUI(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	ShowWindow(topWnd, SW_HIDE);
	return JS_TRUE;
}
JSBool showURI(JSContext * cx, JSObject *obj, uintN argc, jsval * argv, jsval * rval)
{
	JSString * uriStr;
	JSObject * varStoreObj = NULL;
	JSBool async = JS_FALSE;

	if(!JS_ConvertArguments(cx, argc, argv, "S /o b", &uriStr, &varStoreObj, &async))
	{
		JS_ReportError(cx, "Unable to parse arguments in showURI");
		return JS_FALSE;
	}

	UIDATA * dataToPass = (UIDATA*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(UIDATA));

	jschar * uri = JS_GetStringChars(uriStr);

	DWORD len = WideCharToMultiByte(CP_UTF8, 0, uri, -1, NULL, 0, NULL, NULL);
	dataToPass->uiDOM = (LPSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len + 1);
	WideCharToMultiByte(CP_UTF8, 0, uri, -1, dataToPass->uiDOM, len + 1, NULL, NULL);

	if(varStoreObj != NULL)
		dataToPass->parsingVars = (VarStore*)JS_GetPrivate(cx, varStoreObj);
	
	if(async == JS_FALSE)
	{
		JSObject * localVarObj = JS_ConstructObject(cx, &varStoreClass, varStoreProto, obj);
		*rval = OBJECT_TO_JSVAL(localVarObj);
		dataToPass->localVars = (VarStore*)JS_GetPrivate(cx, localVarObj);
	}

	LRESULT showResult = SendMessage(topWnd, WM_SHOWURI, 0, (LPARAM)dataToPass);
	HeapFree(GetProcessHeap(), 0, dataToPass->uiDOM);
	HeapFree(GetProcessHeap(), 0, dataToPass);

	if(async == JS_TRUE)
	{
		*rval = JS_TRUE;
		return JS_TRUE;
	}

	HANDLE waitHandle = CreateEvent(NULL, TRUE, FALSE, TEXT("xprep_uireturn_wait"));
	ResetEvent(waitHandle);
	if(WaitForSingleObject(waitHandle, INFINITE) != WAIT_OBJECT_0)
	{
		JS_ReportError(cx, "The UI timed out.");
		return JS_FALSE;
	}
	return JS_TRUE;
}

JSBool InitUIFunctions(JSContext * cx, JSObject * global)
{
	struct JSFunctionSpec uiMethods[] = {
		{ "InitUI", initUI, 0, 0, 0 },
		{ "ShowUI", showUI, 1, 0, 0 },
		{ "ShowURI", showURI, 1, 0, 0 },
		{ "HideUI", hideUI, 0, 0, 0 },
		{ "ShutdownUI", shutdownUI, 0, 0, 0 },
		{ 0, 0, 0, 0, 0 }
	};

	return JS_DefineFunctions(cx, global, uiMethods);
}
void UIListener::SetTitle(const char *newTitle)
{
    HWND hWnd = (HWND)mMozView->GetParentWindow();
    WCHAR* newTitleW = Utf8ToWchar(newTitle);
    ::SetWindowTextW(hWnd, newTitleW);
    delete[] newTitleW;
}

PRBool UIListener::OpenURI(const char* newLocation)
{
	HANDLE waitHandle = OpenEvent(EVENT_MODIFY_STATE, FALSE, TEXT("xprep_uireturn_wait"));
	if(waitHandle == NULL)
		return FALSE;
	
	localVars->SuckOutVars(mMozView);
	SetEvent(waitHandle);
	CloseHandle(waitHandle);
    return TRUE;
}

MozView* UIListener::OpenWindow(PRUint32 flags)
{
    HWND hWnd;
	HINSTANCE hInst = (HINSTANCE)GetModuleHandle(NULL);

    hWnd = CreateWindowW(UI_CLASS_NAME, TEXT("Setup"), WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInst, NULL);

    if (!hWnd) {
        return 0;
    }

    ShowWindow(hWnd, SW_SHOW);
    UpdateWindow(hWnd);

	MozView * pNewView = (MozView*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
    pNewView->SetParentView(mMozView);
    return pNewView;
}

void UIListener::SizeTo(PRUint32 width, PRUint32 height)
{
    HWND hParentWnd = (HWND)mMozView->GetParentWindow();
    HWND hWnd = (HWND)mMozView->GetNativeWindow();
    RECT parentRect;
    RECT rect;
    ::GetWindowRect(hParentWnd, &parentRect);
    ::GetWindowRect(hWnd, &rect);
    ::SetWindowPos(hParentWnd, 0, 0, 0,
        width + (parentRect.right - parentRect.left) - (rect.right - rect.left),
        height + (parentRect.bottom - parentRect.top) - (rect.bottom - rect.top),
        SWP_NOMOVE | SWP_NOZORDER);
}

void UIListener::SetVisibility(PRBool visible)
{
    HWND hWnd = (HWND)mMozView->GetParentWindow();
    ::ShowWindow(hWnd, visible ? SW_SHOW : SW_HIDE);
}

void UIListener::StartModal()
{
    gDoModal = true;
    MSG msg;
    MozView* parentView = mMozView->GetParentView();
    ::EnableWindow((HWND)parentView->GetParentWindow(), FALSE);
    while (GetMessage(&msg, NULL, 0, 0) && gDoModal == true)
	{
		if(msg.message == WM_DESTROY)
			gDoModal = false;
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

void UIListener::ExitModal(nsresult result)
{
    MozView* parentView = mMozView->GetParentView();
    ::EnableWindow((HWND)parentView->GetParentWindow(), TRUE);
    HWND hWnd = (HWND)mMozView->GetParentWindow();
    DestroyWindow(hWnd);
}