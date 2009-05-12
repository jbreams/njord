#include "nsIDOMEventListener.h"
#include "nsIDOMEventTarget.h"
#include "nsStringApi.h"

class PrivateData;

class EventRegistration
{
public:
	EventRegistration();
	~EventRegistration();

	nsIDOMNode * target;
	nsString domEvent;
	LPSTR callback;
	HANDLE windowsEvent;

	EventRegistration * next;
};

class DOMEventListener : public nsIDOMEventListener
{
public:
	DOMEventListener(PrivateData * aOwner);
	virtual ~DOMEventListener();

	NS_DECL_ISUPPORTS
	NS_DECL_NSIDOMEVENTLISTENER

	BOOL RegisterEvent(nsIDOMNode *target, LPWSTR type, LPSTR callback);
	void UnregisterEvent(nsIDOMNode * targetNode, LPWSTR type);
	void UnregisterAll();
	BOOL WaitForSingleEvent(nsIDOMNode *target, LPWSTR type, DWORD timeout);
	BOOL WaitForAllEvents(DWORD timeout, BOOL waitAll, JSObject ** out);

private:
	PrivateData * mOwner;
	EventRegistration * head;
	DWORD regCount;
	CRITICAL_SECTION headLock;
	HANDLE addRemoveMutex;

};