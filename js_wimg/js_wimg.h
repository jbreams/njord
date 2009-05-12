// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the JS_WIMG_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// JS_WIMG_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef JS_WIMG_EXPORTS
#define JS_WIMG_API __declspec(dllexport)
#else
#define JS_WIMG_API __declspec(dllimport)
#endif
#include "resource.h"

struct uiInfo {
	HWND hDlg;
	HANDLE wimFile;
	LONG state;
	struct uiInfo * next;
};
DWORD uiThread(LPVOID param);

#define UISTATE_ADDED 1
#define UISTATE_RUNNING 2
#define UISTATE_DYING 4
#define UISTATE_DEAD 8

class MatchEntry
{
public:
	MatchEntry(LPWSTR copyFrom);
	~MatchEntry();

	BOOL Match(LPWSTR target, LPWSTR patternStr);
	MatchEntry * next;
private:
	LPWSTR matchString;
	DWORD matchLen;
	BOOL fileName;
};

class MatchSet
{
public:
	MatchSet();
	~MatchSet();
	BOOL MatchMe(LPWSTR fullPath);
	void AddMatch(LPWSTR pattern);
	void ClearSet();
	MatchSet * next;

private:
	CRITICAL_SECTION setLock;
	MatchEntry * entriesHead;
};

JSBool wimg_set_exceptions(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval);