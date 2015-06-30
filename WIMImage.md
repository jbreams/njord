The WIMImage class represents an image stored in a [WIMFile](WIMFile.md), it is created either by calling [CaptureImage](WIMCaptureImage.md) or [LoadImage](WIMLoadImage.md) from a valid [WIMFile](WIMFile.md).

## Members ##
| **Name** | **Type** | **Description** |
|:---------|:---------|:----------------|
| [ApplyImage](WIMApplyImage.md) | Function | Applies the image to a local filesystem |
| [Close](WIMClose.md) | Function | Manually closes the image and cleans up any resources it has opened |
| [information](WIMInformation.md) | Property | Returns the XML metadata associated with the image |