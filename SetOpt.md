SetOpt sets cURL options on the session encapsulated by the Curl class.

## Arguments ##
| **Name** | **Type** | **Description** |
|:---------|:---------|:----------------|
| Code     | UInt32   | The option code of the option you want to set |
| Value    | Variable | The content you want to apply to this option. |

## Return value ##
Returns true on success and an error code on failure.

## Remarks ##

The SetOpt function supports all the cURL options that are strings, integers, or boolean values. For more information about options, see the documentation for [curl\_easy\_setopt](http://curl.haxx.se/libcurl/c/curl_easy_setopt.html) at the cURL website.