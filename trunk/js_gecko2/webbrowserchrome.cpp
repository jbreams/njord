#include "stdafx.h"
#include "WebBrowserChrome.h"
#include "nsStringAPI.h"
#include "nsIDOMWindow.h"
#include "nsIURI.h"
#include "nsComponentManagerUtils.h"
#include "nsIBaseWindow.h"
#include "nsEmbedCID.h"
#include "nsIWebNavigation.h"
#include "nsMemory.h"

LRESULT CALLBACK MozViewProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
ATOM wndClassAtom = 0;

WebBrowserChrome::WebBrowserChrome()
{
	mNativeWindow = NULL;
	mSizeSet = PR_FALSE;
	mDocumentLoaded = PR_FALSE;
	mContinueModalLoop = PR_FALSE;
	mCurrentLocation = NULL;
	mChromeFlags = 0;
}

WebBrowserChrome::~WebBrowserChrome()
{

}

NS_IMPL_ISUPPORTS6(WebBrowserChrome,
				   nsIWebBrowserChrome,
				   nsIWebBrowserChromeFocus,
				   nsIInterfaceRequestor,
				   nsIEmbeddingSiteWindow,
				   nsIWebProgressListener,
				   nsSupportsWeakReference)

nsresult WebBrowserChrome::CreateBrowser(PRInt32 aX, PRInt32 aY, PRInt32 aCX, PRInt32 aCY)
{
	nsresult rv;
	mWebBrowser = do_CreateInstance(NS_WEBBROWSER_CONTRACTID, &rv);
	if(NS_FAILED(rv))
		return NS_ERROR_FAILURE;

	rv = mWebBrowser->SetContainerWindow((nsIWebBrowserChrome*)this);
	nsCOMPtr<nsIBaseWindow> browserBaseWindow = do_QueryInterface(mWebBrowser);

	RECT windowRect;
	windowRect.left = aX;
	windowRect.top = aY;
	windowRect.right = aX + aCX;
	windowRect.bottom = aY + aCY;
	AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);
	mNativeWindow = CreateWindowW(TEXT("NJORD_GECKOEMBED_2"), TEXT("Gecko"), WS_OVERLAPPEDWINDOW, windowRect.left, 
		windowRect.top,windowRect.right - windowRect.left, windowRect.bottom - windowRect.top, NULL, NULL, GetModuleHandle(NULL), NULL);
	ShowWindow(mNativeWindow, SW_SHOW);
	UpdateWindow(mNativeWindow);

	if(!mNativeWindow)
		return NS_ERROR_FAILURE;
	SetWindowLongPtrW(mNativeWindow, GWLP_USERDATA, (LONG_PTR)this);

	rv = browserBaseWindow->InitWindow(mNativeWindow, nsnull, windowRect.left, windowRect.top, 
		windowRect.right - windowRect.left, windowRect.bottom - windowRect.top);
	rv = browserBaseWindow->Create();
	nsCOMPtr<nsIWebProgressListener> listener(static_cast<nsIWebProgressListener*>(this));
	nsCOMPtr<nsIWeakReference> thisListener(do_GetWeakReference(listener));
	rv = mWebBrowser->AddWebBrowserListener(thisListener, NS_GET_IID(nsIWebProgressListener));

	return NS_OK;
}

NS_IMETHODIMP WebBrowserChrome::GetInterface(const nsIID &aIID, void** aInstancePtr)
{
    NS_ENSURE_ARG_POINTER(aInstancePtr);

    *aInstancePtr = 0;
    if (aIID.Equals(NS_GET_IID(nsIDOMWindow)))
    {
        if (!mWebBrowser)
            return NS_ERROR_NOT_INITIALIZED;

        return mWebBrowser->GetContentDOMWindow((nsIDOMWindow **) aInstancePtr);
    }
    return QueryInterface(aIID, aInstancePtr);
}

