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
#include "nsIWebNavigation.h"
#include "nsIWidget.h"
#include "privatedata.h"
#include "nsIWindowCreator2.h"
#include "nsIWindowWatcher.h"
#include "nsIWebBrowserChrome.h"

BOOL keepUIGoing = FALSE;
CRITICAL_SECTION viewsLock;
PrivateData * viewsHead = NULL;

LRESULT CALLBACK MozViewProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
	default:
		return ::DefWindowProc(hWnd, message, wParam, lParam);
	}
}

BOOL PrivateData::CreateBrowser(PRUint32 aChromeFlags)
{
	nsresult rv;
	EnterCriticalSection(&mCriticalSection);
	mWebBrowser = do_CreateInstance(NS_WEBBROWSER_CONTRACTID, &rv);
	nsCOMPtr<nsIBaseWindow> baseWindow = do_QueryInterface(mWebBrowser);
	rv = baseWindow->InitWindow(mParentWindow, NULL, requestedRect.left, requestedRect.top,
		requestedRect.right - requestedRect.left, requestedRect.bottom - requestedRect.top);

	nsIWebBrowserChrome ** aNewWindow = getter_AddRefs(mChrome);
	CallQueryInterface(static_cast<nsIWebBrowserChrome*>(new WebBrowserChrome(mParentWindow)), aNewWindow);

	rv = mWebBrowser->SetContainerWindow(mChrome);
	rv = mChrome->SetWebBrowser(mWebBrowser);
	rv = mChrome->SetChromeFlags(aChromeFlags);

	if(aChromeFlags * (nsIWebBrowserChrome::CHROME_OPENAS_CHROME | nsIWebBrowserChrome::CHROME_OPENAS_DIALOG))
	{
		nsCOMPtr<nsIDocShellTreeItem>docShellItem(do_QueryInterface(baseWindow));
		docShellItem->SetItemType(nsIDocShellTreeItem::typeChromeWrapper);
	}

	rv = baseWindow->Create();
	rv = baseWindow->SetVisibility(PR_TRUE);

	nsCOMPtr<nsIDOMWindow> domWindow;
	rv = mWebBrowser->GetContentDOMWindow(getter_AddRefs(domWindow));
	mDOMWindow = do_QueryInterface(domWindow);

	mWebNavigation = do_QueryInterface(baseWindow);

	nsCOMPtr<nsIWebProgressListener> listener = do_QueryInterface(mChrome);
	nsCOMPtr<nsIWeakReference> thisListener(do_GetWeakReference(listener));
	rv = mWebBrowser->AddWebBrowserListener(thisListener, NS_GET_IID(nsIWebProgressListener));

	nsCOMPtr<nsIWebBrowserFocus> browserFocus;
	browserFocus = do_QueryInterface(mWebBrowser);
	rv = browserFocus->Activate();
	LeaveCriticalSection(&mCriticalSection);
	initialized = TRUE;
	return TRUE;
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

	HWND parentWindow;	
	WebBrowserChrome * mParent = static_cast<WebBrowserChrome*> (parent);
	mParent->GetSiteWindow((void**)&parentWindow);
	HWND newWnd = ::CreateWindowW(TEXT("NJORD_GECKOEMBED_2"), TEXT("Gecko"), WS_OVERLAPPEDWINDOW, 0, 0, 800, 600, 
		parentWindow, NULL, GetModuleHandle(NULL), NULL);

	PrivateData * newPrivateData = new PrivateData();
	newPrivateData->mParentWindow = newWnd;
	newPrivateData->CreateBrowser(0);

	EnterCriticalSection(&viewsLock);
	newPrivateData->next = viewsHead;
	viewsHead = newPrivateData;
	LeaveCriticalSection(&viewsLock);

	*_retval = newPrivateData->mChrome;
	NS_IF_ADDREF(*_retval);
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
				curView->mParentWindow = ::CreateWindowW(TEXT("NJORD_GECKOEMBED_2"), TEXT("Gecko"), WS_OVERLAPPEDWINDOW, 0, 0, 800, 600, NULL, NULL, GetModuleHandle(NULL), NULL);
				curView->requestedRect.top = 0;
				curView->requestedRect.left = 0;
				curView->requestedRect.right = 800;
				curView->requestedRect.bottom = 600;
				curView->CreateBrowser(0);
				ShowWindow(curView->mParentWindow, SW_SHOW);
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
	}

	UnregisterClassW(TEXT("NJORD_GECKOEMBED_2"), GetModuleHandle(NULL));
	return 0;
}