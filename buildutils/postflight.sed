#!/bin/csh -f
# This postflight script echoes the values of the available 
# arguments and environmental variables.
#

set version_app = "VERSION_APP";

echo "Start postflight script"
echo ""

set InstDest = $argv[2]


#
#
#
#
echo "Linking executables..."

if (! -d /usr/local/bin) then
	mkdir -p /usr/local/bin
	if ($status != 0) exit 1
endif

foreach file ( asciitf2vtf ncdf2vdf raw2vdf vapor-setup.csh vapor-setup.sh vaporgui vaporversion vdf2raw vdfbkup.pl vdfcreate vdfls wrf2vdf wrfvdfcreate vdfedit)

	if (-e  /usr/local/bin/$file || -l /usr/local/bin/$file) then
		/bin/rm /usr/local/bin/$file
	endif

	ln -s $InstDest/VAPOR.app/Contents/MacOS/$file /usr/local/bin/$file
	if ($status != 0) exit 1
end


#
#
#
#
echo "Linking libraries..."

if (! -d /usr/local/lib) then
	mkdir -p /usr/local/lib
	if ($status != 0) exit 1
endif

foreach fullpath ($InstDest/VAPOR.app/Contents/MacOS/*.dylib $InstDest/VAPOR.app/Contents/MacOS/*.so $InstDest/VAPOR.app/Contents/MacOS/*.dlm)

	set file = `basename $fullpath`
	if (-e  /usr/local/lib/$file || -l /usr/local/lib/$file) then
		/bin/rm /usr/local/lib/$file
	endif

	ln -s $InstDest/VAPOR.app/Contents/MacOS/$file /usr/local/lib/$file
	if ($status != 0) exit 1
end

#
#
#
#
echo "Linking examples..."

if (! -d /usr/local/share/$version_app/examples) then
	mkdir -p /usr/local/share/$version_app/examples
	if ($status != 0) exit 1
endif

foreach fullpath ($InstDest/VAPOR.app/Contents/SharedSupport/$version_app/examples/*)

	set file = `basename $fullpath`
	if (-e  /usr/local/share/$version_app/examples/$file || -l /usr/local/share/$version_app/examples/$file) then
		/bin/rm /usr/local/share/$version_app/examples/$file
	endif

	ln -s $InstDest/VAPOR.app/Contents/SharedSupport/$version_app/examples/$file /usr/local/share/$version_app/examples/$file
	if ($status != 0) exit 1
end

#
#
#
#
echo "Linking palettes..."

if (! -d /usr/local/share/$version_app/palettes) then
	mkdir -p /usr/local/share/$version_app/palettes
	if ($status != 0) exit 1
endif

foreach fullpath ($InstDest/VAPOR.app/Contents/SharedSupport/$version_app/palettes/*)

	set file = `basename $fullpath`
	if (-e  /usr/local/share/$version_app/palettes/$file || -l /usr/local/share/$version_app/palettes/$file) then
		/bin/rm /usr/local/share/$version_app/palettes/$file
	endif

	ln -s $InstDest/VAPOR.app/Contents/SharedSupport/$version_app/palettes/$file /usr/local/share/$version_app/palettes/$file
	if ($status != 0) exit 1
end

#
#
#
#
echo "Linking doc..."

if (! -d /usr/local/share/$version_app/doc) then
	mkdir -p /usr/local/share/$version_app/doc
	if ($status != 0) exit 1
endif

foreach fullpath ($InstDest/VAPOR.app/Contents/SharedSupport/$version_app/doc/*)

	set file = `basename $fullpath`
	if (-e  /usr/local/share/$version_app/doc/$file || -l /usr/local/share/$version_app/doc/$file) then
		/bin/rm /usr/local/share/$version_app/doc/$file
	endif

	ln -s $InstDest/VAPOR.app/Contents/SharedSupport/$version_app/doc/$file /usr/local/share/$version_app/doc/$file
	if ($status != 0) exit 1
end

#
#
#
#
echo "Linking man..."

if (! -d /usr/local/share/man/man1) then
	mkdir -p /usr/local/share/man/man1
	if ($status != 0) exit 1
endif

foreach fullpath ($InstDest/VAPOR.app/Contents/SharedSupport/man/man1/*)

	set file = `basename $fullpath`
	if (-e  /usr/local/share/man/man1/$file || -l /usr/local/share/man/man1/$file) then
		/bin/rm /usr/local/share/man/man1/$file
	endif

	ln -s $InstDest/VAPOR.app/Contents/SharedSupport/man/man1/$file /usr/local/share/man/man1/$file
	if ($status != 0) exit 1
end
