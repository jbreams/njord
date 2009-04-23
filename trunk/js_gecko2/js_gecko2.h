// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the JS_GECKO2_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// JS_GECKO2_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef JS_GECKO2_EXPORTS
#define JS_GECKO2_API __declspec(dllexport)
#else
#define JS_GECKO2_API __declspec(dllimport)
#endif

enum GECKO_CALLBACK {
	GECKO_SETSTATUS,
	GECKO_SIZEBROWSERTO,
	GECKO_SHOWASMODAL,
	GECKO_EXITMODAL,
	GECKO_DOCUMENTLOADED,
	GECKO_LOCATION,
	GECKO_VISIBILITY,
	GECKO_SETTITLE,
	GECKO_OPENURI
};

class MozViewListener;
class WindowCreator;

class nsIDOMWindow2;
class nsIDOMWindowInternal;
class nsIInterfaceRequestor;
class nsIWebNavigation;

class njEncap
{
public:
	njEncap();
	~njEncap();

	BOOL HandleCallback(GECKO_CALLBACK callbackType, unsigned int argc, jsval * argv, jsval * rval);
	HWND GetWndHandle(BOOL parent);
	JSContext * getContext();

	class Private;
	Private * mPrivate;
};