NS_IMETHODIMP WebBrowserChrome::SetStatus(PRUint32 statusType, const PRUnichar *status)
{
	return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP WebBrowserChrome::GetWebBrowser(nsIWebBrowser * *aWebBrowser)
{
    NS_ENSURE_ARG_POINTER(aWebBrowser);
    *aWebBrowser = mWebBrowser;
    NS_IF_ADDREF(*aWebBrowser);
    return NS_OK;
}

NS_IMETHODIMP WebBrowserChrome::SetWebBrowser(nsIWebBrowser * aWebBrowser)
{
    mWebBrowser = aWebBrowser;
    return NS_OK;
}

NS_IMETHODIMP WebBrowserChrome::GetChromeFlags(PRUint32 *aChromeFlags)
{
    *aChromeFlags = mChromeFlags;
    return NS_OK;
}

NS_IMETHODIMP WebBrowserChrome::SetChromeFlags(PRUint32 aChromeFlags)
{
    mChromeFlags = aChromeFlags;
    return NS_OK;
}

NS_IMETHODIMP WebBrowserChrome::DestroyBrowserWindow()
{
	if(mContinueModalLoop)
		mContinueModalLoop = PR_FALSE;
	DestroyWindow(mNativeWindow);
	return NS_OK;
}

NS_IMETHODIMP WebBrowserChrome::SizeBrowserTo(PRInt32 aCX, PRInt32 aCY)
{
	RECT rect;
	GetWindowRect(mNativeWindow, &rect);
	rect.bottom = rect.top + aCY;
	rect.right = rect.left + aCX;
	AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
	SetWindowPos(mNativeWindow, NULL, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, SWP_NOMOVE | SWP_NOZORDER);
	return NS_OK;
}

NS_IMETHODIMP WebBrowserChrome::ShowAsModal()
{
	mContinueModalLoop = PR_TRUE;
	while(mContinueModalLoop)
	{
		MSG msg;
		if(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
			Sleep(100);
	}
    return NS_OK;
}

NS_IMETHODIMP WebBrowserChrome::IsWindowModal(PRBool *_retval)
{
    NS_ENSURE_ARG_POINTER(_retval);
    *_retval = mContinueModalLoop;
    return NS_OK;
}

NS_IMETHODIMP WebBrowserChrome::ExitModalEventLoop(nsresult aStatus)
{
	mContinueModalLoop = PR_FALSE;
    return NS_OK;
}

// ----- Progress Listener -----

NS_IMETHODIMP WebBrowserChrome::OnStateChange(nsIWebProgress * /*aWebProgress*/,
                                              nsIRequest * /*aRequest*/,
                                              PRUint32 aStateFlags,
                                              nsresult /*aStatus*/)
{
    if ((aStateFlags & STATE_STOP) && (aStateFlags & STATE_IS_DOCUMENT)) {
        // if it was a chrome window and no one has already specified a size,
        // size to content
        if (!mSizeSet &&
            (mChromeFlags & nsIWebBrowserChrome::CHROME_OPENAS_CHROME)) {
            nsCOMPtr<nsIDOMWindow> contentWin;
            mWebBrowser->GetContentDOMWindow(getter_AddRefs(contentWin));
            if (contentWin)
                contentWin->SizeToContent();
            SetVisibility(PR_TRUE);
        }

		mDocumentLoaded = PR_TRUE;
    }

    return NS_OK;
}


NS_IMETHODIMP WebBrowserChrome::OnProgressChange(nsIWebProgress * /*aWebProgress*/,
                                                 nsIRequest * /*aRequest*/,
                                                 PRInt32 /*aCurSelfProgress*/,
                                                 PRInt32 /*aMaxSelfProgress*/,
                                                 PRInt32 /*aCurTotalProgress*/,
                                                 PRInt32 /*aMaxTotalProgress*/)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP WebBrowserChrome::OnLocationChange(nsIWebProgress * /*aWebProgress*/,
                                                 nsIRequest * /*aRequest*/,
                                                 nsIURI *aLocation)
{
    NS_ENSURE_ARG_POINTER(aLocation);
    nsCString spec;
    aLocation->GetSpec(spec);
	if(mCurrentLocation == NULL)
		mCurrentLocation = (LPSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, spec.Length() + 1);
	if(spec.Length() > HeapSize(GetProcessHeap(), 0, mCurrentLocation))
		mCurrentLocation = (LPSTR)HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, mCurrentLocation, spec.Length() + 1);
	strcpy_s(mCurrentLocation, HeapSize(GetProcessHeap(), 0, mCurrentLocation), spec.get());
    return NS_OK;
}

NS_IMETHODIMP WebBrowserChrome::OnStatusChange(nsIWebProgress * /*aWebProgress*/,
                                               nsIRequest * /*aRequest*/,
                                               nsresult /*aStatus*/,
                                               const PRUnichar * /*aMessage*/)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP WebBrowserChrome::OnSecurityChange(nsIWebProgress * /*aWebProgress*/,
                                                 nsIRequest * /*aRequest*/,
                                                 PRUint32 /*aState*/)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP WebBrowserChrome::SetDimensions(PRUint32 flags,
                                              PRInt32 aX, PRInt32 aY,
                                              PRInt32 aCx, PRInt32 aCy)
{
	RECT rect;
	UINT swpFlags = SWP_NOZORDER;
	if(!(flags & nsIEmbeddingSiteWindow::DIM_FLAGS_POSITION))
	{
		aX = 0;
		aY = 0;
		swpFlags |= SWP_NOMOVE;
	}
	rect.top = aY;
	rect.bottom = aY + aCy;
	rect.left = aX;
	rect.right = aX + aCx;
	AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
	SetWindowPos(mNativeWindow, NULL, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, swpFlags);
    mSizeSet = PR_TRUE;
    return NS_OK;
}

NS_IMETHODIMP WebBrowserChrome::GetDimensions(PRUint32 /*aFlags*/,
                                              PRInt32 * /*aX*/, PRInt32 * /*aY*/,
                                              PRInt32 * /*aCx*/, PRInt32 * /*aCy*/)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP WebBrowserChrome::SetFocus()
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP WebBrowserChrome::GetVisibility(PRBool * /*aVisibility*/)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP WebBrowserChrome::SetVisibility(PRBool aVisibility)
{
	ShowWindow(mNativeWindow, aVisibility ? SW_SHOW : SW_HIDE);
    return NS_OK;
}

NS_IMETHODIMP WebBrowserChrome::GetTitle(PRUnichar ** /*aTitle*/)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP WebBrowserChrome::SetTitle(const PRUnichar *aTitle)
{
	SetWindowText(mNativeWindow, aTitle);
    return NS_OK;
}

NS_IMETHODIMP WebBrowserChrome::GetSiteWindow(void * *aSiteWindow)
{
    NS_ENSURE_ARG_POINTER(aSiteWindow);
    *aSiteWindow = mNativeWindow;
    return NS_OK;
}

// ----- WebBrowser Chrome Focus

NS_IMETHODIMP WebBrowserChrome::FocusNextElement()
{
    return NS_OK;
}

NS_IMETHODIMP WebBrowserChrome::FocusPrevElement()
{
    return NS_OK;
}
