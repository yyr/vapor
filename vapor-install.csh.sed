#!/bin/csh -f

set arch = ARCH


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
tar cf - bin include lib examples | (cd $directory; tar xf -)


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
/bin/sed -e "s#$old0#$new0#" -e "s#$old1#$new1#" -e "s#$old2#$new2#" -e "s#$old3#$new3#" < $directory/bin/vapor-setup.csh >! $directory/bin/vapor-setup.tmp
/bin/mv $directory/bin/vapor-setup.tmp $directory/bin/vapor-setup.csh


set old0 = 'root=.*$'
set new0 = "root=$directory"
set old1 = 'expat=.*$'
set new1 = "expat="
set old2 = 'netcdf=.*$'
set new2 = "netcdf="
set old3 = 'qt=.*$'
set new3 = "qt="
/bin/sed -e "s#$old0#$new0#" -e "s#$old1#$new1#" -e "s#$old2#$new2#" -e "s#$old3#$new3#" < $directory/bin/vapor-setup.sh >! $directory/bin/vapor-setup.tmp
/bin/mv $directory/bin/vapor-setup.tmp $directory/bin/vapor-setup.sh



exit 0
