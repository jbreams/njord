class PrivateData {
public:
	PrivateData()
	{
		InitializeCriticalSection(&mCriticalSection);
		next = NULL;
		initialized = FALSE;
	}
	~PrivateData()
	{
		DeleteCriticalSection(&mCriticalSection);
	}

	WebBrowserChrome * mChrome;
	RECT requestedRect;
	BOOL initialized;
	PrivateData * next;
	CRITICAL_SECTION mCriticalSection;

};