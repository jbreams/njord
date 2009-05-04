#include "stdafx.h"
// CRT headers
#include <iostream>
#include <string>
using namespace std;

#include "nsXULAppAPI.h"
#include "nsXPCOMGlue.h"
#include "nsCOMPtr.h"
#include "nsStringAPI.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsProfileDirServiceProvider.h"

#include "nsILocalFile.h"

XRE_InitEmbeddingType XRE_InitEmbedding = 0;
XRE_TermEmbeddingType XRE_TermEmbedding = 0;
XRE_NotifyProfileType XRE_NotifyProfile = 0;
XRE_LockProfileDirectoryType XRE_LockProfileDirectory = 0;
extern CRITICAL_SECTION viewsLock;
extern BOOL keepUIGoing;

static int gInitCount = 0;

// ------------------------------------------------------------------------

nsIDirectoryServiceProvider *sAppFileLocProvider = 0;
nsCOMPtr<nsILocalFile> sProfileDir = 0;
nsISupports * sProfileLock = 0;

class MozEmbedDirectoryProvider : public nsIDirectoryServiceProvider2
{
public:
    NS_DECL_ISUPPORTS_INHERITED
    NS_DECL_NSIDIRECTORYSERVICEPROVIDER
    NS_DECL_NSIDIRECTORYSERVICEPROVIDER2
};

static const MozEmbedDirectoryProvider kDirectoryProvider;

NS_IMPL_QUERY_INTERFACE2(MozEmbedDirectoryProvider,
                         nsIDirectoryServiceProvider,
                         nsIDirectoryServiceProvider2)

NS_IMETHODIMP_(nsrefcnt)
MozEmbedDirectoryProvider::AddRef()
{
    return 1;
}

NS_IMETHODIMP_(nsrefcnt)
MozEmbedDirectoryProvider::Release()
{
    return 1;
}

