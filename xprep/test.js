
function xd_get_state(key)
{
	statefile = new File();
	if(!statefile.Create("state.xml"))
		return null;
	var stateContents = statefile.Read();
	statefile.Close();
	if(stateContents.length == 0)
		return null;
	var state = new XML(stateContents);
	for(i = 0; i < state.*.length(); i++)
	{
		var curParam = state.param[i];
		if(curParam.@name.toString() == key)
			return curParam.toString();
	}
	return null;
}

function xd_set_state(key, value)
{
	statefile = new File();
	if(!statefile.Create("state.xml"))
	{
		stateContents = <state></state>;
		var stateContents = statefile.Read();
		statefile.Close();
	}
	var state = new XML(stateContents);
	var foundAndChanged = false;
	for(i = 0; i < state.*.length(); i++)
	{
		var curParam = state.param[i];
		if(curParam.@name.toString() == key)
		{
			foundAndChanged = true;
			curParam = value;
		}
	}
	
	if(!foundAndChanged)
	{
		var newParam = <param name={key}>{value}</param>;
		state.appendChild(newParam);
	}

	statefile.Create("c:\\xDeploy\\var\\state.xml", TRUNCATE_EXISTING);
	statefile.Write(state.toXMLString());
	statefile.Close();
}

LoadNative("js_curl.dll");
LoadNative("js_xdeploy.dll");
LoadNative("js_gecko.dll");
LoadJSLib("xmlrpc.js");
function xd_rpc_call(rpcRequest)
{
	var xmlContent = rpcRequest.xml();
	var httpRequest = "POST /rpc/ HTTP/1.0\r\n" +
		"User-Agent: xmlrpc-njord-curl\r\n" +
		"Content-Type: text/xml\r\n" +
		"Content-Length:" + xmlContent.length + "\r\n" +
		"\r\n" +
		xmlContent;
	
	var curlObj = new Curl();
	curlObj.SetOpt(CURLOPT_CAINFO, "c:/xDeploy/etc/ca-bundle.crt");
	curlObj.SetOpt(CURLOPT_CUSTOMREQUEST, httpRequest);
	curlObj.SetOpt(CURLOPT_URL, "https://xd.drew.edu/rpc/");
	curlObj.SetOpt(CURLOPT_HEADER, 0);
	xmlBuffer = curlObj.Perform();
	xmlBuffer = xmlBuffer.substring(xmlBuffer.indexOf("<methodResponse>", 0), xmlBuffer.length);
	xmlContent = new XML(xmlBuffer);
	return decode_xmlrpc(xmlContent);
}

function file_get_contents(path)
{
	var file = new File();
	file.Create(path);
	return file.Read();
}

function xd_ui(body, docTitle, timeout) {
    var head = <head>
					<title>{docTitle}</title>
					<meta http-equiv='Content-Type' content="text/html; charset=iso-8859-1" />
					<link ref='assets/styles.css' rel='stylesheet' type='text/css' />
				</head>;
	if(timeout > 0)
		head.appendChild(<meta http-equiv='refresh' content={timeout + ";nextpage.html"} />);
	
	var htmlReturn = <html></html>;
	htmlReturn.appendChild(head);
	htmlReturn.appendChild(body);
	return ShowUI(htmlReturn.toXMLString(), null, 'file:///b:/xdeploy/htdocs/');
}

function welcome01()
{
	var welcomeMessage;
	if(xd_rpc_call(new XMLRPCMessage('on_network')) === false)
	{
		welcomeMessage = new XML("<body><h1>Not on network</h1><p>This short process would customize this computer for the end user, and deploy it properly.  Unfortunately this process can only be executed properly from the Drew University campus network. Please click <a href='shutdown.html'>here</a> to shutdown your machine.</p></body>");
		xd_ui(welcomeMessage, 'eXtreme Deployment', 0);
	//	exec("shutdown /f /s /t 0");
	}
	welcomeMessage = <body><h1><font color='#666abc'>Welcome to eXtreme Deployment.</font></h1><p>This short process will customize this computer for the end user, and deploy it properly.  The process should take less than a half hour, and will require two reboots.</p><p>At times a window will pop up, do not worry, this is just part of the process.  If you have any problems during eXtreme Deployment, please contact the CNS Helpdesk at extension 3205.  The next step will start in 10 seconds.<p></p>If at any point you are asked if you wish to reboot to install new hardware, please click on no.</p></body>;
	xd_ui(welcomeMessage, 'eXtreme Deployment', 5);
	xd_set_state('step', 'displayinfo');
}

