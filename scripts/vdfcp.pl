#!/usr/bin/perl
#
#      $Id$
#
#########################################################################
#                                                                        #
#               Copyright (C)  2013                                      #
#         University Corporation for Atmospheric Research                #
#               All Rights Reserved                                        #
#                                                                        #
#########################################################################
#
#    File:        vdfcp.pl
#
#    Author:        Miles Rufat-Latre
#            National Center for Atmospheric Research
#            PO 3000, Boulder, Colorado
#
#    Date:        Fri Mar 13 18:00:32 MST 2013
#
#    Description:    A command-line utility for copying .vdf datasets
#
#    Usage:
#
#    Environment:
#
#    Files:
#
#
#    Options:

use strict;
#some warnings are expected in normal execution
#use warnings;
use File::Copy;
use File::Path;
use Scalar::Util;
no warnings "uninitialized";

##########################################
#BEGIN GLOBAL VARIABLE DECLARATIONS
my $usage = 
"Usage: vdfcp.pl [options] source.vdf <dest>\n" .
"OPTIONS...\n" . 
"-ts0 #\n" .
"    Set '#' as first timestep to copy.\n" .
"-ts1 #\n" .
"    Set '#' as last timestep to copy.\n" .
"-numts #\n" .
"    Copy '#' timesteps.\n" .
"-vars a:b:c\n" .
"    Copy only variables 'a', 'b' and 'c'.\n" .
"-nvars a:b:c\n" .
"    Copy all variables EXCEPT 'a', 'b' and 'c'.\n" .
"-level #\n" .
"    Copy all levels of detail up to and including '#'.\n" .
"-info\n" .
"    Show available levels, frames and variables in a human-readable format.\n    Can also be used to show selected ranges. No target needed.\n" .
"-list\n" .
"    Output a list of all files (each with its full path)\n    that would be copied given the selected ranges.\n" .
"-help\n" .
"    Print this usage statement and exit.\n" .
"-v\n" .
"    verbose: output detailed information when copying.\n" .
"-f\n" .
"    force: overwrite existing datasets, create destination directory.\n";

#user input variables
my $mints = -1; #these four start at -1 so we know if there was user input
my $maxts = -1;
my $numts = -1;
my $level = -1;
my $info = 0;
my $list = 0; #instead of copying, list the files that would be copied
my $force = 0; #overwrite target files if they exist
my $verbose = 0; #print extra information

#variables used to store information about the ranges in a dataset
my $amints;
my $amaxts;
my $alevel;

#variables wanted/not wanted
my @vars;
my @include;
my @exclude;
my %inc; #mapped versions of the arrays above
my %exc;
#a list of those files which are actually available
my @varfiles;

#information about the source and target datasets
my $srcvdf;
my $srcdir;
my $dstvdf;
my $dstdir;

#utility variables
my @argbuf;
my $arg;
my @tok;
#END VARIABLE DECLARATIONS
##########################################

##########################################
#BEGIN EXECUTED CODE
#the following function calls fill out the variables above
parseOptions(); #get any options the user input
parseArgs(); #get the source and destination

print "SOURCE=$srcvdf\nTARGET=$dstvdf\n" if($verbose);

#get all the variables we'll be handling
@vars = grep{-d "$srcdir/$_" && ($_ eq "mask" || 
    (keys(%inc) == 0 || $inc{$_})) && !$exc{$_}} dirContents($srcdir);

#scanvars prints scanned information and exits if in info mode
scanvars();

#handle force cases
if(-d $dstdir && !$list)
{
    unless($force)
    {
        print "cannot copy over existing target directory\n    use '-f' option to force overwrite\n";
        exit(0);
    }
    print "WARNING: overwriting existing target directory!\n";
    rmtree($dstdir);
}
if(-f $dstvdf && $list == 0)
{
    unless($force)
    {
        print "cannot copy over existing target .vdf metafile\n    use '-f' option to force overwrite\n";
        exit(0);
    }
    print "WARNING: overwriting existing target .vdf metafile!\n";
    unlink($dstvdf);
}

#copy the files (or just print them if user said "list")
print "making target directory: $dstdir\n" if($verbose);
mkdir($dstdir);
if($list)
{
    print "$srcvdf\n";
}
else
{
    print "copying metafile \"$srcvdf\"...\n" if($verbose);
    File::Copy::copy($srcvdf, $dstvdf) 
        or die "failed to copy source metafile to target location: $!\n";
}

