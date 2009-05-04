#include "stdafx.h"
#include "webbrowserchrome.h"
#include "nsXULAppApi.h"
#include "nsXPComGlue.h"
#include "nsStringAPI.h"
#include "nsComponentManagerUtils.h"
#include "nsNetUtil.h"
#include "nsEmbedCID.h"
#include "nsIProxyObjectManager.h"
#include "nsProxiedService.h"

#include "nsIBaseWindow.h"
#include "nsIDocShellTreeItem.h"
#include "nsIDOMEventTarget.h"
#include "nsIDOMWindow2.h"
#include "nsIURI.h"
#include "nsIWeakReference.h"
#include "nsIWeakReferenceUtils.h"
#include "nsIWebBrowser.h"
#include "nsIWebBrowserFocus.h"
#include "nsIWebBrowserStream.h"
#include "nsIWebBrowserChrome.h"
#include "nsIWebNavigation.h"
#include "nsIWidget.h"
#include "privatedata.h"
#include "nsIWindowCreator2.h"
#include "nsIWindowWatcher.h"
#include "js_gecko2.h"
#include "domeventhandler.h"

BOOL keepUIGoing = FALSE;
CRITICAL_SECTION viewsLock;
PrivateData * viewsHead = NULL;
LONG ThreadInitialized = 0;

LRESULT CALLBACK MozViewProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	WebBrowserChrome * mChrome = (WebBrowserChrome*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
	nsCOMPtr<nsIWebBrowser> mBrowser;
	if(mChrome)
		mChrome->GetWebBrowser(getter_AddRefs(mBrowser));
	switch(message)
	{
	case WM_SIZE:
		if(mChrome)
		{
			RECT rect;
			GetClientRect(hWnd, &rect);
			nsCOMPtr<nsIBaseWindow> baseWindow = do_QueryInterface(mBrowser);
			baseWindow->SetPositionAndSize(rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, PR_TRUE);
		}

    case WM_ACTIVATE:
		if (mChrome) {
			nsCOMPtr<nsIWebBrowserFocus> mFocus = do_QueryInterface(mBrowser);
			switch (wParam) {
			case WA_CLICKACTIVE:
			case WA_ACTIVE:
				mFocus->Activate();
				break;
			case WA_INACTIVE:
				mFocus->Deactivate();
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
		return ::DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

DWORD UiThread(LPVOID lpParam)
{
	WNDCLASS wc;
	memset(&wc, 0, sizeof(WNDCLASS));
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = (WNDPROC)MozViewProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = GetModuleHandle(NULL);
//	wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_GECKO));
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = TEXT("NJORD_GECKOEMBED_2");
	RegisterClass(&wc);

	if(!InitGRE(NULL, (LPSTR)lpParam))
		return 1;

	InterlockedIncrement(&ThreadInitialized);
	while(keepUIGoing)
	{
		EnterCriticalSection(&viewsLock);
		PrivateData * curView = viewsHead;
		while(curView != NULL)
		{
			if(curView->initialized == FALSE)
			{
				nsresult rv;
				HWND nWnd = CreateWindowW(TEXT("NJORD_GECKOEMBED_2"), TEXT("Gecko"), WS_OVERLAPPEDWINDOW, curView->requestedRect.left, curView->requestedRect.top,
					curView->requestedRect.right - curView->requestedRect.left, curView->requestedRect.bottom - curView->requestedRect.top, NULL, NULL,
					GetModuleHandle(NULL), NULL);
				curView->mNativeWindow = nWnd;
				curView->mChrome = new WebBrowserChrome();
				curView->mChrome->CreateBrowser(nWnd);
				curView->mChrome->GetWebBrowser(getter_AddRefs(curView->mBrowser));
				curView->mDOMWindow = do_GetInterface(curView->mBrowser);
				curView->mDOMListener = new DOMEventListener(curView);
				curView->initialized = TRUE;
			}
			curView = curView->next;
		}
		LeaveCriticalSection(&viewsLock);

		MSG msg;
		if(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
			Sleep(100);
	}

	InterlockedDecrement(&ThreadInitialized);
	UnregisterClassW(TEXT("NJORD_GECKOEMBED_2"), GetModuleHandle(NULL));
	return 0;
}

JSBool g2_init(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	keepUIGoing = TRUE;
	LPSTR pathToGRE;
	JS_BeginRequest(cx);
	if(!JS_ConvertArguments(cx, argc, argv, "/s", &pathToGRE))
	{
		JS_ReportError(cx, "Error parsing arguments in g2_init");
		JS_EndRequest(cx);
		return JS_FALSE;
	}
	CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)UiThread, pathToGRE, 0, NULL);
	LONG stillWaiting = 1;
	while(stillWaiting)
	{
		Sleep(1000);
		InterlockedCompareExchange(&stillWaiting, 0, ThreadInitialized);
	}
	*rval = JSVAL_TRUE;
	return JS_TRUE;
}