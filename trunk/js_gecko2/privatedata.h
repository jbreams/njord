class PrivateData {
public:
	PrivateData()
	{
		InitializeCriticalSection(&mCriticalSection);
		initialized = FALSE;
	}
	~PrivateData()
	{
		DeleteCriticalSection(&mCriticalSection);
	}

	HWND mParentWindow;
	PrivateData * mParentView;

	BOOL CreateBrowser(PRUint32 aChromeFlags);

	nsCOMPtr<nsIWebBrowser> mWebBrowser;
	nsCOMPtr<nsIDOMWindow2> mDOMWindow;
	nsCOMPtr<nsIWebNavigation> mWebNavigation;
	nsCOMPtr<nsIWebBrowserChrome> mChrome;
	CRITICAL_SECTION mCriticalSection;
	PrivateData * next;
	BOOL initialized;
	RECT requestedRect;
};