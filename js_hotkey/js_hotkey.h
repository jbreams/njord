// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the JS_HOTKEY_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// JS_HOTKEY_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef JS_HOTKEY_EXPORTS
#define JS_HOTKEY_API __declspec(dllexport)
#else
#define JS_HOTKEY_API __declspec(dllimport)
#endif

// This class is exported from the js_hotkey.dll
class JS_HOTKEY_API Cjs_hotkey {
public:
	Cjs_hotkey(void);
	// TODO: add your methods here.
};

extern JS_HOTKEY_API int njs_hotkey;

JS_HOTKEY_API int fnjs_hotkey(void);
