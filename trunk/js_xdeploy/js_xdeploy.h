// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the JS_XDEPLOY_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// JS_XDEPLOY_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef JS_XDEPLOY_EXPORTS
#define JS_XDEPLOY_API __declspec(dllexport)
#else
#define JS_XDEPLOY_API __declspec(dllimport)
#endif

// This class is exported from the js_xdeploy.dll
class JS_XDEPLOY_API Cjs_xdeploy {
public:
	Cjs_xdeploy(void);
	// TODO: add your methods here.
};

extern JS_XDEPLOY_API int njs_xdeploy;

JS_XDEPLOY_API int fnjs_xdeploy(void);