function displayinfo02()
{
	var modelNumber = GetWMIProperty("Win32_ComputerSystemProduct", "Name");
	var serialNumber = GetWMIProperty("Win32_ComputerSystemProduct", "IdentifyingNumber");

	getCNMessage = new XMLRPCMessage("get_computer_name");
	getCNMessage.addParameter(serialNumber);
	getCNMessage.addParameter(modelNumber);
	var infoRecord = xd_rpc_call(getCNMessage);

	var infoTable;
	if(infoRecord === false)
	{
		infoTable = <body><h1>Information about this computer from our database:</h1><hr />
			<p>No information for this computer was found in the deployment database.  Please contact the CNS Helpdesk at (973)408-3205 or helpdesk@drew.edu.  Please make sure you have the model and serial number for this machine ready.</p></body>;
	}
	else
	{
		infoTable = <body><h1>Information about your computer from our database:</h1>
			<hr />
			<table width='70%' border='0'>
			<tr><td>Listed owner's name:</td><td>{infoRecord['owner_name']}</td></tr>
			<tr><td>Listed owner's username:</td><td>{infoRecord['owner_username']}</td></tr>
			<tr><td>Designated computer name:</td><td>{infoRecord['name']}</td></tr>
			<tr><td>Computer model number:</td><td>{modelNumber}</td></tr>
			<tr><td>Computer serial number:</td><td>{serialNumber}</td></tr>
			</table>
			<hr />
			<p>Please verify that the above owner information is correct, and then press continue.  If you believe there is a problem, please contact the CNS Helpdesk at extension 3205 or helpdesk@drew.edu.</p>
			<form action='licenseagree.html' method='post'>
				<input type='submit' value='Continue' />
			</form></body>;
		
		xd_set_state('local_model', modelNumber);
		xd_set_state('local_sn', serialNumber);
		xd_set_state('compname', infoRecord['name']);
		xd_set_state('owner_username', infoRecord['owner_username']);
		xd_set_state('owner_name', infoRecord['owner_name']);
	}
	xd_ui(infoTable, 'eXtreme Deployment', 0);
	xd_set_state('step', 'licensepage');
}

function licensepage03()
{
	var licensePage = "<html><head><title>eXtreme Deployment</title><meta http-equiv='Content-Type' content=\"text/html; charset=iso-8859-1\" /><link ref='assets/styles.css' rel='stylesheet' type='text/css' /></head><body>" + 
	"<h1>Drew University Desktop Software Terms of Use</h1>" +
	"<b><p>ONLY SELECT \"I AGREE\" IF YOU AGREE TO ALL DREW UNIVERSITY AND MANUFACTURER LICENSE AGREEMENTS.  SELECTING \"I AGREE\" INDICATES YOUR FULL ACCEPTANCE OF THE ENTIRE CONTENTS OF THIS DOCUMENT.  IF YOU DISAGREE WITH ANY PART OF THIS DOCUMENT, YOU MAY NOT PROCEED WITH THE INSTALLATION.</p>" +
	"<p>THE SOFTWARE IS PROVIDED AS A PACKAGE, YOU MAY NOT ELECT TO ACCEPT PARTS OF THE AGREEMENT SEPARATELY.</p></b>" +
	file_get_contents('license.html') +
	"<p><b>SELECT \"I AGREE\" BELOW ONLY IF YOU AGREE WITH THE ENTIRE CONTENTS OF THIS DOCUMENT.</b></p>" +
	"<form action='agree.php' method=\"post\">" +
	"<input type='submit' name='I AGREE' value='I AGREE' />" +
	"</form>" +
	"</body></html";

	ShowUI(licensePage, null, "file:///b:/xdeploy/htdocs/");
	xd_set_state('step', 'compname');
}

function compname04()
{
	var compName = xd_get_state('compname');
	var message;
	var renameOK = true;
	//var renameOK = SetComputerName(compName, ComputerNamePhysicalNetBIOS);
	if(renameOK)
		message = <body><h1>Computer Renamed</h1><p>The computer will be renamed to {compName} after it reboots.</p></body>;
	else
		message = <body><h1>Computer Renamed</h1><p>There was an error renaming the computer.</p></body>;
		
	xd_ui(message, 'eXtreme Deployment', 5);
	xd_set_state('step', 'joindomain');
}

