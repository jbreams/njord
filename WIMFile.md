The WIMFile class encapsulates a WIM file. It exposes functions for applying and capturing images and for showing a progress dialog to the user. It is created by a call to [WIMCreateFile](WIMCreateFile.md).

## Members ##
|**Name**|**Type**|**Description**|
|:-------|:-------|:--------------|
|[CaptureImage](WIMCaptureImage.md) | Function | Captures an image from a mounted filesystem |
|[DeleteImage](WIMDeleteImage.md) | Function | Deletes an image stored in the WIM file |
|[StartUI](WIMStartUI.md) | Function | Starts the UI thread if it's not already started and creates a progress dialog box |
|[StopUI](WIMStopUI.md) | Function | Destroys the progress dialog box and stops the UI thread if there are no more boxes |
|[LoadImage](WIMLoadImage.md) | Function | Loads an image stored in the WIM file and returns it as a [WIMImage](WIMImage.md) object |
|[SetTemporaryPath](WIMSetTemporaryPath.md) | Function | sets the path the WIMG engine should use for storing temporary file. This should be done before calling [LoadImage](WIMLoadImage.md) |
|[SetExceptionsList](WIMSetExceptionsList.md) | Function | Sets a list of exceptions to be used during image capture |
|[Close](WIMClose.md) | Function | Closes the WIM file and cleans up all resources |
|[imageCount](WIMGetImageCount.md) | Read-only Property (UInt32) | Gets the number of images stored in the WIM file |
|[information](WIMInformation.md) | Property (String) | The XML metadata associated with the loaded WIM file. Must call [SetTemporaryPath](WIMSetTemporaryPath.md) to set this |