#include "stdafx.h"
#include "js_gecko2.h"
#include "WebBrowserChrome.h"

#include "nsStringAPI.h"
#include "nsIComponentManager.h"
#include "nsIDOMWindow.h"
#include "nsIGenericFactory.h"
#include "nsIInterfaceRequestor.h"
#include "nsIRequest.h"
#include "nsIURI.h"
#include "nsIWebProgress.h"
#include "nsCWebBrowser.h"
#include "nsIProfileChangeStatus.h"

// Glue APIs (not frozen, but safe to use because they are statically linked)
#include "nsComponentManagerUtils.h"

// NON-FROZEN APIS!
#include "nsIWebNavigation.h"

LRESULT CALLBACK MozViewProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
ATOM wndClassAtom = 0;

WebBrowserChrome::WebBrowserChrome(HWND myWnd)
{
	mNativeWindow = myWnd;
	mSizeSet = PR_FALSE;
}

WebBrowserChrome::~WebBrowserChrome()
{
	DestroyWindow(mNativeWindow);
}

//*****************************************************************************
// WebBrowserChrome::nsISupports
//*****************************************************************************   

NS_IMPL_ADDREF(WebBrowserChrome)
NS_IMPL_RELEASE(WebBrowserChrome)

NS_INTERFACE_MAP_BEGIN(WebBrowserChrome)
NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIWebBrowserChrome)
NS_INTERFACE_MAP_ENTRY(nsIInterfaceRequestor)
NS_INTERFACE_MAP_ENTRY(nsIWebBrowserChrome)
NS_INTERFACE_MAP_ENTRY(nsIEmbeddingSiteWindow)
NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
NS_INTERFACE_MAP_END

//*****************************************************************************
// WebBrowserChrome::nsIInterfaceRequestor
//*****************************************************************************   

NS_IMETHODIMP WebBrowserChrome::GetInterface(const nsIID &aIID, void** aInstancePtr)
{
	NS_ENSURE_ARG_POINTER(aInstancePtr);

	*aInstancePtr = 0;
	if (aIID.Equals(NS_GET_IID(nsIDOMWindow)))
	{
		if (mWebBrowser)
		{
			return mWebBrowser->GetContentDOMWindow((nsIDOMWindow **) aInstancePtr);
		}
		return NS_ERROR_NOT_INITIALIZED;
	}
	return QueryInterface(aIID, aInstancePtr);
}

//*****************************************************************************
// WebBrowserChrome::nsIWebBrowserChrome
//*****************************************************************************   
/*NS_IMETHODIMP WebBrowserChrome::SetStatus(PRUint32 aType, const PRUnichar* aStatus)
{
WebBrowserChromeUI::UpdateStatusBarText(this, aStatus);
return NS_OK;
}*/

NS_IMETHODIMP WebBrowserChrome::GetWebBrowser(nsIWebBrowser** aWebBrowser)
{
	NS_ENSURE_ARG_POINTER(aWebBrowser);
	*aWebBrowser = mWebBrowser;
	NS_IF_ADDREF(*aWebBrowser);
	return NS_OK;
}

NS_IMETHODIMP WebBrowserChrome::SetWebBrowser(nsIWebBrowser* aWebBrowser)
{
	mWebBrowser = aWebBrowser;
	return NS_OK;
}

NS_IMETHODIMP WebBrowserChrome::GetChromeFlags(PRUint32* aChromeMask)
{
	*aChromeMask = mChromeFlags;
	return NS_OK;
}

NS_IMETHODIMP WebBrowserChrome::SetChromeFlags(PRUint32 aChromeMask)
{
	mChromeFlags = aChromeMask;
	return NS_OK;
}

NS_IMETHODIMP WebBrowserChrome::DestroyBrowserWindow(void)
{
	DestroyWindow(mNativeWindow);
	return NS_OK;
}


// IN: The desired browser client area dimensions.
NS_IMETHODIMP WebBrowserChrome::SizeBrowserTo(PRInt32 aWidth, PRInt32 aHeight)
{
	/* This isn't exactly correct: we're setting the whole window to
	the size requested for the browser. At time of writing, though,
	it's fine and useful for winEmbed's purposes. */
	RECT wndSize;
	GetWindowRect(mNativeWindow, &wndSize);
	wndSize.right = aWidth + wndSize.left;
	wndSize.bottom = aHeight + wndSize.top;
	AdjustWindowRect(&wndSize, WS_OVERLAPPEDWINDOW, FALSE);
	MoveWindow(mNativeWindow, wndSize.left, wndSize.top, wndSize.right - wndSize.left, wndSize.bottom - wndSize.top, TRUE);
	mSizeSet = PR_TRUE;
	return NS_OK;
}