my $var;
my $buf0;
my $buf1;
foreach(@vars)
{
    print "copying variable \"$_\"...\n" if($verbose && !$list);
    $var = $_;
    @varfiles = grep{-f "$srcdir/$var/$_"} dirContents("$srcdir/$var");
    mkdir("$dstdir/$var");
    foreach(@varfiles)
    {
        @tok = split(/\./, $_);
        #note that we check for '-1' value in some parameters
        if
        (
            #exploded the condition for easier reading
            (
                substr($tok[0], -5) eq "_mask" 
              &&
                ($maxts == -1 || $tok[1] <= $maxts)
              &&
            ($mints == -1 || $tok[1] >= $mints)
            ) 
          || 
            $var eq "mask"
          || 
            (
                ($maxts == -1 || $tok[1] <= $maxts) 
              && 
                ($mints == -1 || $tok[1] >= $mints) 
              &&
                ($level == -1 || substr($tok[2], 2) <= $level)
            )
        )
        {
            copyOrList("$var/$_");
        }
    }
}
#END EXECUTED CODE
##########################################

##########################################
#BEGIN FUNCTION DEFINITIONS
#parses command-line options to set global variables
sub parseOptions
{
    #process any ARGVs
    while(@ARGV > 0)
    {
        #send any non-option arguments to argbuf
        #once we've emptied ARGV, we'll copy them back
        unless(substr($ARGV[0], 0, 1) eq "-")
        {
            push(@argbuf, $ARGV[0]);
            shift(@ARGV);
            next;
        }
        #remove the dash for "-" options
        $arg = substr($ARGV[0], 1);
        #remove that argument for handler
        shift @ARGV;
        #call the handler through an if-elsif-etc
        if($arg eq "ts0")
        {
            die "Invalid ts0.\n"
                if($ARGV[0] < 0 || $ARGV[0] + 0 != $ARGV[0]);
            $mints = shift @ARGV;
        }
        elsif($arg eq "ts1")
        {
            die "Invalid ts1.\n"
                if($ARGV[0] < 0 || $ARGV[0] < $mints 
                || $ARGV[0] + 0 != $ARGV[0]);
            $maxts = shift @ARGV;
        }
        elsif($arg eq "numts")
        {
            die "Invalid numts.\n"
                if($ARGV[0] <= 0 || $ARGV[0] + 0 != $ARGV[0]);
            $numts = shift @ARGV;
        }
        elsif($arg eq "vars")
        {
            @include = split(/:/, shift @ARGV);
        }
        elsif($arg eq "nvars")
        {
            @exclude = split(/:/, shift @ARGV);
        }
        elsif($arg eq "level")
        {
            die "Invalid level.\n"
                if($ARGV[0] < 0 || $ARGV[0] + 0 != $ARGV[0]);
            $level = shift @ARGV;
        }
        elsif($arg eq "info")
        {
            $info = 1;
            $verbose = 0;
        }
        elsif($arg eq "list")
        {
            $list = 1;
            $verbose = 0;
        }
        elsif($arg eq "help")
        {
            print $usage;
            exit(0);
        }
        elsif($arg eq "v")
        {
            $verbose = 1 unless($info || $list);
        }
        elsif($arg eq "f")
        {
            $force = 1;
        }
        else
        {
            die "unrecognized option \"-$arg\", exiting...\n$usage";
        }
    }
    
    #recover the non-optional arguments
    @ARGV = @argbuf;
    
    #map-ify the includes and excludes for faster finding
    %inc = map{$_ => $_} @include;
    %exc = map{$_ => $_} @exclude;

    #set up $mints and $maxts using $numts if $numts was given
    die "ts0 must be less than or equal to ts1!\n"
        if($maxts != -1 && $mints > $maxts && !($list || $info));
}

