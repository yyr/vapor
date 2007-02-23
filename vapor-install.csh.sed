#!/bin/csh -f

set arch = Linux


if ($#argv != 1) then
	echo "Usage: $0 directory"
	exit (1)
endif

set directory = $argv[1]



echo directory = $directory
#
# If the directory name is not an absolute path then make it one.
#
if (`echo $directory | grep "^/"` == "") then
	set directory = `(cd $directory; pwd)`
endif

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


#
# Edit the user environment setup scripts
#
set old0 = 'set[ 	][ 	]*root[ 	][ 	]*=.*$'
set new0 = "set root = $directory"
set old1 = 'set[ 	][ 	]*expat[ 	][ 	]*=.*$'
set new1 = "set expat = "
set old2 = 'set[ 	][ 	]*netcdf[ 	][ 	]*=.*$'
set new2 = "set netcdf = "
set old3 = 'set[ 	][ 	]*qt[ 	][ 	]*=.*$'
set new3 = "set qt = "
$sedcmd -e "s#$old0#$new0#" -e "s#$old1#$new1#" -e "s#$old2#$new2#" -e "s#$old3#$new3#" < $directory/bin/vapor-setup.csh >! $directory/bin/vapor-setup.tmp
/bin/mv $directory/bin/vapor-setup.tmp $directory/bin/vapor-setup.csh


set old0 = 'root=.*$'
set new0 = "root=$directory"
set old1 = 'expat=.*$'
set new1 = "expat="
set old2 = 'netcdf=.*$'
set new2 = "netcdf="
set old3 = 'qt=.*$'
set new3 = "qt="
$sedcmd -e "s#$old0#$new0#" -e "s#$old1#$new1#" -e "s#$old2#$new2#" -e "s#$old3#$new3#" < $directory/bin/vapor-setup.sh >! $directory/bin/vapor-setup.tmp
/bin/mv $directory/bin/vapor-setup.tmp $directory/bin/vapor-setup.sh



exit 0
