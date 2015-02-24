
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
vaporbin = vaporhome & "\bin"
vaporshare = vaporhome & "\share"

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

'Create shortcuts on Desktop and Program Menu
'Note that these shortcuts only work with the 32-bit installer.
'The 64 bit installer puts the shortcuts into itself, not using VBS

DesktopPath = shell.SpecialFolders("Desktop")
Set link = shell.CreateShortcut(DesktopPath & "\vaporgui.lnk")
link.Description = "Vaporgui"
link.IconLocation = vaporhome & "\vapor-win-icon.ico"
link.TargetPath = vaporhome & "\bin\vaporgui.exe"
link.WindowStyle = 1
link.WorkingDirectory = vaporhome
link.Save
if (allUserProp) then
    LinkPath = Shell.SpecialFolders("AllUsersPrograms")
else
    LinkPath = Shell.SpecialFolders("Programs")
end if

Set link = Shell.CreateShortcut(LinkPath & "\vaporgui.lnk")
link.Description = "Vaporgui"
link.IconLocation = vaporhome &"\vapor-win-icon.ico"
link.TargetPath = vaporhome & "\bin\vaporgui.exe"
link.WindowStyle = 1
link.WorkingDirectory = vaporhome
link.Save

Set link = shell.CreateShortcut(DesktopPath & "\vdcwizard.lnk")
link.Description = "vdcwizard"
link.IconLocation = vaporhome & "\VDCWizard.ico"
link.TargetPath = vaporhome & "\bin\vdcwizard.exe"
link.WindowStyle = 1
link.WorkingDirectory = vaporhome
link.Save
if (allUserProp) then
    LinkPath = Shell.SpecialFolders("AllUsersPrograms")
else
    LinkPath = Shell.SpecialFolders("Programs")
end if

Set link = Shell.CreateShortcut(LinkPath & "\vdcwizard.lnk")
link.Description = "vdcwizard"
link.IconLocation = vaporhome &"\VDCWizard.ico"
link.TargetPath = vaporhome & "\bin\vdcwizard.exe"
link.WindowStyle = 1
link.WorkingDirectory = vaporhome
link.Save