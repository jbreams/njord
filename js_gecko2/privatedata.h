class WebBrowserChrome;
class DOMEventListener;
#include "nsCOMPtr.h"

class PrivateData 
{
public:
	PrivateData()
	{
		next = NULL;
		initialized = FALSE;
		mChrome = NULL;
		mNativeWindow = NULL;
		allowClose = FALSE;
	}
	~PrivateData()
	{
	}

	WebBrowserChrome * mChrome;
	DOMEventListener * mDOMListener;
	RECT requestedRect;
	BOOL initialized;
	BOOL allowClose;
	BOOL destroying;
	HWND mNativeWindow;
	nsCOMPtr<nsIWebBrowser> mBrowser;
	nsCOMPtr<nsIDOMWindow> mDOMWindow;
	nsCOMPtr<nsIProxyObjectManager> nsIPO;
	PrivateData * next;

	JSContext * mContext;
};