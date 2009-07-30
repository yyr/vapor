#!/bin/csh -f

set arch = "ARCH"
set version_app = "VERSION_APP"

unset directory
unset vapor_root
set nocopy = 0
while ($#argv && ! $?directory)
	if ("$argv[1]" == "-nocopy") then
		set nocopy = 1
	else if ($#argv && "$argv[1]" == "-root") then
		shift
		set vapor_root = $argv[1]
	else
		set directory = $argv[1]
		endif
	shift
end

if ($#argv || ! $?directory) then
	echo "Usage: $0 [-nocopy] [-root <root>] directory"
	exit (1)
endif

if ("$directory" !~ /*) then
	echo "Installatin directory ($directory) must specify an absolute path"
	exit (1)
endif


if (! $?vapor_root) set vapor_root = $directory

echo directory = $directory


if (! $nocopy) then
	#
	# If the installation directory doesn't exist, create it.
	#
	if (! -e $directory) mkdir $directory

	if (! -d $directory) then
		echo "$0 : $directory is not a directory"
		exit 1
	endif

	echo "Installing VAPOR to $directory"

	#
	# Copy the distribution to the target directory
	#
	tar cf - bin include lib share | (cd $directory; tar xf -)
endif

if (-e /bin/sed) set sedcmd = /bin/sed
if (-e /usr/bin/sed) set sedcmd = /usr/bin/sed
if ($?SEDCMD) then
	set sedcmd = $SEDCMD
endif
if (! $?sedcmd) then 
	echo "********WARNING***********"
	echo "The sed command was not found on this system and the installation"
	echo "was not completed. To complete the installation, perform either "
	echo "one of the following:"
	echo ""
	echo "1. Set the SEDCMD environment variable to point to the sed "
	echo "stream editor path and rerun this script."
	echo ""
	echo "or"
	echo "" 
	echo "2. Edit the scripts "
	echo "" 
	echo "$directory/bin/vapor-setup.csh"
	echo "$directory/bin/vapor-setup.sh"
	echo ""
	echo "changing the values of the root, expat, netcdf and possibly the qt "
	echo "variables to the values below:"
	echo ""
	echo "root : $directory"
	if ($?expat) then
		echo "expat : $expat"
	endif
	if ($?netcdf) then
		echo "netcdf : $netcdf"
	endif
	if ($?qt) then
		echo "qt : $qt"
	endif
	echo ""
	echo "********WARNING***********"
	exit 1
endif


if (-e $directory/bin/vapor-setup.csh) then
	set dir = $directory/bin
else
	set dir = $directory
endif

#
# Edit the user environment setup scripts
#
set old0 = 'set[ 	][ 	]*root[ 	][ 	]*=.*$'
set new0 = "set root = $vapor_root"
set old1 = 'set[ 	][ 	]*expat[ 	][ 	]*=.*$'
set new1 = "set expat = "
set old2 = 'set[ 	][ 	]*netcdf[ 	][ 	]*=.*$'
set new2 = "set netcdf = "
set old3 = 'set[ 	][ 	]*qt[ 	][ 	]*=.*$'
set new3 = "set qt = "
set old4 = 'set[ 	][ 	]*share[ 	][ 	]*=.*$'
set new4 = "set share = $vapor_root/share/$version_app"
$sedcmd -e "s#$old0#$new0#" -e "s#$old1#$new1#" -e "s#$old2#$new2#" -e "s#$old3#$new3#" -e "s#$old4#$new4#" < $dir/vapor-setup.csh >! $dir/vapor-setup.tmp
/bin/mv $dir/vapor-setup.tmp $dir/vapor-setup.csh


set old0 = 'root=.*$'
set new0 = "root=$vapor_root"
set old1 = 'expat=.*$'
set new1 = "expat="
set old2 = 'netcdf=.*$'
set new2 = "netcdf="
set old3 = 'qt=.*$'
set new3 = "qt="
set old4 = 'share=.*$'
set new4 = "share=$vapor_root/share/$version_app"
$sedcmd -e "s#$old0#$new0#" -e "s#$old1#$new1#" -e "s#$old2#$new2#" -e "s#$old3#$new3#" -e "s#$old4#$new4#" < $dir/vapor-setup.sh >! $dir/vapor-setup.tmp
/bin/mv $dir/vapor-setup.tmp $dir/vapor-setup.sh



exit 0
