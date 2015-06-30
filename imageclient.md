
```
SetDllDirectory("x:\\Program Files\\nJord\\libs");
LoadNative('js_gecko2.dll');
LoadNative('js_wimg.dll');
LoadNative('js_wmi.dll');
if(!GeckoInit("x:\\Program Files\\GRE\\xpcom.dll"))
	MessageBox("Failed to init gecko");

var geckoView = GeckoCreateView(640,480);
var wmiSvc = WbemConnect("root\\cimv2");
	
function already_has_idrive()
{
	var partEnum = wmiSvc.ExecQuery("select DiskIndex, Index, Type from Win32_DiskPartition");
	var part = partEnum.Next();
	while(part != false)
	{
		if(part.DiskIndex == 0 && part.Index > 0 && part.Type == "Installable File System")
			return true;
		part = partEnum.Next();
	}
	return false;
}

function build_idrive_script()
{
	var diskEnum = wmiSvc.OpenClass("Win32_DiskDrive");
	var disk = diskEnum.Next();
	if(disk == false)
		return false;
	var diskSize = disk.Size;
	diskSize = (diskSize / 1048576) / 2;
	diskSize = Math.floor(diskSize);
	var diskPartScript = "select disk 0\r\nclean\r\ncreate partition primary size " + diskSize + " noerr\r\nformat label=\"System Volume\" quick noerr override\r\nactive\r\nassign letter=c\r\ncreate partition primary size " + diskSize + " noerr\r\nformat label=\"User Data\" quick noerr override\r\nassign letter=i";
	return diskPartScript;
}

function apply_image()
{	
	imagePath = GetOpenFileName("y:\\", "wim", "Select an Image File", new Array('Windows Image File', '*.wim'));
	var wimFile = WIMCreateFile(imagePath.filePath);
	if(wimFile == false)
	{
		MessageBox("Error opening WIM file.");
		return;
	}
	SetCurrentDirectory("x:\\Program Files\\nJord");
	hasIDrive = already_has_idrive();
	var warningPage = "<html><head><title>iDrive Detected</title></head><body><h1>iDrive Detected!</h1><p>This computer already has an I-Drive that may contain user documents. You can now select to <b>erase that drive and all the documents on it</b> or <b>leave the drive untouched</b></p><p><input type='radio' name='option' value='makebothdrives'/> Erase the hard drive completely and create an iDrive and a System Drive.</p>";
	if(hasIDrive)
		warningPage += "<p><input type='radio' name='option' value='keepidrive'> Erase the system volume but leave the iDrive alone.</p>";
	warningPage += "<p><input type='radio' name='option' value='noidrive'> I don't want an iDrive, erase the harddrive and make one big partition.</p><p><input type='submit' value='Confirm' id='submitbutton'/></p></body></html>";
	geckoView.LoadData(warningPage, "file:///x:/Program Files/nJord", 'submitbutton', 'click');
	var hardDriveChoice = geckoView.GetInput('option');

	var workingPage = "<html><head><title>Working</title></head><body><h1>Please wait...</h1><p>Please wait while the hard drive is prepared for imaging, this may take a minute. When the hard drive is ready, this window will close and imaging will begin.</p></body></html>";
	geckoView.LoadData(workingPage, "file:///x:/Program Files/nJord");
	var zisdResult = Exec("zisdclear.exe", true, true, false, false);
	if(zisdResult != 0)
		MessageBox("Error occurred during image safe data clearing: " + GetLastErrorMessage(zisdResult));
	if(hardDriveChoice == 'keepidrive')
		var diskPartScript = "select disk 0\r\nselect partition 1\r\nformat fs=ntfs label=\"System Volume\" quick override noerr\r\nactive\r\nexit";
	else if(hardDriveChoice == 'makebothdrives')
		var diskPartScript = build_idrive_script(wmiSvc);
	else if(hardDriveChoice == 'noidrive')
		var diskPartScript = "select disk 0\r\nclean\r\ncreate partition primary noerr\r\nformat label=\"System Volume\" quick override noerr\r\nactive\r\nassign letter=c";

	FileSetContents('dps.s', diskPartScript);
	Exec("diskpart /s dps.s");
	DeleteFile("dps.s");
	
	CreateDirectory("c:\\imagingtemp");
	wimFile.SetTemporaryPath("c:\\imagingtemp");
	var image = wimFile.LoadImage(1);
	geckoView.Destroy();
	wimFile.StartUI();
	if(!image.ApplyImage("c:\\", WIM_FLAG_FILEINFO))
		MessageBox(GetLastErrorMessage());
	wimFile.StopUI();
	RemoveDirectory("c:\\imagingtemp");
	MessageBox("Imaging complete!");
	return	
}

function take_image()
{
	var diskEnum = wmiSvc.ExecQuery("select Name  from Win32_LogicalDisk where DriveType=3 and MediaType=12");
	var disks = new Array();
	var curDisk = diskEnum.Next();
	while(curDisk != false)
	{
		disks.push(curDisk.Name);
		curDisk = diskEnum.Next();
	}
	var diskSelectionPage = "<html><head><title>Setup Image Layout</title></head><body><h1>Image Layout</h1>" +
		"<p>Please select the local disk you want to capture. When finished, click continue.</p><ul>";
	for(i = 0; i < disks.length; i++)
		diskSelectionPage += "<li><input type=radio value='" + disks[i] + "' name='diskselector'> Local Disk " + disks[i] + "</li>";
	diskSelectionPage += "</ul><input id='continue' type='submit' value='Continue'></body></html>";
	
	geckoView.LoadData(diskSelectionPage, "file:///c:/Program Files/nJord", 'continue', 'click');
	var workTodo = geckoView.GetInput('diskselector');
	var diskSelectionPage = "<html><head><title>Select an Image</title><body><h1>Select an Image</h1><p>Please select a target image.</p></body></html>";
	geckoView.LoadData(diskSelectionPage, "file:///c:/Program Files/nJord");
	var imagePath = GetOpenFileName("y:\\", "wim", "Select an Image File", new Array('Windows Image File', '*.wim'), true);
	SetCurrentDirectory("x:\\Program Files\\nJord");
	var summaryPage = "<html><head><title>Summary</title><body><h1>Summary</h1><p>A summary of the image that is about to be created is listed below. " + 
		"Click continue to create the image.</p><p>Image path: " + imagePath.filePath + "</p><p>Capturing the following local disks to the image:</p>" +
		"<p>Capturing local disk " + workTodo + "</p><input type='submit' value='Continue' id='continue'></body></html>";
	geckoView.LoadData(summaryPage, "file:///c:/Program Files/nJord", 'continue', 'click');
	geckoView.Destroy();
	
	var wimFile = WIMCreateFile(imagePath.filePath, WIM_GENERIC_WRITE | WIM_GENERIC_READ, WIM_CREATE_ALWAYS, WIM_FLAG_VERIFY, WIM_COMPRESS_LXZ);
	if(wimFile == false)
	{
		MessageBox("Error creating WIM file instance. Epic fail.", "nJord Imaging Platform");
		return;
	}
	
	wimFile.StartUI();
	wimFile.SetTemporaryPath("y:\\temp");
	wimFile.SetExceptionsList(new Array("$ntfs.log", "hiberfil.sys", "pagefile.sys", "\\System Volume Information\\*", "\\RECYCLER\\*", "\\Windows\\CSC\\*"));
	var newImage = wimFile.CaptureImage(workTodo);
	wimFile.StopUI();
	if(newImage == false)
		MessageBox("Error creating image: " + GetLastErrorMessage());
	else
		newImage.Close();
	wimFile.StopUI();
	wimFile.Close();
}

function showError(errorMessage)
{
	var errorPage = "<html><head><title>Error</title></head><body><h1>Error</h1><p>" + errorMessage.toString() + "</p></body></html>";
	geckoView.LoadData(errorPage, "file:///x:/Program Files/nJord");
	Sleep(3000);
}

function auto_image()
{
	var welcomePage = "<html><head><title>Welcome to Auto Imaging</title></head><body><h1>Please wait...</h1><p>while the nJord imaging client decides how to image your computer.</p></body></html>";
	geckoView.LoadData(welcomePage, "file:///x:/Program Files/nJord");
	
	var configFile = new File();
	if(!configFile.Create("y:\\config.xml"))
	{
		showError("The configuration file for automatic imaging was not found. Reverting to manual imaging.");
		apply_image();
		return;
	}
	var configText = configFile.Read();
	configFile.Close();
	
	var mnQuery = wmiSvc.ExecQuery("select Name from Win32_ComputerSystemProduct");
	var mnResult = mnQuery.Next();
		
	var configXML = new XML(configText);
	var myConfig = configXML.target.(model== mnResult.Name);
	if(!myConfig.@image)
	{
		showError("A configuration file was found, but no image file was defined for this model. Reverting to manual imaging.");
		apply_image();
		return;
	}
	var wimFile = WIMCreateFile(myConfig.@image);
	if(wimFile == false)
	{
		showError("The imaging client was unable to open the image file. The returned error message was " + GetLastErrorMessage());
		return;
	}
	switch(myConfig.@iDrive)
	{
		default:
		case 'hasidrive':
			if(already_has_idrive() == true)
			{
				var askPage = "<html><head><title>iDrive Detected</title></head><body><h1>User Data Drive Detected</h1><p>The imaging client has detected that this computer already has a volume that may contain user data. Before imaging can proceed, you must specifiy whether the client should overwrite that volume. Overwriting the volume will remove any viruses that may be on the volume, but all user data will be erased.</p><p>Check the box on the right if you want the user data volume to be erased during imaging. Otherwise leave it blank. There will be no further warning prior to the volumes destruction. <input type='checkbox' name='eraseok'></p><p>Click continue when ready.</p><p><input type='submit' value='Continue' id='submit'></p></body></html>";
				geckoView.LoadData(askPage, 'file:///x:/Program Files/nJord', 'submit', 'click');
				if(geckoView.GetInput('eraseok') == true)
					var diskPartScript = build_idrive_script();
				else
					var diskPartScript = "select disk 0\r\nselect partition 1\r\nformat label=\"System Volume\" quick override\r\nactive\r\nassign letter=c";
			}
			else
				diskPartScript = build_idrive_script();
				break;
		case 'noidrive':
			var partEnum = wmiSvc.ExecQuery("Index from Win32_DiskPartition where DiskIndex = 0");
			if(partEnum.Next() != false)
				var diskPartScript = "select disk 0\r\nselect partition 1\r\nformat label=\"System Volume\" quick override\r\nactive\r\nassign letter=c";
			else
				var diskPartScript = "select disk 0\r\nclean\r\ncreate partition primary\r\nformat label=\"System Volume\" quick override\r\nactive\r\n assign letter=c";
			break;
	}
	
	FileSetContents("dps.s", diskPartScript);
	
	var workingPage = "<html><head><title>Working...</title></head><body><h1>Working</h1><p>Please wait while the imaging client prepares the hard drive to receive the image. Imaging will commence immediately after the hard drive has been prepared.</p></body></html>";
	geckoView.LoadData(workingPage, "file:///x:/Program Files/nJord");
	Exec("diskpart /s dps.s");
	
	CreateDirectory("c:\\imagingtemp");
	wimFile.SetTemporaryPath("c:\\imagingtemp");
	var image = wimFile.LoadImage(1);
	geckoView.Destroy();
	wimFile.StartUI();
	if(!image.ApplyImage("c:\\", WIM_FLAG_FILEINFO))
		MessageBox(GetLastErrorMessage());
	RemoveDirectory("c:\\imagingtemp");
	wimFile.StopUI();
}
var welcomePage = "<html><head><title>Welcome to nJord Imaging!</title></head><body><h1>Welcome to nJord Imaging!</h1><p>Please select a mode of operation.</p><ul><li><input type='radio' name='mode' value='Auto' checked> Automaticall Image Your Computer</li><input type='radio' name='mode' value='Apply Image' /> Apply an image</li><li><input type='radio' name='mode' value='Make an Image' /> Make an Image</li></ul><input type='submit' value='Continue' id='continuebutton' /></body></html>";
geckoView.LoadData(welcomePage, "file:///x:/Program Files/nJord");
var continueButton = geckoView.GetElementByID('continuebutton');
geckoView.RegisterEvent(continueButton, 'click');

var hasYDrive = false;
var yDriveEnum = wmiSvc.ExecQuery("select ProviderName from Win32_LogicalDisk where DeviceID='Y:'");
var yDrive = yDriveEnum.Next();
if(yDrive != false && yDrive.ProviderName == '\\\\kimmel\\images')
	Exec("net use y: /delete");

geckoView.WaitForStuff(continueButton, 'click');
if(geckoView.GetInput("mode") == "Make an Image")
{
	var loginPage = "<html><head><title>Please login</title></head><body><h1>Please login</h1><p>You may only create images if you are logged in as an administrator. Please login using your administrative credentials below to gain access to the imaging server.</p><p>Username: <input type=text name='username'></p><p>Password:<input type=password name='password'></p><p><input id='login' type=submit value='Login'></p><p><font color=red id=errorplace></font></p>";
	geckoView.LoadData(loginPage, 'file:///x:/Program Files/nJord', 'login');
	var useResult = Exec("net use y: \\\\kimmel\\images /user:drew-ad\\" + geckoView.GetInput('username') + " " + geckoView.GetInput('password'));
	while(useResult.exitCode != 0)
	{
		var errorPlace = geckoView.GetElementByID('errorplace');
		errorPlace.text = useResult.stdErr;
		geckoView.WaitForThings();
		useResult = Exec("net use y: \\\\kimmel\\images /user:drew-ad\\" + geckoView.GetInput('username') + " " + geckoView.GetInput('password'));
	}
	take_image();
}
else
{
	var useResult = Exec("net use y: \\\\kimmel\\images /user:kimmel\\imager password");
	if(useResult.exitCode != 0)
	{
		MessageBox("Unable to map network drive\n" + useResult.stdErr + "\n\nnJord will open a command prompt so you can map the drive manually.");
		Exec("cmd.exe", true, false, false, false);
	}
	switch(geckoView.GetInput("mode"))
	{
		case 'Apply Image':
			apply_image();
			break;
		case 'Auto':
			auto_image();
			break;
	}
}
```