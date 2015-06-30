js\_curl provides a binding for the cURL libraries for URL handling. A cURL request is encapsulated in the Curl class.

## Methods ##

  * [SetOpt](SetOpt.md) - Sets cURL options on the cURL easy session encapsulated by the class.
  * [Perform](Perform.md) - Performs the cURL session and returns the results as a string.
  * [SaveAs](SaveAs.md) - Same as [Perform](Perform.md), but saves the returned content as a file rather than return it as a string.

## Remarks ##

The session is cleaned up automatically when the object is finalized.