#include "nsIDOMEventListener.h"

class PrivateData;

class DOMEventListener : public nsIDOMEventListener
{
public:
	DOMEventListener(PrivateData * aOwner);
	virtual ~DOMEventListener();

	NS_DECL_ISUPPORTS
	NS_DECL_NSIDOMEVENTLISTENER

private:
	PrivateData * mOwner;
};