NS_IMETHODIMP
MozEmbedDirectoryProvider::GetFile(const char *aKey, PRBool *aPersist,
                                   nsIFile* *aResult)
{
    if (sAppFileLocProvider) {
        nsresult rv = sAppFileLocProvider->GetFile(aKey, aPersist, aResult);
        if (NS_SUCCEEDED(rv))
            return rv;
    }

    if (sProfileDir && !strcmp(aKey, NS_APP_USER_PROFILE_50_DIR)) {
        *aPersist = PR_TRUE;
        return sProfileDir->Clone(aResult);
    }

    if (sProfileDir && !strcmp(aKey, NS_APP_PROFILE_DIR_STARTUP)) {
        *aPersist = PR_TRUE;
        return sProfileDir->Clone(aResult);
    }

    if (sProfileDir && !strcmp(aKey, NS_APP_CACHE_PARENT_DIR)) {
        *aPersist = PR_TRUE;
        return sProfileDir->Clone(aResult);
    }

    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
MozEmbedDirectoryProvider::GetFiles(const char *aKey,
                                    nsISimpleEnumerator* *aResult)
{
    nsCOMPtr<nsIDirectoryServiceProvider2>
        dp2(do_QueryInterface(sAppFileLocProvider));

    if (!dp2)
        return NS_ERROR_FAILURE;

    return dp2->GetFiles(aKey, aResult);
}

BOOL InitGRE(char * aProfilePath, LPSTR pathToGRE)
{
    nsresult rv;

    ++gInitCount;
    if (gInitCount > 1)
		return TRUE;

    // Find the GRE (xul shared lib). We are only using frozen interfaces, so we
    // should be compatible all the way up to (but not including) mozilla 2.0
    static const GREVersionRange vr = {
        "1.9a1",
        PR_TRUE,
        "2.0",
        PR_FALSE
    };
    // find xpcom shared lib (uses GRE_HOME env var if set)
    char temp[MAX_PATH];
    rv = GRE_GetGREPathWithProperties(&vr, 1, nsnull, 0, temp, sizeof(temp));
    string xpcomPath(temp);
    cout << "xpcom: " << xpcomPath << endl;

	if(NS_FAILED(rv))
		rv = XPCOMGlueStartup(pathToGRE);
	else
		rv = XPCOMGlueStartup(xpcomPath.c_str());
    if (NS_FAILED(rv)) {
		return FALSE;
    }

    // get rid of the bogus TLS warnings
    NS_LogInit();

    // load XUL functions
    nsDynamicFunctionLoad nsFuncs[] = {
            {"XRE_InitEmbedding", (NSFuncPtr*)&XRE_InitEmbedding},
            {"XRE_TermEmbedding", (NSFuncPtr*)&XRE_TermEmbedding},
            {"XRE_NotifyProfile", (NSFuncPtr*)&XRE_NotifyProfile},
            {"XRE_LockProfileDirectory", (NSFuncPtr*)&XRE_LockProfileDirectory},
            {0, 0}
    };

    rv = XPCOMGlueLoadXULFunctions(nsFuncs);
    if (NS_FAILED(rv)) {
		return FALSE;
    }

    // strip the filename from xpcom so we have the dir instead
    size_t lastslash = xpcomPath.find_last_of("/\\");
    if (lastslash == string::npos) {
		return FALSE;
    }
    string xpcomDir = xpcomPath.substr(0, lastslash);

    // create nsILocalFile pointing to xpcomDir
    nsCOMPtr<nsILocalFile> xuldir;
    rv = NS_NewNativeLocalFile(nsCString(xpcomDir.c_str()), PR_FALSE,
                               getter_AddRefs(xuldir));
    if (NS_FAILED(rv)) {
		return FALSE;
	}

    // create nsILocalFile pointing to appdir
    char self[MAX_PATH];
    GetModuleFileNameA(GetModuleHandle(NULL), self, sizeof(self));
    string selfPath(self);
    lastslash = selfPath.find_last_of("/\\");
    if (lastslash == string::npos) {
		return FALSE;
    }

    selfPath = selfPath.substr(0, lastslash);

    nsCOMPtr<nsILocalFile> appdir;
    rv = NS_NewNativeLocalFile(nsCString(selfPath.c_str()), PR_FALSE,
                               getter_AddRefs(appdir));
    if (NS_FAILED(rv)) {
		return FALSE;
    }

    // setup profile dir
    if (aProfilePath) {
        rv = NS_NewNativeLocalFile(nsCString(aProfilePath), PR_FALSE,
                                   getter_AddRefs(sProfileDir));
        NS_ENSURE_SUCCESS(rv, rv);
    } else {
        // for now use a subdir under appdir
        nsCOMPtr<nsIFile> profFile;
        rv = appdir->Clone(getter_AddRefs(profFile));
        NS_ENSURE_SUCCESS(rv, rv);
        sProfileDir = do_QueryInterface(profFile);
        sProfileDir->AppendNative(NS_LITERAL_CSTRING("mozembed"));
    }

    // create dir if needed
    PRBool dirExists;
    rv = sProfileDir->Exists(&dirExists);
    NS_ENSURE_SUCCESS(rv, rv);
    if (!dirExists) {
        sProfileDir->Create(nsIFile::DIRECTORY_TYPE, 0700);
    }

    // Lock profile directory
    if (sProfileDir && !sProfileLock) {
        rv = XRE_LockProfileDirectory(sProfileDir, &sProfileLock);
        NS_ENSURE_SUCCESS(rv, rv);
    }

    // init embedding
    rv = XRE_InitEmbedding(xuldir, appdir,
                           const_cast<MozEmbedDirectoryProvider*>(&kDirectoryProvider),
                           0, NULL);
    if (NS_FAILED(rv)) {
		return FALSE;
    }

    // initialize profile:
    XRE_NotifyProfile();

    NS_LogTerm();
	InitializeCriticalSection(&viewsLock);
	return TRUE;
}

JSBool g2_term(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
    --gInitCount;
    if (gInitCount > 0)
        return JS_TRUE;

	keepUIGoing = FALSE;

    nsresult rv;

    // get rid of the bogus TLS warnings
    NS_LogInit();

    // terminate embedding
    if (!XRE_TermEmbedding) {
		*rval = JSVAL_FALSE;
        cerr << "XRE_TermEmbedding not set." << endl;
        return JS_TRUE;
    }

    XRE_TermEmbedding();

    // make sure this is freed before shutting down xpcom
    NS_IF_RELEASE(sProfileLock);
    sProfileDir = 0;

    // shutdown xpcom
    rv = XPCOMGlueShutdown();
    if (NS_FAILED(rv)) {
		*rval = JSVAL_FALSE;
        cerr << "Could not shutdown XPCOM glue." << endl;
        return JS_TRUE;
    }

    NS_LogTerm();
	DeleteCriticalSection(&viewsLock);
    return JS_TRUE;
}