#parses the non-optional arguments to get the source and target datasets
sub parseArgs
{
    #the remaining argument(s) should just be source and target
    die "Too few arguments\n" . $usage if(@ARGV < 1);
    die "Must provide target unless in info or list mode\n" . $usage
        if(@ARGV < 2 && $info == 0 && $list == 0);

    #get information about source and target from arguments
    #$ARGV[0] is the "source" argument. Must be .vdf
    #$ARGV[1] is the "dest" argument. May be .vdf or EXISTING directory.
    $srcvdf = $ARGV[0];
    die "Source must be an existing *.vdf file.\n" 
        unless(substr($srcvdf, -4, 4) eq ".vdf" && -f $srcvdf);
    $srcdir = $srcvdf; 
    substr($srcdir, -4, 4, "_data");
    die "Source data directory (\"$srcdir\") doesn't exist!\n"
        unless(-d $srcdir);
    
    if(substr($ARGV[1], -4, 4) eq ".vdf")
    {
        $dstvdf = $ARGV[1];
        $dstdir = $dstvdf;
        substr($dstdir, -4, 4, "_data");
    }
    else
    {
        @tok = split(/\//, $srcvdf);
        $dstvdf = $ARGV[1] . "/" . pop(@tok);
        $dstdir = $dstvdf;
        substr($dstdir, -4, 4, "_data");
        die "Target directory (\"$ARGV[1]\") must exist!\n    (Non-vdf names in target are interpreted as directory names)\n" unless(-d $ARGV[1] || $info == 1 || $list == 1);
    }

    #Disable any extra messages if -list is on
    if($info == 1)
    {
        $verbose = 0;
        $list = 0;
    }
    elsif ($list == 1)
    {
        $verbose = 0;
    }
    $dstvdf = "" if($info == 1);
}

#copies, or prints the item to stdout based on whether $list is on
sub copyOrList
{
    if($list == 1)
    {
        print "$srcdir/$_[0]\n";
        return;
    }
    File::Copy::copy("$srcdir/$_[0]", "$dstdir/$_[0]") or die "failed to copy file \"$srcdir/$_[0]\" to \"$dstdir/$_[0]\": $!\n";
}

#scans the first available variable to see what ranges are present
#culls these to the ranges input by the user
#dies with an error if the user asked for a range that is absent
#prints the resulting information and exits if we are in info mode
sub scanvars
{
    #make sure we don't try to scan the mask folder
    die "no variables selected, or no selected variables available!\n"
        if(@vars < 1);
    if($vars[0] eq "mask")
    {
        die "no variables selected, or no selected variables available!\n"
            if(@vars < 2);
    $vars[0] = $vars[1];
    $vars[1] = "mask";
    }
    @varfiles = grep{-f "$srcdir/$vars[0]/$_"} dirContents("$srcdir/$vars[0]");
    $amints = -1;
    $amaxts = $amints;
    my $curr;
    $alevel = -1;
    foreach(@varfiles)
    {
        @tok = split(/\./, $_);
        #ignore mask files for the purpose of LOD, but not for ts#
        if(substr($tok[0], -5) ne "_mask")
        {
            $curr = substr($tok[2], 2);
            $alevel = $curr if($curr > $alevel);
        }
        $curr = $tok[1];
        $amaxts = $curr if($curr > $amaxts || $amaxts == -1);
        $amints = $curr if($curr < $amints || $amints == -1);
    }
    die "no timesteps available in selected dataset!\n"
        if($amints < 0 || $amints < 0);

    #correct numts
    unless($numts == -1)
    {
        $numts--;
        if($mints == -1)
        {
            if($maxts == -1)
            {
                $mints = $amints;
                $maxts = $mints + $numts;
            }
            else
            {
                $mints = $maxts - $numts;
            }
        }
        else
        {
            $maxts = $mints + $numts;
        }
    }

    die "ts0 is less than minimum timestep in dataset!\n" 
        if($mints != -1 && $mints < $amints);
    die "ts1 is greater than maximum timestep in dataset!\n"
        if($maxts != -1 && $maxts > $amaxts);
    die "selected level is higher than available in dataset!\n"
        if($level != -1 && $level > $alevel);

    #cull the ranges to what the user selected
    if($maxts < $amaxts && $maxts != -1)
    {
        $amaxts = $maxts;
        $amints = $amaxts if($amints > $amaxts);
    }
    if($mints > $amints && $mints != -1)
    {
        $amints = $mints;
        $amaxts = $amints if($amaxts < $amints);
    }
    $alevel = $level if($level < $alevel && $level != -1);
    $mints = $amints;
    $maxts = $amaxts;
    $level = $alevel;

    return unless($info);

    #do the printing thing! yes! :D
    print "==========RANGE==========\n",
          "max LOD:        ", $alevel, "\n",
          "first timestep: ", $amints, "\n",
          "last timestep:  ", $amaxts, "\n";
    print "========VARIABLES========\n";
    foreach(@vars)
    {
        print "$_\n";
    }
    exit(0); 
}

#grabs the contents of a directory, removing '.' and '..'
sub dirContents
{
    opendir DIR, $_[0] or die "failed to open directory \"$_[0]\": $!\n";
    my @buf = grep{$_ ne "." && $_ ne ".."} readdir DIR;
    closedir DIR;
    return @buf;
}
#END FUNCTION DEFINITIONS
##########################################

#END OF FILE
##########################################
