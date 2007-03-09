#!/usr/bin/perl

use English;
use POSIX;
use Cwd;
use Cwd 'abs_path';
use File::Basename;
#use File::Glob;
use File::Copy;
use File::Spec;

$0 =~ s/.*\///;
$ProgName = $0;

sub usage {
	my($msg) = @_;

	if ($msg) {
		printf STDERR "$ProgName: $msg\n";
	}

    my($format) = "\t%-12.12s  %-12.12s  %s\n";
        

	print STDERR "Usage: $ProgName [options] targetlist... libdir";
	print STDERR "\nWhere \"options\" are:\n\n";
	printf STDERR $format, "Option name", "Default", "Description";
	printf STDERR $format, "------ ----", "-------", "-----------";
	printf STDERR $format, "-exclude", "expression", "Paths to exclude";
	printf STDERR $format, "-include", "expression", "Paths to include (override -exclude)";
	printf STDERR $format, "-ldlibpath", "expression", "library search path";
	print STDERR "\n";

    
    exit(1)


}


sub chaselink {
    my($path) = @_;

    if (defined($link = readlink($path))) {
		if (! File::Spec->file_name_is_absolute($link)) {
			my($name0,$dir0) = fileparse($path);
			my($name1,$dir1) = fileparse($link);

			$link = "$dir0" . "$name1"; 
		}

        return(chaselink($link));
    }
    else {
        return($path);
    }   
}

sub get_deps {
	my($target) = @_;


	my(@Deps) = ();

	#
	# clean up path to target
	#
	my($volume, $directories, $file) = File::Spec->splitpath($target);
	$directories = abs_path($directories);
	$target = File::Spec->catpath($volume, $directories, $file);

	$darwin = 0;
	if (-e "/usr/bin/otool" ) {
		# Mac system
		@lddcmd = ("/usr/bin/otool", "-L"); 
		$darwin = 1;
	} else {
		@lddcmd = ("/usr/bin/ldd");
	}

	$cmd = join(' ', @lddcmd, $target);
	$_ = `$cmd`;
	if ($?>>8) {
		printf STDERR "$ProgName: Command \"$cmd\" failed\n";
		exit(1);
	}

	@lines = split /\n/, $_;
	if ($darwin) {
		shift @lines;	# discard first line
	}


LINE:	foreach $line (@lines) {
		$line =~ s/^\s+//;
		if ($darwin) {
			($lib) = split(/\s+/, $line)
		}
		else {
			($junk1, $junk2, $lib) = split(/\s+/, $line)
		}
		next LINE if (! defined($lib));
		if ($lib =~ "not found") {
			printf STDERR "$ProgName: Command \"$cmd\" failed - library not found\n";
			exit(1);
		}

		#
		# clean up directory path
		#
		my($volume, $directories, $file) = File::Spec->splitpath($lib);
		$directories = abs_path($directories);
		$lib = File::Spec->catpath($volume, $directories, $file);


		($name, $path, $suffix) = fileparse($lib);
		next LINE if ($darwin && !($lib =~ /dylib/));

		if (! -f $lib) {
			printf STDERR "$ProgName: Command \"$cmd\" failed - library not found\n";
			exit(1);
		}


		$toss = 0;
		foreach $exclude (@ExcludePaths) {
			if ($lib =~ m!$exclude!) {
				$toss = 1;
			}
		}
		if ($toss) {
			foreach $include (@IncludePaths) {
				if ($lib =~ m!$include!) {
					$toss = 0;
				}
			}
		}

		if ($lib eq $target) {
			$toss = 1;
		}

		if (! $toss) {
			push @Deps, $lib;
		}
	}
	return(@Deps);
}
			
	

@IncludePaths = ();
@ExcludePaths = ();
while ($ARGV[0] =~ /^-/) {
    $_ = shift @ARGV;

    if (/^-exclude$/) {
        defined($_ = shift @ARGV) || die "Missing argument";
		push(@ExcludePaths, $_);
    }
    elsif (/^-include$/) {
        defined($_ = shift @ARGV) || die "Missing argument";
		push(@IncludePaths, $_);
    }
    elsif (/^-ldlibpath$/) {
        defined($_ = shift @ARGV) || die "Missing argument";
		$LD_LIBRARY_PATH = defined($LD_LIBRARY_PATH) ? "$LD_LIBRARY_PATH:$_" : $_;
    }
    else {
        usage("Invalid option: $_");
    }
}


if (! (defined($Libdir = pop @ARGV))) {
	usage("Wrong # of arguments");
}

if (! -d $Libdir) {
	print STDERR "$ProgName: Library directory $libdir does not exist\n";
	exit(1);
}

@Targets = @ARGV;

$ENV{"LD_LIBRARY_PATH"} = $LD_LIBRARY_PATH;
$ENV{"LD_LIBRARYN32_PATH"} = $LD_LIBRARY_PATH;
$ENV{"LD_LIBRARY64_PATH"} = $LD_LIBRARY_PATH;

@cpfiles = ();
while (defined($target = shift(@Targets))) {


	@_ = get_deps($target);

	foreach $dep (@_) {
		$match = 0;
		foreach $target (@Targets) {
			$match = 1 if ($dep eq $target);
		}
		push @Targets, $dep if (! $match);

		$match = 0;
		foreach $cpfile (@cpfiles) {
			$match = 1 if ($dep eq $cpfile);
		}
		push @cpfiles, $dep if (! $match);
	}
}

foreach $_ (@cpfiles) {
	print "Copying $_ to $Libdir\n";
	copy($_,$Libdir) || die "$ProgName: file copy failed - $!\n";
}


exit 0;
