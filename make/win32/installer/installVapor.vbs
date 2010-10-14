MsgBox("executing installer script")
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
MsgBox("setting enviroment paths")
vaporhome = Right(inputstring, len(inputstring) - posn -1)

vaporhome = vaporhome & "VAPOR"
vaporshare = vaporhome & "\share"
vaporbin = vaporhome & "\bin"
vaporpython = vaporhome & "\lib\Python2.6.6\lib\site-packages"
pythonhome = vaporhome & "\lib\Python2.6.6"

set shell = CreateObject("wscript.shell")

If allUserProp Then
	set sysEnv = shell.Environment("SYSTEM")
Else
	set sysEnv = shell.Environment("USER")
End If
MsgBox("setting environment variables")
SysEnv("VAPOR_HOME") = vaporhome
SysEnv("VAPOR_SHARE") = vaporshare
SysEnv("PYTHONHOME") = pythonhome

'  Insert VAPOR_HOME\bin at start of path
pathvar = sysEnv("path")
pathvar = vaporbin & ";" & pathvar
SysEnv("path") = pathvar

'  Insert vaporpython at start of pythonpath
MsgBox("setting pythonpath")
pathvar = sysEnv("PYTHONPATH")
pathvar = vaporpython & ";" & pathvar
SysEnv("PYTHONPATH") = pathvar

idlpath = sysenv("IDL_DLM_PATH")

'  unzip the Python directory
ZipFile= vaporhome & "\python266.zip"
'  The folder the contents should be extracted to:
ExtractTo= vaporhome & "\lib"
MsgBox("Preparing to extract zip file")
'  If the extraction location does not exist create it.
Set fso = CreateObject("Scripting.FileSystemObject")
If NOT fso.FolderExists(ExtractTo) Then
  fso.CreateFolder(ExtractTo)
End If

'  Extract the contents of the zip file.
set objShell = CreateObject("Shell.Application")
set FilesInZip=objShell.NameSpace(ZipFile).items
objShell.NameSpace(ExtractTo).CopyHere(FilesInZip)
Set objShell = Nothing

'  Delete the zip file:
MsgBox("test for zipfile exists")
if (fso.FileExists(ZipFile)) then
    MsgBox("Deleting zipfile")
    MsgBox(ZipFile)
    fso.DeleteFile(ZipFile)
end if

Set fso = Nothing

'Insert vapor_home\bin at start of IDL_DLM_PATH
if idlpath <> "" Then 
	idlpath = ";" & idlpath
End if
sysenv("IDL_DLM_PATH") = vaporbin & idlpath 