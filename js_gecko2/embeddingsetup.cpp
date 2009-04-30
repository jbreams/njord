#include "stdafx.h"
#include "nsIDirectoryService.h"
#include "prtypes.h"
#include "nsXULAppAPI.h"
#include "nsXPCOMGlue.h"
#include "nsCOMPtr.h"
#include "nsStringAPI.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsProfileDirServiceProvider.h"
#include "nsILocalFile.h"

extern CRITICAL_SECTION viewsLock;
XRE_InitEmbeddingType XRE_InitEmbedding = 0;
XRE_TermEmbeddingType XRE_TermEmbedding = 0;
XRE_NotifyProfileType XRE_NotifyProfile = 0;
XRE_LockProfileDirectoryType XRE_LockProfileDirectory = 0;

nsIDirectoryServiceProvider * sAppFileLocProvider = NULL;
nsCOMPtr<nsILocalFile> sProfileDir = NULL;
nsISupports * sProfileLock = NULL;

class G2EmbedDirectoryProvider : public nsIDirectoryServiceProvider2
{
public:
	NS_DECL_ISUPPORTS_INHERITED
	NS_DECL_NSIDIRECTORYSERVICEPROVIDER
	NS_DECL_NSIDIRECTORYSERVICEPROVIDER2
};

static const G2EmbedDirectoryProvider kDirectoryProvider;

NS_IMPL_QUERY_INTERFACE2(G2EmbedDirectoryProvider,
						 nsIDirectoryServiceProvider,
						 nsIDirectoryServiceProvider2)

NS_IMETHODIMP_(nsrefcnt) G2EmbedDirectoryProvider::AddRef()
{
	return 1;
}

NS_IMETHODIMP_(nsrefcnt) G2EmbedDirectoryProvider::Release()
{
	return 1;
}

NS_IMETHODIMP G2EmbedDirectoryProvider::GetFile(const char *prop, PRBool *persistent, nsIFile **_retval)
{
	if(sAppFileLocProvider)
	{
		nsresult rv = sAppFileLocProvider->GetFile(prop, persistent, _retval);
		if(NS_SUCCEEDED(rv))
			return rv;
	}

	if(sProfileDir && !strcmp(prop, NS_APP_USER_PROFILE_50_DIR))
	{
		*persistent = PR_TRUE;
		return sProfileDir->Clone(_retval);
	}

	if(sProfileDir && !strcmp(prop, NS_APP_PROFILE_DIR_STARTUP))
	{
		*persistent = PR_TRUE;
		return sProfileDir->Clone(_retval);
	}

	if(sProfileDir && !strcmp(prop, NS_APP_CACHE_PARENT_DIR))
	{
		*persistent = PR_TRUE;
		return sProfileDir->Clone(_retval);
	}
	return NS_ERROR_FAILURE;
}

NS_IMETHODIMP G2EmbedDirectoryProvider::GetFiles(const char *prop, nsISimpleEnumerator **_retval)
{
	nsCOMPtr<nsIDirectoryServiceProvider2> dp2(do_QueryInterface(sAppFileLocProvider));
	if(!dp2)
		return NS_ERROR_FAILURE;
	return dp2->GetFiles(prop, _retval);
}

