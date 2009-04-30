#include "stdafx.h"
#include "webbrowserchrome.h"
#include "nsXULAppApi.h"
#include "nsXPComGlue.h"
#include "nsStringAPI.h"
#include "nsComponentManagerUtils.h"
#include "nsNetUtil.h"
#include "nsEmbedCID.h"

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

BOOL keepUIGoing = FALSE;
CRITICAL_SECTION viewsLock;
PrivateData * viewsHead = NULL;

LRESULT CALLBACK MozViewProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	WebBrowserChrome * mChrome = (WebBrowserChrome*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
	switch(message)
	{
    case WM_CLOSE:
		if(mChrome)
		{
			mChrome->ExitModalEventLoop(0);
			mChrome->DestroyBrowserWindow();
		}
		break;
    case WM_SIZE:
		if(mChrome)
		{
			RECT cRect;
			GetClientRect(hWnd, &cRect);
			mChrome->SizeBrowserTo(cRect.right - cRect.left, cRect.bottom - cRect.top);
		}
		break;
    case WM_ACTIVATE:
		if (mChrome) {
			switch (wParam) {
			case WA_CLICKACTIVE:
			case WA_ACTIVE:
				mChrome->SetFocus();
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

class WindowCreator: public nsIWindowCreator2
{
public:
	WindowCreator();
	virtual ~WindowCreator()
	{

	}

	NS_DECL_ISUPPORTS
	NS_DECL_NSIWINDOWCREATOR
	NS_DECL_NSIWINDOWCREATOR2
};

NS_IMPL_ISUPPORTS2(WindowCreator, nsIWindowCreator, nsIWindowCreator2)

NS_IMETHODIMP WindowCreator::CreateChromeWindow(nsIWebBrowserChrome *parent, PRUint32 chromeFlags, nsIWebBrowserChrome **_retval)
{
	NS_ENSURE_ARG_POINTER(_retval);
	return NS_OK;
}

NS_IMETHODIMP WindowCreator::CreateChromeWindow2(nsIWebBrowserChrome *parent, PRUint32 chromeFlags, PRUint32 contextFlags, nsIURI *uri, PRBool *cancel, nsIWebBrowserChrome **_retval)
{
	return CreateChromeWindow(parent, chromeFlags, _retval);
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

	while(keepUIGoing)
	{
		EnterCriticalSection(&viewsLock);
		PrivateData * curView = viewsHead;
		while(curView != NULL)
		{
			if(curView->initialized == FALSE)
			{
				nsresult rv;
				curView->mChrome = new WebBrowserChrome();
				NS_ADDREF(curView->mChrome);

				curView->mChrome->CreateBrowser(curView->requestedRect.left, curView->requestedRect.top, curView->requestedRect.right - curView->requestedRect.left,
					curView->requestedRect.bottom - curView->requestedRect.top);
				curView->initialized = TRUE;
			}
			curView = curView->next;
		}
		LeaveCriticalSection(&viewsLock);

		for(WORD messageCount = 0; messageCount < 500; messageCount++)
		{
			MSG msg;
			if(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
	}

	UnregisterClassW(TEXT("NJORD_GECKOEMBED_2"), GetModuleHandle(NULL));
	return 0;
}