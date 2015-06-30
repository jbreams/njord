The ProgressDlg object encapsulates a progress window with a progress bar and a static text control. The object is created using [CreateProgressDlg](CreateProgressDlg.md). While the object actually encapsulates an HWND handle, there is also a thread running to handle the message pump for the window. The thread and the window will be stopped and cleaned up when the object is finalized. The object has no properties, but exposes several methods for manipulating the window.

  * [SetStaticText](SetStaticText.md)
  * [SetProgressBar](SetProgressBar.md)
  * [Show](ProgressDlgShow.md)