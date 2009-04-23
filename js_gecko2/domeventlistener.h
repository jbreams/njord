#include "nsIDOMEventListener.h"

class njEncap;
class EventListener : public nsIDOMEventListener
{
public:
	EventListener(njEncap * aOwner);
	virtual ~EventListener();

	NS_DECL_ISUPPORTS
	NS_DECL_NSIDOMEVENTLISTENER

private:
	njEncap * mOwner;
};