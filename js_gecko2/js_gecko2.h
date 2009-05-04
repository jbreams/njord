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

JSBool g2_term(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval);
JSBool g2_init(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval);
JSBool g2_wait_for_things(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval);
JSBool g2_wait_for_stuff(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval);
JSBool g2_register_event(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval);
JSBool g2_unregister_event(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval);
JSBool g2_get_input_value(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval);
DWORD UiThread(LPVOID lpParam);
BOOL InitGRE(char * aProfilePath);
JSBool initDOMNode(JSContext * cx, JSObject * global);