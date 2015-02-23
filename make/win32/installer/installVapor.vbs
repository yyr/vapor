
inputstring = Session.Property("CustomActionData")

' the inputstring either starts with <>, or it has a "1<>" at the start.
' If it begins with a "1", then it's an "ALLUSER" install

posn = inStr(inputstring, "<>")

leftSide = Left(inputstring, posn-1)

if leftSide = "1" Then
	allUserProp = true
Else
	allUserProp = false
End if

vaporhome = Right(inputstring, len(inputstring) - posn -1)

vaporhome = vaporhome & "VAPOR"
vaporshare = vaporhome & "\share"

pythonhome = vaporhome & "\lib\python2.7"
gribdir = vaporshare & "\grib_api\definitions"

set shell = CreateObject("wscript.shell")

If allUserProp Then
	set sysEnv = shell.Environment("SYSTEM")
Else
	set sysEnv = shell.Environment("USER")
End If

SysEnv("VAPOR_HOME") = vaporhome
SysEnv("VAPOR_SHARE") = vaporshare

idlpath = sysenv("IDL_DLM_PATH")

'Insert vapor_home\bin at start of IDL_DLM_PATH
if idlpath <> "" Then 
	idlpath = ";" & idlpath
End if
sysenv("IDL_DLM_PATH") = vaporbin & idlpath 

