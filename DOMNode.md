A DOMNode object represents a single node or element in the DOM of the loaded document. Each object is live, so any changes made will be immediately reflected in the browser window. This provides a convenient way to change content on loaded page without loading a new document - much in the same way AJAX is used in web development. DOMNodes are also used in [RegisterEvent](RegisterEvent.md) and the other event functions to specified a node that will be an event target. DOMNodes represent all nodes, but have functions that are specific to element nodes - those functions will only succeed if the node is actually an element. Each DOMNode is very lightweight, since they are only references to the underlying XPCOM node objects and properties are implemented with getters and setters instead of stored data, so having lots of them referenced will not create an unacceptable amount of memory used.

## Methods ##
| **Name** | **Description** |
|:---------|:----------------|
| [SetAttribute](SetAttribute.md) || Sets an attribute on an element (only valid on an element). ||
| [RemoveAttribute](RemoveAttribute.md) || Removes an attribute from and element (only valid on an element). ||
| [HasAttributeAttribute](HasAttribute.md) || Returns true if an element has a specified attribute (only valid on an element). ||
| [GetElementsByTagName](GetElementsByTagName.md) || Returns an array of element nodes with the specified tag name. ||
| [InsertBefore](InsertBefore.md) || Inserts a child node before a specified reference node. ||
| [ReplaceChild](ReplaceChild.md) || Replaces a child node with another node. ||
| [RemoveChild](RemoveChild.md) || Removes a specified child node. ||
| [AppendChild](AppendChild.md) || Appends a child node. ||

## Properties ##
| **Name** | **Type** | **Description** |
|:---------|:---------|:----------------|
| name     | String   || The name of the node. ||
| tagName  | String   || The tagname of the element ||
| value| String   || The value of the node. ||
| parentNode | DOMNode  || The nodes parent. ||
| firstChild | DOMNode  || The first child of the node ||
| lastChild | DOMNode  || The last child of the node ||
| previousSibling | DOMNode  || The previous sibling of the node. ||
| nextSibling | DOMNode  || The next sibling of the node. ||
| children | Array    || An array of all the child nodes of the node. ||
| text| String   || The text content of the current node. ||