JSBool g2_init(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	nsresult rv;
	static const GREVersionRange vr = {
		"1.9a1", PR_TRUE,
		"2.0", PR_FALSE
	};
	BOOL needToFree = FALSE;
	LPSTR pathToGRE = NULL, profilePath = NULL;
	JS_BeginRequest(cx);
	if(!JS_ConvertArguments(cx, argc, argv, "/s s", &pathToGRE, &profilePath))
	{
		JS_ReportWarning(cx, "Error parsing arguments in g2_init");
		JS_EndRequest(cx);
		return JS_FALSE;
	}
	
	if(pathToGRE == NULL)
	{
		pathToGRE = (LPSTR)JS_malloc(cx, MAX_PATH);
		needToFree = TRUE;
		rv = GRE_GetGREPathWithProperties(&vr, 1, nsnull, 0, pathToGRE, MAX_PATH);
		if(NS_FAILED(rv))
		{
			JS_ReportWarning(cx, "No path to GRE was passed and the GRE cannot be found from the environment path. Cannot continue loading.");
			JS_EndRequest(cx);
			*rval = JSVAL_FALSE;
			return JS_TRUE;
		}
	}
	JS_YieldRequest(cx);
	rv = XPCOMGlueStartup(pathToGRE);

	if(NS_FAILED(rv))
	{
		JS_NewNumberValue(cx, rv, rval);
		JS_EndRequest(cx);
		if(needToFree) 
			JS_free(cx, pathToGRE);
		return JS_TRUE;
	}

	NS_LogInit();
	nsDynamicFunctionLoad nsFuncs [] = {
		{"XRE_InitEmbedding", (NSFuncPtr*)&XRE_InitEmbedding},
		{"XRE_TermEmbedding", (NSFuncPtr*)&XRE_TermEmbedding},
		{"XRE_NotifyProfile", (NSFuncPtr*)&XRE_NotifyProfile},
		{"XRE_LockProfileDirectory", (NSFuncPtr*)&XRE_LockProfileDirectory },
		{ 0, 0 }
	};

	rv = XPCOMGlueLoadXULFunctions(nsFuncs);
	if(NS_FAILED(rv))
	{
		JS_NewNumberValue(cx, rv, rval);
		JS_EndRequest(cx);
		if(needToFree) 
			JS_free(cx, pathToGRE);
		return JS_TRUE;
	}

	{
		char * lastBackslash = strrchr(pathToGRE, '\\');
		pathToGRE[lastBackslash - pathToGRE] = '\0';
		
		nsCOMPtr<nsILocalFile>xuldir;
		rv = NS_NewNativeLocalFile(nsCString(pathToGRE), PR_FALSE, getter_AddRefs(xuldir));
		if(needToFree)
			JS_free(cx, pathToGRE);
		if(NS_FAILED(rv))
		{
			JS_NewNumberValue(cx, rv, rval);
			JS_EndRequest(cx);
			return JS_TRUE;
		}

		LPSTR self = (LPSTR)JS_malloc(cx, MAX_PATH);
		GetModuleFileNameA(GetModuleHandle(NULL), self, MAX_PATH);
		LPSTR selfLastBackslash = strrchr(self, '\\');
		self[selfLastBackslash - self] = '\0';
		
		nsCOMPtr<nsILocalFile>appdir;
		rv = NS_NewNativeLocalFile(nsCString(self), PR_FALSE, getter_AddRefs(appdir));
		JS_free(cx, self);
		if(NS_FAILED(rv))
		{
			JS_NewNumberValue(cx, rv, rval);
			JS_EndRequest(cx);
			return JS_TRUE;
		}

		if(profilePath)
		{
			rv = NS_NewNativeLocalFile(nsCString(profilePath), PR_FALSE, getter_AddRefs(sProfileDir));
			NS_ENSURE_SUCCESS(rv, rv);
		}
		else
		{
			nsCOMPtr<nsIFile> profFile;
			rv = appdir->Clone(getter_AddRefs(profFile));
			NS_ENSURE_SUCCESS(rv, rv);
			sProfileDir = do_QueryInterface(profFile);
			sProfileDir->AppendNative(NS_LITERAL_CSTRING("njordgecko"));
		}

		PRBool dirExists;
		rv = sProfileDir->Exists(&dirExists);
		NS_ENSURE_SUCCESS(rv, rv);
		if(!dirExists)
			sProfileDir->Create(nsIFile::DIRECTORY_TYPE, 0700);

		if(sProfileDir && !sProfileLock)
		{
			rv = XRE_LockProfileDirectory(sProfileDir, &sProfileLock);
			NS_ENSURE_SUCCESS(rv, rv);
		}

		JS_YieldRequest(cx);
		rv = XRE_InitEmbedding(xuldir, appdir, const_cast<G2EmbedDirectoryProvider*>(&kDirectoryProvider), nsnull, 0);
		if(NS_FAILED(rv))
		{
			JS_NewNumberValue(cx, rv, rval);
			JS_EndRequest(cx);
			return JS_TRUE;
		}

		XRE_NotifyProfile();
		NS_LogTerm();
	}

	InitializeCriticalSection(&viewsLock);
	JS_EndRequest(cx);
	*rval = JSVAL_TRUE;
	return JS_TRUE;
}

JSBool g2_term(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	NS_LogInit();
	if(!XRE_TermEmbedding)
	{
		*rval = JSVAL_FALSE;
		return JS_TRUE;
	}

	XRE_TermEmbedding();
	NS_IF_RELEASE(sProfileLock);
	sProfileDir = 0;

	nsresult rv = XPCOMGlueShutdown();
	if(NS_FAILED(rv))
	{
		JS_BeginRequest(cx);
		JS_NewNumberValue(cx, rv, rval);
		JS_EndRequest(cx);
		return JS_TRUE;
	}

	NS_LogTerm();
	DeleteCriticalSection(&viewsLock);
	*rval = JSVAL_TRUE;
	return JS_TRUE;
}