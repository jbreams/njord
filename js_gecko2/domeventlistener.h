#include "nsIDOMEventListener.h"

class DOMEventListener : public nsIDOMEventListener
{
public:
	DOMEventListener(JSContext * cx);
	virtual ~DOMEventListener();

	NS_DECL_ISUPPORTS
	NS_DECL_NSIDOMEVENTLISTENER

private:
	JSContext * cx;
};