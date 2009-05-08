class WebBrowserChrome;
class DOMEventListener;
#include "nsCOMPtr.h"

class EventRegistration
{
public:
	EventRegistration();
	~EventRegistration();

	LPTSTR domEvent;
	LPSTR callback;
	BOOL stopBubbles;
	HANDLE windowsEvent;

	EventRegistration * next;
};

class PrivateData 
{
public:
	PrivateData()
	{
		next = NULL;
		initialized = FALSE;
		mChrome = NULL;
		mNativeWindow = NULL;
		InitializeCriticalSection(&eventHeadLock);
		eventRegCount = 0;
		eventHead = NULL;
		allowClose = FALSE;
	}
	~PrivateData()
	{
		DeleteCriticalSection(&eventHeadLock);
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

	DWORD eventRegCount;
	EventRegistration * eventHead;
	CRITICAL_SECTION eventHeadLock;

	JSContext * mContext;
};