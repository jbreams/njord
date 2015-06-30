js\_hotkey is an extension to the nJord platform that allows script to implement custom global hotkeys that will call back a JavaScript function. For example, you could register a hotkey will spawn a registry editor when a user types Ctrl-Alt-R. Currently, the extension allows registering the control, alt, and shift buttons along with printable, non white-space, ascii characters as hotkey registrations. The extension exposes the following functions to the nJord platform:

| **Name** | **Description** |
|:---------|:----------------|
| [RegisterHotKey](RegisterHotKey.md) | Registers a hotkey with windows. |
| [UnregisterHotKey](UnregisterHotKey.md) | Unregisters a hotkey from windows. |
| [StopHotKeys](StopHotKeys.md) | Unregisters all hotkeys from windows. |