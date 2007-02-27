#!/usr/bin/perl
#
#      $Id$
#
#########################################################################
#									#
#			   Copyright (C)  2006				7
#	     University Corporation for Atmospheric Research		#
#			   All Rights Reserved				#
#									#
#########################################################################
#
#	File:		macbundle.pl
#
#	Author:		John Clyne
#			National Center for Atmospheric Research
#			PO 3000, Boulder, Colorado
#
#	Date:		Thu Feb  1 15:18:09 MST 2007
#
#	Description:	Bundle up vapor for mac platform
#
#	Usage:
#
#	Environment:
#
#	Files:
#
#
#	Options:

use English;
use POSIX;
use File::Spec;
use File::Glob ':globally';	# what the hell is this?

$0 =~ s/.*\///;
$ProgName = $0;


sub usage {
	my($msg) = @_;

	my($format) = "\t%-12.12s  %-16.16s  %s\n";

	if ($msg) {
		printf STDERR "$ProgName: $msg\n";
	}

	printf STDERR "Usage: %s install_path template_path dst_dir version\n", $ProgName;

	exit(1);
}

sub mysystem {
	my(@cmd) = @_;

	print "cmd=@cmd\n";

	system(@cmd);
	if ($? != 0) {
		print STDERR "$ProgName: \"@cmd\" exited with error\n";
		exit(1);
	}
}



sub copy_dir_contents{
	my($srcdir, $destdir) = @_;

	$tmpfile =  "/tmp/ProgName.$$.tar";

	$cwd = getcwd();
	chdir $srcdir or die "$ProgName: Can't cd to $srcdir: $!\n";
	@cmd = ("/sw/bin/tar", "-cf", $tmpfile, ".");
	mysystem(@cmd);

	chdir $cwd or die "$ProgName: Can't cd to $cwd: $!\n";
	chdir $destdir or die "$ProgName: Can't cd to $destdir: $!\n";

	@cmd = ("/usr/bin/tar", "-xf", $tmpfile);
	mysystem(@cmd);

	chdir $cwd or die "$ProgName: Can't cd to $cwd: $!\n";

	unlink $tmpfile;
}

sub copy_template {
	($template_path, $bundle_path) = @_;

	if (-e $bundle_path) {
		@cmd = ("/bin/rm", "-fr", $bundle_path);
		mysystem(@cmd);
	}

	mkdir $bundle_path or die "$ProgName: Can't mkdir $bundle_path: $!\n";
	copy_dir_contents($template_path, $bundle_path);
}

sub copy_install_targets {
	my($install_path, $bundle_path) = @_;

	$srcdir = File::Spec->catdir($install_path, "bin");
	$dstdir = File::Spec->catdir($bundle_path, "Contents", "MacOS");
	copy_dir_contents($srcdir, $dstdir);

	$srcdir = File::Spec->catdir($install_path, "lib");
	$dstdir = File::Spec->catdir($bundle_path, "Contents", "MacOS");
	copy_dir_contents($srcdir, $dstdir);

	$srcdir = File::Spec->catdir($install_path, "include");
	$dstdir = File::Spec->catdir($bundle_path, "Contents", "FrameWorks", "Headers");
	copy_dir_contents($srcdir, $dstdir);

	$srcdir = File::Spec->catdir($install_path, "share");
	$dstdir = File::Spec->catdir($bundle_path, "Contents", "SharedSupport");
	copy_dir_contents($srcdir, $dstdir);

}

sub edit_template {
	my($templatepath, $bundlepath, $version) = @_;


	$file = File::Spec->catfile($templatepath, "Contents", "Info.plist");
	if (! open(IFH, "<$file")) {
		printf STDERR "$ProgName: Can't open \"$file\": $!\n";
		exit(1);
	}

	$file = File::Spec->catfile($bundlepath, "Contents", "Info.plist");
	if (! open(OFH, ">$file")) {
		printf STDERR "$ProgName: Can't open \"$file\": $!\n";
		exit(1);
	}

	while (<IFH>) {
		s/VERSION/$version/;
		printf OFH "%s", $_;
	}

	close (OFH);
	close (IFH);
}

sub install_name_change {
	($target, @libnames) = @_;


	$cmd = "/usr/bin/otool -L $target";
	@lines = `$cmd`;
	if ($?>>8) {
		printf STDERR "$ProgName: Command \"$cmd\" failed\n";
		exit(1);
	}

	foreach $line (@lines) {
		# remove leading white space
		$line  =~ s/^\s+//;

		($libpath) = split(/\s+/, $line);

		foreach $libname (@libnames) {
			if ($libpath =~ $libname && ! ($target =~ $libname)) {
				@cmd = ("/usr/bin/install_name_tool", "-change", $libpath, "\@executable_path/$libname", $target);
				mysystem(@cmd);
			}
		}
	}
}

sub install_name {
	my($bundle_path) = @_;

	$execdir = File::Spec->catdir($bundle_path, "Contents", "MacOS");

	@libpaths = <$execdir/*.dylib>;
	foreach $libpath (@libpaths) {
		@dirs = File::Spec->splitdir($libpath);
		$libname = pop(@dirs);

		@cmd = ("/usr/bin/install_name_tool", "-id", "\@executable_path/$libname", $libpath);
		mysystem(@cmd);
		push @libnames, $libname;
	}

	@execpaths = <$execdir/*>;
	foreach $execpath (@execpaths) {

		$cmd = "/usr/bin/file $execpath";
		$line = `$cmd`;
		if ($?>>8) {
			printf STDERR "$ProgName: Command \"$cmd\" failed\n";
			exit(1);
		}

		if ($line =~ "Mach-O") {
			install_name_change($execpath, @libnames);
		}
	}
}
		

if ($#ARGV != 3) {
	usage("Wrong # of arguments");
}

$InstallPath = File::Spec->canonpath(shift(@ARGV));
$TemplatePath = File::Spec->canonpath(shift(@ARGV));
$DstDir = File::Spec->canonpath(shift(@ARGV));
$VersionString = shift(@ARGV);

@dirs = File::Spec->splitdir($TemplatePath);
$BundleName = pop(@dirs);
$SrcDir = File::Spec->catdir(@dirs);
$BundlePath = File::Spec->catdir($DstDir, $BundleName);


if (defined(shift @ARGV)) {
	usage("Wrong # of arguments");
}

copy_template($TemplatePath, $BundlePath);

edit_template($TemplatePath, $BundlePath, $VersionString);

copy_install_targets($InstallPath, $BundlePath);

install_name($BundlePath);

$vapor_install = File::Spec->catfile($InstallPath, "vapor-install.csh");
$macos_dir = File::Spec->catdir($bundle_path, "Contents", "MacOS");
my (@cmd) = ($vapor_install, "-nocopy", "-root", "/usr/local", $macos_dir);
mysystem(@cmd);

exit(0);
