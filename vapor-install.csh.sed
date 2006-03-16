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
tar cf - bin include lib | (cd $directory; tar xf -)


#
# Edit the user environment setup scripts
#
set old0 = 'set[ 	][ 	]*root[ 	][ 	]*=.*$'
set new0 = "set root = $directory"
set old1 = 'set[ 	][ 	]*expat[ 	][ 	]*=.*$'
set new1 = "set expat = "
set old2 = 'set[ 	][ 	]*netcdf[ 	][ 	]*=.*$'
set new2 = "set netcdf = "
/bin/ex - $directory/bin/vapor-setup.csh <<EOF
1,\$s#$old0#$new0#
1,\$s#$old1#$new1#
1,\$s#$old2#$new2#
w
q
EOF

set old0 = 'root=.*$'
set new0 = "root=$directory"
set old1 = 'expat=.*$'
set new1 = "expat="
set old2 = 'netcdf=.*$'
set new2 = "netcdf="
/bin/ex - $directory/bin/vapor-setup.sh <<EOF
1,\$s#$old0#$new0#
1,\$s#$old1#$new1#
1,\$s#$old2#$new2#
w
q
EOF



exit 0
