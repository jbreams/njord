JS\_Gecko2 is the second version of the Mozilla Gecko Runtime for nJord. It is designed to provide an interface for rapid development of user interfaces using standard HTML. It improves over the previous version by making the Gecko views as objects rather than global functions, adding direct access to the DOM after a page has been loaded, and support for registering for callbacks and waiting for DOM events to drive the execution of nJord scripts.

## Global Functions ##
| **Name** | **Description** |
|:---------|:----------------|
| [GeckoInit](GeckoInit.md) | Initializes the GRE and starts the thread that the Gecko UI will run under. |
| [GeckoTerm](GeckoTerm.md) | Terminates the Gecko UI thread and shuts down the GRE. |
| [GeckoCreateView](GeckoCreateView.md) | Creates a new browser window and returns it to the user. |

## Classes ##
| **Name** | **Description** |
|:---------|:----------------|
| [GeckoView](GeckoView.md) | Encapsulates a single browser window. This is the main interface into the GRE |
| [DOMDocument](DOMDocument.md) | Extends DOMNode to provide access to creating new elements and other nodes as well as accessing child nodes by ID/tag name |
| [DOMNode](DOMNode.md) | Encapsulates a DOM node from the loaded HTML document. |

## Load Requirements ##
You must have a local copy of the GRE installed on the system. JS\_Gecko2 is designed to operate with the standard installation of Mozilla FireFox. The path to the GRE can be specified at runtime using [Gecko.GeckoInit] or can be specified by the environment variable GRE\_HOME. Otherwise, there are no requirements for loading the module.