NS_IMETHODIMP WebBrowserChrome::ShowAsModal(void)
{
	mContinueModalLoop = PR_TRUE;
	ShowWindow(mNativeWindow, SW_SHOW);
	while(mContinueModalLoop)
	{
		MSG msg;
		if(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	mContinueModalLoop = PR_TRUE;
	return NS_OK;
}

NS_IMETHODIMP WebBrowserChrome::IsWindowModal(PRBool *_retval)
{
	*_retval = mContinueModalLoop;
	return NS_OK;
}

NS_IMETHODIMP WebBrowserChrome::ExitModalEventLoop(nsresult aStatus)
{
	mContinueModalLoop = PR_FALSE;
	while(mContinueModalLoop == PR_FALSE) ;
	mContinueModalLoop = PR_FALSE;
	return NS_OK;
}

//*****************************************************************************
// WebBrowserChrome::nsIWebBrowserChromeFocus
//*****************************************************************************

NS_IMETHODIMP WebBrowserChrome::FocusNextElement()
{
	return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP WebBrowserChrome::FocusPrevElement()
{
	return NS_ERROR_NOT_IMPLEMENTED;
}

//*****************************************************************************
// WebBrowserChrome::nsIEmbeddingSiteWindow
//*****************************************************************************   

NS_IMETHODIMP WebBrowserChrome::SetDimensions(PRUint32 aFlags, PRInt32 x, PRInt32 y, PRInt32 cx, PRInt32 cy)
{
	RECT wndSize;
	wndSize.left = x;
	wndSize.right = cx + x;
	wndSize.top = y;
	wndSize.bottom = y + cy;
	AdjustWindowRect(&wndSize, WS_OVERLAPPEDWINDOW, FALSE);
	MoveWindow(mNativeWindow, wndSize.left, wndSize.top, wndSize.right - wndSize.left, wndSize.bottom - wndSize.top, TRUE);
	return NS_OK;
}

NS_IMETHODIMP WebBrowserChrome::GetDimensions(PRUint32 aFlags, PRInt32 *x, PRInt32 *y, PRInt32 *cx, PRInt32 *cy)
{
	RECT clientSize;
	GetWindowRect(mNativeWindow, &clientSize);
	if (aFlags & nsIEmbeddingSiteWindow::DIM_FLAGS_POSITION)
	{
		*x = clientSize.left + GetSystemMetrics(SM_CXFRAME);
		*y = clientSize.top + GetSystemMetrics(SM_CYFRAME);
	}
	if (aFlags & nsIEmbeddingSiteWindow::DIM_FLAGS_SIZE_INNER ||
		aFlags & nsIEmbeddingSiteWindow::DIM_FLAGS_SIZE_OUTER)
	{
		*cx = clientSize.right - clientSize.left;
		*cy = clientSize.bottom - clientSize.top;
	}
	return NS_OK;
}

/* void setFocus (); */
NS_IMETHODIMP WebBrowserChrome::SetFocus()
{
	::SetFocus(mNativeWindow);
	return NS_OK;
}

/* attribute wstring title; */
NS_IMETHODIMP WebBrowserChrome::GetTitle(PRUnichar * *aTitle)
{
	NS_ENSURE_ARG_POINTER(aTitle);
	*aTitle = nsnull;
	return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP WebBrowserChrome::SetTitle(const PRUnichar * aTitle)
{
	SetWindowText(mNativeWindow, (LPCWSTR)aTitle);
	return NS_OK;
}

/* attribute boolean visibility; */
NS_IMETHODIMP WebBrowserChrome::GetVisibility(PRBool * aVisibility)
{
	NS_ENSURE_ARG_POINTER(aVisibility);
	*aVisibility = IsWindowVisible(mNativeWindow);
	return NS_OK;
}
NS_IMETHODIMP WebBrowserChrome::SetVisibility(PRBool aVisibility)
{
	ShowWindow(mNativeWindow, aVisibility ? SW_SHOW : SW_HIDE);
	return NS_OK;
}

/* attribute nativeSiteWindow siteWindow */
NS_IMETHODIMP WebBrowserChrome::GetSiteWindow(void * *aSiteWindow)
{
	NS_ENSURE_ARG_POINTER(aSiteWindow);

	*aSiteWindow = mNativeWindow;
	return NS_OK;
}

//*****************************************************************************
// WebBrowserChrome::nsIWebProgressListener
//*****************************************************************************   

NS_IMETHODIMP WebBrowserChrome::OnProgressChange(nsIWebProgress *progress, nsIRequest *request,
												 PRInt32 curSelfProgress, PRInt32 maxSelfProgress,
												 PRInt32 curTotalProgress, PRInt32 maxTotalProgress)
{
	return NS_OK;
}

NS_IMETHODIMP WebBrowserChrome::OnStateChange(nsIWebProgress *progress, nsIRequest *request,
											  PRUint32 progressStateFlags, nsresult status)
{
//	if ((progressStateFlags & STATE_STOP) && (progressStateFlags & STATE_IS_DOCUMENT))
//		ContentFinishedLoading();

	return NS_OK;
}


NS_IMETHODIMP WebBrowserChrome::OnLocationChange(nsIWebProgress* aWebProgress,
												 nsIRequest* aRequest,
												 nsIURI *location)
{
	return NS_OK;
}

NS_IMETHODIMP 
WebBrowserChrome::OnStatusChange(nsIWebProgress* aWebProgress,
								 nsIRequest* aRequest,
								 nsresult aStatus,
								 const PRUnichar* aMessage)
{
	return NS_OK;
}



NS_IMETHODIMP 
WebBrowserChrome::OnSecurityChange(nsIWebProgress *aWebProgress, 
								   nsIRequest *aRequest, 
								   PRUint32 state)
{
	return NS_OK;
}


NS_IMETHODIMP
WebBrowserChrome::SetStatus(PRUint32 statusType, const PRUnichar *status)
{
	return NS_ERROR_NOT_IMPLEMENTED;
}