function joindomain05()
{
	var ownerUsername = xd_get_state('owner_username');
	var credentialsPage = <body><h1>Joining Active Directory</h1><a name='error'></a><p>Please enter your password in below.  If you are setting up this computer for someone else, please enter your username and password.</p><form action='joindomain'><table width='80%' border='0'><tr><td>Username: <input name='username' type='text' value={ownerUsername} size='30' maxlength='15' /></td><td>Password: <input name='password' type='password' size='30' /></td><td><input type='submit' value='Login' /></td></tr></table></form></body>;

	var credStore = xd_ui(credentialsPage, 'eXtreme Deployment', 0);

	var joinOK = false;//= NetJoinDomain(null, 'drew-ad', xd_get_state('ou'), credStore.Get("username"), credStore.Get("password"), NETSETUP_JOIN_DOMAIN);
	if(!joinOK)
	{
		// html.p.(@id == "p1")
		credentialsPage.a = <p><font color='red'>Invalid credentials, please re-enter your username and password.</font></p>;
	}
	while(joinOK == false)
	{
		credStore = xd_ui(credentialsPage, 'eXtreme Deployment', 0);
	//	joinOK = NetJoinDomain(null, 'drew-ad', xd_get_state('ou'), credStore.Get("username"), credStore.Get("password"), NETSETUP_JOIN_DOMAIN);	
		joinOK = true;
	}
	xd_set_state('step', 'addingadmin');
}
function addingadmin06()
{
	var adminUserPage = <body><h1>The designated administrators for this machine have been given full rights.</h1><hr/><p>This list of designated administrators consists of:</p></body>;
	adminMsg = new XMLRPCMessage('get_admins');
	adminMsg.addParameter(serialNumber);
	adminMsg.addParameter(modelNumber);
	adminList = xd_rpc_call(adminMsg);
	if(adminList === false)
		MessageBox("No admins found!");
	adminListElement = <ul></ul>;
	for(i = 0; i < adminList.length; i++)
		adminListElement.appendChild(<li>{adminList[i].toString()}</li>);
	adminUserPage.appendChild(adminListElement);
	NetLocalGroupAddMembers("Administrators", adminList);
	xd_ui(adminUserPage, 'eXtreme Deployment', 5);
	xd_set_state('step', 'notifyserver');
}

function notifyserver07()
{
	var notifyPage = <body><h1>Notified deployment server</h1><hr /><p>The deployment server has been updated.  Only one more step remains.</p></body>;
	var notifyMsg = new XMLRPCMessage('inform_deploy');
	notifyMsg.addParameter(serialNumber);
	notifyMsg.addParameter(modelNumber);
	//notifyOK = xd_rpc_call(notifyMsg);
	xd_ui(notifyPage, 'eXtreme Deployment', 3);
	xd_set_state('step', 'adminpassword');
}

function adminpassword08()
{
	var validChars = "abcdefghjklmnpqrstuvwxyzABCDEFGHJKLMNPQRSTUVWXYZ23456789";
	var password = "";
	length = 12;
	while(length > 0)
	{
		var randomIndex = ((Math.random() * 100)%validChars.length).toFixed();
		password += validChars[randomIndex];
		length--;
	}
	//exec("net user administrator " + password);
	var passwordpage = <body><h1>The password for the Administrator account on this machine</h1><hr /><p>{password}</p><hr /><p>Please do not share this password with anyone else, or lose it.  Keep this password secure and safe, unauthorized use of this password could endanger the security of this computer.  Loss of the password to the Administrator account can prevent repair of certain software problems without loss of all user data.</p><p>The current Administrator password has been stored in a secure database in case of loss.  The password is held in escrow by Computing and Network Services, and will only be available to trusted CNS employees while an incident with this machine is open.</p><p>eXtreme Deployment is now complete.  Please click continue so that you can log in and begin using the computer.</p><form><input type='submit' value='Continue' /></form></body>;
	xd_ui(passwordpage, 'eXtreme Deployment', 0);
	xd_set_state('step', 'finished');
}
InitUI();
var currentStep = xd_get_state('step');
while(currentStep != 'finished')
{
	if(currentStep == 'compname')
		compname04();
	if(currentStep == 'joindomain')
		joindomain05();
	if(currentStep == 'addingadmin')
	{
		addingadmin06();
		notifyserver07();
		adminpassword08();
	}
	else
	{
		welcome01();
		displayinfo02();
		licensepage03();
	}
	currentStep = xd_get_State('step');
}
ShutdownUI();