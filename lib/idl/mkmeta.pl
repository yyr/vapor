#!/usr/bin/perl


@Table = (
	{
		idlname	=> "VDF_MSETGRIDTYPE",
		name => "SetGridType",
		set => 1,
		type	=> "string",
		vector	=> 0,
		ts		=> 0,
		var		=> 0,
		tag		=> 0
	},
	{
		idlname	=> "VDF_MGETGRIDTYPE",
		name => "GetGridType",
		set => 0,
		type	=> "string",
		vector	=> 0,
		ts		=> 0,
		var		=> 0,
		tag		=> 0
	},
	{
		idlname	=> "VDF_MSETCOORDTYPE",
		name => "SetCoordSystemType",
		set => 1,
		type	=> "string",
		vector	=> 0,
		ts		=> 0,
		var		=> 0,
		tag		=> 0
	},
	{
		idlname	=> "VDF_MGETCOORDTYPE",
		name => "GetCoordSystemType",
		set => 0,
		type	=> "string",
		vector	=> 0,
		ts		=> 0,
		var		=> 0,
		tag		=> 0
	},
	{
		idlname	=> "VDF_MSETEXTENTS",
		name => "SetExtents",
		set => 1,
		type	=> "double",
		vector	=> 1,
		ts		=> 0,
		var		=> 0,
		tag		=> 0
	},
	{
		idlname	=> "VDF_MGETEXTENTS",
		name => "GetExtents",
		set => 0,
		type	=> "double",
		vector	=> 1,
		ts		=> 0,
		var		=> 0,
		tag		=> 0
	},
	{
		idlname	=> "VDF_MSETNUMTIMESTEPS",
		name => "SetNumTimeSteps",
		set => 1,
		type	=> "long",
		vector	=> 0,
		ts		=> 0,
		var		=> 0,
		tag		=> 0
	},
	{
		idlname	=> "VDF_MGETNUMTIMESTEPS",
		name => "GetNumTimeSteps",
		set => 0,
		type	=> "long",
		vector	=> 0,
		ts		=> 0,
		var		=> 0,
		tag		=> 0
	},
	{
		idlname	=> "VDF_MSETVARNAMES",
		name => "SetVariableNames",
		set => 1,
		type	=> "string",
		vector	=> 1,
		ts		=> 0,
		var		=> 0,
		tag		=> 0
	},
	{
		idlname	=> "VDF_MGETVARNAMES",
		name => "GetVariableNames",
		set => 0,
		type	=> "string",
		vector	=> 1,
		ts		=> 0,
		var		=> 0,
		tag		=> 0
	},
	{
		idlname	=> "VDF_MSETCOMMENT",
		name => "SetComment",
		set => 1,
		type	=> "string",
		vector	=> 0,
		ts		=> 0,
		var		=> 0,
		tag		=> 0
	},
	{
		idlname	=> "VDF_MGETCOMMENT",
		name => "GetComment",
		set => 0,
		type	=> "string",
		vector	=> 0,
		ts		=> 0,
		var		=> 0,
		tag		=> 0
	},
	{
		idlname	=> "VDF_MSETTUSERTIME",
		name => "SetTSUserTime",
		set => 1,
		type	=> "double",
		vector	=> 1,
		ts		=> 1,
		var		=> 0,
		tag		=> 0
	},
	{
		idlname	=> "VDF_MGETTUSERTIME",
		name => "GetTSUserTime",
		set => 0,
		type	=> "double",
		vector	=> 1,
		ts		=> 1,
		var		=> 0,
		tag		=> 0
	},
	{
		idlname	=> "VDF_MSETTXCOORDS",
		name => "SetTSXCoords",
		set => 1,
		type	=> "double",
		vector	=> 1,
		ts		=> 1,
		var		=> 0,
		tag		=> 0
	},
	{
		idlname	=> "VDF_MGETTXCOORDS",
		name => "GetTSXCoords",
		set => 0,
		type	=> "double",
		vector	=> 1,
		ts		=> 1,
		var		=> 0,
		tag		=> 0
	},
	{
		idlname	=> "VDF_MSETTYCOORDS",
		name => "SetTSYCoords",
		set => 1,
		type	=> "double",
		vector	=> 1,
		ts		=> 1,
		var		=> 0,
		tag		=> 0
	},
	{
		idlname	=> "VDF_MGETTYCOORDS",
		name => "GetTSYCoords",
		set => 0,
		type	=> "double",
		vector	=> 1,
		ts		=> 1,
		var		=> 0,
		tag		=> 0
	},
	{
		idlname	=> "VDF_MSETTZCOORDS",
		name => "SetTSZCoords",
		set => 1,
		type	=> "double",
		vector	=> 1,
		ts		=> 1,
		var		=> 0,
		tag		=> 0
	},
	{
		idlname	=> "VDF_MGETTZCOORDS",
		name => "GetTSZCoords",
		set => 0,
		type	=> "double",
		vector	=> 1,
		ts		=> 1,
		var		=> 0,
		tag		=> 0
	},
	{
		idlname	=> "VDF_MSETTCOMMENT",
		name => "SetTSComment",
		set => 1,
		type	=> "string",
		vector	=> 0,
		ts		=> 1,
		var		=> 0,
		tag		=> 0
	},
	{
		idlname	=> "VDF_MGETTCOMMENT",
		name => "GetTSComment",
		set => 0,
		type	=> "string",
		vector	=> 0,
		ts		=> 1,
		var		=> 0,
		tag		=> 0
	},
	{
		idlname	=> "VDF_MSETVCOMMENT",
		name => "SetVComment",
		set => 1,
		type	=> "string",
		vector	=> 0,
		ts		=> 1,
		var		=> 1,
		tag		=> 0
	},
	{
		idlname	=> "VDF_MGETVCOMMENT",
		name => "GetVComment",
		set => 0,
		type	=> "string",
		vector	=> 0,
		ts		=> 1,
		var		=> 1,
		tag		=> 0
	},
	{
		idlname	=> "VDF_MSETVDRANGE",
		name => "SetVDataRange",
		set => 1,
		type	=> "double",
		vector	=> 1,
		ts		=> 1,
		var		=> 1,
		tag		=> 0
	},
	{
		idlname	=> "VDF_MGETVDRANGE",
		name => "GetVDataRange",
		set => 0,
		type	=> "double",
		vector	=> 1,
		ts		=> 1,
		var		=> 1,
		tag		=> 0
	},
	#
	# Top Level (Global) User-Defined Metdata Attributes
	#
	{
		idlname	=> "VDF_MSETLONG",
		name => "SetUserDataLong",
		set => 1,
		type	=> "long",
		vector	=> 1,
		ts		=> 0,
		var		=> 0,
		tag		=> 1
	},
	{
		idlname	=> "VDF_MGETLONG",
		name	=> "GetUserDataLong",
		set		=> 0,
		type	=> "long",
		vector	=> 1,
		ts		=> 0,
		var		=> 0,
		tag		=> 1
	},
	{
		idlname	=> "VDF_MGETLONGTAGS",
		name	=> "GetUserDataLongTags",
		set		=> 0,
		type	=> "string",
		vector	=> 1,
		ts		=> 0,
		var		=> 0,
		tag		=> 0
	},
	{
		idlname	=> "VDF_MSETDBL",
		name => "SetUserDataDouble",
		set => 1,
		type	=> "double",
		vector	=> 1,
		ts		=> 0,
		var		=> 0,
		tag		=> 1
	},
	{
		idlname	=> "VDF_MGETDBL",
		name	=> "GetUserDataDouble",
		set		=> 0,
		type	=> "double",
		vector	=> 1,
		ts		=> 0,
		var		=> 0,
		tag		=> 1
	},
	{
		idlname	=> "VDF_MGETDBLTAGS",
		name	=> "GetUserDataDoubleTags",
		set		=> 0,
		type	=> "string",
		vector	=> 1,
		ts		=> 0,
		var		=> 0,
		tag		=> 0
	},
	{
		idlname	=> "VDF_MSETSTR",
		name => "SetUserDataString",
		set => 1,
		type	=> "string",
		vector	=> 0,
		ts		=> 0,
		var		=> 0,
		tag		=> 1
	},
	{
		idlname	=> "VDF_MGETSTR",
		name	=> "GetUserDataString",
		set		=> 0,
		type	=> "string",
		vector	=> 0,
		ts		=> 0,
		var		=> 0,
		tag		=> 1
	},
	{
		idlname	=> "VDF_MGETSTRTAGS",
		name	=> "GetUserDataStringTags",
		set		=> 0,
		type	=> "string",
		vector	=> 1,
		ts		=> 0,
		var		=> 0,
		tag		=> 0
	},
	#
	# Time Step User-Defined Metdata Attributes
	#
	{
		idlname	=> "VDF_MSETTLONG",
		name => "SetTSUserDataLong",
		set => 1,
		type	=> "long",
		vector	=> 1,
		ts		=> 1,
		var		=> 0,
		tag		=> 1
	},
	{
		idlname	=> "VDF_MGETTLONG",
		name	=> "GetTSUserDataLong",
		set		=> 0,
		type	=> "long",
		vector	=> 1,
		ts		=> 1,
		var		=> 0,
		tag		=> 1
	},
	{
		idlname	=> "VDF_MGETTLONGTAGS",
		name	=> "GetTSUserDataLongTags",
		set		=> 0,
		type	=> "string",
		vector	=> 1,
		ts		=> 0,
		var		=> 0,
		tag		=> 0
	},
	{
		idlname	=> "VDF_MSETTDBL",
		name => "SetTSUserDataDouble",
		set => 1,
		type	=> "double",
		vector	=> 1,
		ts		=> 1,
		var		=> 0,
		tag		=> 1
	},
	{
		idlname	=> "VDF_MGETTDBL",
		name	=> "GetTSUserDataDouble",
		set		=> 0,
		type	=> "double",
		vector	=> 1,
		ts		=> 1,
		var		=> 0,
		tag		=> 1
	},
	{
		idlname	=> "VDF_MGETTDBLTAGS",
		name	=> "GetTSUserDataDoubleTags",
		set		=> 0,
		type	=> "string",
		vector	=> 1,
		ts		=> 0,
		var		=> 0,
		tag		=> 0
	},
	{
		idlname	=> "VDF_MSETTSTR",
		name => "SetTSUserDataString",
		set => 1,
		type	=> "string",
		vector	=> 0,
		ts		=> 1,
		var		=> 0,
		tag		=> 1
	},
	{
		idlname	=> "VDF_MGETTSTR",
		name	=> "GetTSUserDataString",
		set		=> 0,
		type	=> "string",
		vector	=> 0,
		ts		=> 1,
		var		=> 0,
		tag		=> 1
	},
	{
		idlname	=> "VDF_MGETTSTRTAGS",
		name	=> "GetTSUserDataStringTags",
		set		=> 0,
		type	=> "string",
		vector	=> 1,
		ts		=> 0,
		var		=> 0,
		tag		=> 0
	},
	#
	# Variable User-Defined Metdata Attributes
	#
	{
		idlname	=> "VDF_MSETVLONG",
		name => "SetVUserDataLong",
		set => 1,
		type	=> "long",
		vector	=> 1,
		ts		=> 1,
		var		=> 1,
		tag		=> 1
	},
	{
		idlname	=> "VDF_MGETVLONG",
		name	=> "GetVUserDataLong",
		set		=> 0,
		type	=> "long",
		vector	=> 1,
		ts		=> 1,
		var		=> 1,
		tag		=> 1
	},
	{
		idlname	=> "VDF_MGETVLONGTAGS",
		name	=> "GetVUserDataLongTags",
		set		=> 0,
		type	=> "string",
		vector	=> 1,
		ts		=> 0,
		var		=> 0,
		tag		=> 0
	},
	{
		idlname	=> "VDF_MSETVDBL",
		name => "SetVUserDataDouble",
		set => 1,
		type	=> "double",
		vector	=> 1,
		ts		=> 1,
		var		=> 1,
		tag		=> 1
	},
	{
		idlname	=> "VDF_MGETVDBL",
		name	=> "GetVUserDataDouble",
		set		=> 0,
		type	=> "double",
		vector	=> 1,
		ts		=> 1,
		var		=> 1,
		tag		=> 1
	},
	{
		idlname	=> "VDF_MGETVDBLTAGS",
		name	=> "GetVUserDataDoubleTags",
		set		=> 0,
		type	=> "string",
		vector	=> 1,
		ts		=> 0,
		var		=> 0,
		tag		=> 0
	},
	{
		idlname	=> "VDF_MSETVSTR",
		name => "SetVUserDataString",
		set => 1,
		type	=> "string",
		vector	=> 0,
		ts		=> 1,
		var		=> 1,
		tag		=> 1
	},
	{
		idlname	=> "VDF_MGETVSTR",
		name	=> "GetVUserDataString",
		set		=> 0,
		type	=> "string",
		vector	=> 0,
		ts		=> 1,
		var		=> 1,
		tag		=> 1
	},
	{
		idlname	=> "VDF_MGETVSTRTAGS",
		name	=> "GetVUserDataStringTags",
		set		=> 0,
		type	=> "string",
		vector	=> 1,
		ts		=> 0,
		var		=> 0,
		tag		=> 0
	}
);

sub SetFunc {
	my ($name, $type, $vector, $ts, $var, $tag) = @_;

	$tsCheck = "";
	$tsAssign = "";

	$varCheck = "";
	$varAssign = "";
	$varVecAssign = "";

	$tagCheck = "";
	$tagAssign = "";
	$tagVecAssign = "";

	$argc = 1;

	$params = "";
	if ($ts) {
		$tsCheck = "IDL_ENSURE_SCALAR(argv[$argc]);";
		$tsAssign = "size_t ts = (size_t) IDL_LongScalar(argv[$argc]);";
		$params = $params ? "$params, ts" : "ts";
		$argc++;
	
		if ($var) {
			$varCheck = "IDL_ENSURE_SCALAR(argv[$argc]);";
			$varAssign = "char *var = IDL_VarGetString(argv[$argc]);";
			$varVecAssign = "string varvec = var;";
			$params = $params ? "$params, var" : "var";
			$argc++;

		}

	}
	if ($tag) {
		$tagCheck = "IDL_ENSURE_SCALAR(argv[$argc]);";
		$tagAssign = "char *tag = IDL_VarGetString(argv[$argc]);";
		$tagVecAssign = "string tagvec = tag;";
		$params = $params ? "$params, tag" : "tag";
			$argc++;
		$argc++;

	}

	$valVecAssign = "valuevec[i] = valptr[i];";
	if ($type =~ /string/) {
		$idlTypeSpec = "IDL_TYP_STRING";
		$idlType	= "IDL_STRING";
		$valueAssign = "char    *value = IDL_VarGetString(argv[$argc]);";
		$valVecAssign = "valuevec[i] = IDL_STRING_STR(&valptr[i]);";
		$cType = "string";
	}
	elsif ($type =~ /long/) {
		$idlTypeSpec = "IDL_TYP_LONG";
		$idlType	= "IDL_LONG";
		$valueAssign = "IDL_LONG value = IDL_LongScalar(argv[$argc]);";
		$cType = "long";
	}
	elsif ($type =~ /double/) {
		$idlTypeSpec = "IDL_TYP_DOUBLE";
		$idlType	= "double";
		$valueAssign = "double value = IDL_DoubleScalar(argv[$argc]);";
		$cType = "double";
	}
	else {
		die "Invalid type : $type";
	}


	if ($vector) {
		$valueCheck = "IDL_ENSURE_ARRAY(argv[$argc]);";
		$valueAssign = "IDL_VPTR valvar = IDL_BasicTypeConversion(1, &argv[$argc],$idlTypeSpec); $idlType *valptr = ($idlType *) valvar->value.arr->data;";
		$nElts = "valvar->value.arr->n_elts;";
		$params = $params ? "$params, valuevec" : "valuevec";
        $freeTemp = "if (valvar != argv[$argc]) IDL_Deltmp(valvar);";
	}
	else {
		$valueCheck = "IDL_ENSURE_SCALAR(argv[$argc]);";
		$nElts = "1;";
		$valVecAssign = "valuevec[i] = ($cType) value;";
		$params = $params ? "$params, valuevec[0]" : "valuevec[0]";
		$freeTemp = "";
	}
		

print <<"EOF";

void vdf$name(int argc, IDL_VPTR *argv)
{
    Metadata    *metadata = varGetMetadata(argv[0]);
    $tsCheck
    $varCheck
    $tagCheck
    $valueCheck

    $tsAssign
    $varAssign
    $tagAssign
    $valueAssign

    $varVecAssign
    $tagVecAssign
	int n = $nElts
    vector <$cType> valuevec(n);

	for(int i = 0; i<n; i++) {
		$valVecAssign
	}

    metadata->$name(
		$params
	);

	myBaseErrChk();

	$freeTemp
}


EOF
}




sub GetFunc {
	my ($name, $type, $vector, $ts, $var, $tag) = @_;

	$tsCheck = "";
	$tsAssign = "";

	$varCheck = "";
	$varAssign = "";
	$varVecAssign = "";
	$varParm = "";

	$tagCheck = "";
	$tagAssign = "";
	$tagVecAssign = "";
	$tagParm = "";

	$argc = 1;

	$params = "";
	if ($ts) {
		$tsCheck = "IDL_ENSURE_SCALAR(argv[$argc]);";
		$tsAssign = "size_t ts = (size_t) IDL_LongScalar(argv[$argc]);";
		$params = $params ? "$params, ts" : "ts";
		$argc++;
	
		if ($var) {
			$varCheck = "IDL_ENSURE_SCALAR(argv[$argc]);";
			$varAssign = "char *var = IDL_VarGetString(argv[$argc]);";
			$varVecAssign = "string varvec = var;";
			$params = $params ? "$params, var" : "var";
			$argc++;

		}

	}
	if ($tag) {
		$tagCheck = "IDL_ENSURE_SCALAR(argv[$argc]);";
		$tagAssign = "char *tag = IDL_VarGetString(argv[$argc]);";
		$tagVecAssign = "string tagvec = tag;";
		$params = $params ? "$params, tag" : "tag";
		$argc++;

	}

	if ($type =~ /string/) {
		$idlTypeSpec = "IDL_TYP_STRING";
		$idlType = "IDL_STRING";
		$valueAssign = "char    *value = IDL_VarGetString(argv[$argc]);";
		$cType = "string";
		$resAssign = "result = IDL_StrToSTRING((char *) valueptr->c_str());";
        $resPtrAssign = "IDL_StrStore(&result_ptr[i], (char *) (*valueptr)[i].c_str());";
		$nullVecAssign = "valueptr = &stringNullVec;";
	}
	elsif ($type =~ /long/) {
		$idlTypeSpec = "IDL_TYP_LONG";
		$idlType = "IDL_LONG";
		$valueAssign = "IDL_LONG value = IDL_LongScalar(argv[$argc]);";
		$cType = "long";
		$resAssign = "result = IDL_GettmpLong((IDL_LONG) *valueptr);";
        $resPtrAssign = "result_ptr[i] = (IDL_LONG) (*valueptr)[i];";
		$nullVecAssign = "valueptr = &longNullVec;";
	}
	elsif ($type =~ /double/) {
		$idlTypeSpec = "IDL_TYP_DOUBLE";
		$idlType = "double";
		$cType = "double";
		$resAssign = "result = IDL_Gettmp(); IDL_ALLTYPES v; v.l = *valueptr; IDL_StoreScalar(result, IDL_TYP_DOUBLE, &v)";
        $resPtrAssign = "result_ptr[i] = (double) (*valueptr)[i];";
		$nullVecAssign = "valueptr = &doubleNullVec;";
	}
	else {
		die "Invalid type : $type";
	}


	if ($vector) {
		$valueType = "const vector<$cType> ";
		$nElts = "valueptr->size()";
		$valVecAssign = "valuevec[i] = valvar->value.data[i];";
    	$resAssign = "$idlType *result_ptr = ($idlType *) IDL_MakeTempVector( $idlTypeSpec, valueptr->size(), IDL_ARR_INI_NOP, &result);";
	}
	else {
		$valueType = "const $cType ";
		$nElts = "0";
		$valVecAssign = "valuevec[i] = ($cType) value;";
        $resPtrAssign = "";
		$nullVecAssign = "valueptr = &value;";
	}
		

print <<"EOF";

IDL_VPTR vdf$name(int argc, IDL_VPTR *argv)
{

    Metadata    *metadata = varGetMetadata(argv[0]);
    $tsCheck
    $varCheck
    $tagCheck

    $tsAssign
    $varAssign
    $tagAssign

    $varVecAssign
    $tagVecAssign

	int n;

    $valueType &value = metadata->$name($params);
    $valueType *valueptr = &value;
    if (int rc = Metadata::GetErrCode()) {
		if (rc != XmlNode::ERR_TNP) {
			myBaseErrChk();
        }
        else {
			IDL_Message(
				IDL_M_NAMED_GENERIC, IDL_MSG_INFO, 
				"Requested element not present in metafile"
			);
			$nullVecAssign
        }
    }

	n = $nElts;

    IDL_VPTR result;
	$resAssign

    for(int i=0; i<n; i++) {
		$resPtrAssign
    }

	return(result);

}

EOF

}


sub doProcTable {

	$first = 1;
	foreach $r (@Table) {

		$del = "";
		if ($r->{set}) {
			$args = 2;
			if ($r->{ts}) {$args++;}
			if ($r->{var}) {$args++;}
			if ($r->{tag}) {$args++;}
			if (! $first) {$del = ",";}
			$first = 0;
			$name = $r->{name};
			$idlname = $r->{idlname};
			

print <<"EOF";
		$del
		{ (IDL_SYSRTN_GENERIC) vdf$name,
		\"$idlname\", $args, $args, 0, 0
		}
EOF
		}
	}
}

sub doFuncTable {

	$first = 1;
	foreach $r (@Table) {

		$del = "";
		if (! $r->{set}) {
			$args = 1;
			if ($r->{ts}) {$args++;}
			if ($r->{var}) {$args++;}
			if ($r->{tag}) {$args++;}
			if (! $first) {$del = ",";}
			$first = 0;
			$name = $r->{name};
			$idlname = $r->{idlname};
			

print <<"EOF";
		$del
		{ (IDL_SYSRTN_GENERIC) vdf$name,
		\"$idlname\", $args, $args, 0, 0
		}
EOF
		}
	}
}

sub doProcDLM {

	foreach $r (@Table) {

		$del = "";
		if ($r->{set}) {
			$args = 2;
			if ($r->{ts}) {$args++;}
			if ($r->{var}) {$args++;}
			if ($r->{tag}) {$args++;}
			$idlname = $r->{idlname};
			$idlname =~ tr/A-Z/a-z/;
			

print <<"EOF";
PROCEDURE	$idlname $args $args
EOF
		}
	}
}

sub doFuncDLM {

	foreach $r (@Table) {

		if (! $r->{set}) {
			$args = 1;
			if ($r->{ts}) {$args++;}
			if ($r->{var}) {$args++;}
			if ($r->{tag}) {$args++;}
			$name = $r->{name};
			$idlname = $r->{idlname};
			$idlname =~ tr/A-Z/a-z/;
			

print <<"EOF";
FUNCTION	$idlname $args $args
EOF
		}
	}
}

sub     usage {
	my($s, $exit) = @_;


	if (defined ($s)) {
		print STDERR "$ProgName: $s\n";
	}

	print STDERR "$ProgName < template_file > output_file\n";

	exit(1)
}

sub doSysFuncs() {
	foreach $r (@Table) {
		print STDERR "Processing $r->{name}\n";
		if ($r->{set}) {
			SetFunc($r->{name}, $r->{type}, $r->{vector}, $r->{ts}, $r->{var}, $r->{tag});
		} else {
			GetFunc($r->{name}, $r->{type}, $r->{vector}, $r->{ts}, $r->{var}, $r->{tag});

		}

	}
}

$0              =~ s/.*\///;
$ProgName       = $0;

if (@ARGV != 0) {
    usage("Wrong # args");
}


while ($_ = <STDIN>) {
	if ($_ =~ /IDL_SYS_FUNCS_HERE/) {
		doSysFuncs();
	}
	elsif ($_ =~ /IDL_FUNC_TABLE_HERE/) {
		doFuncTable();
	}
	elsif ($_ =~ /IDL_PROC_TABLE_HERE/) {
		doProcTable();
	}
	elsif ($_ =~ /IDL_DLM_PROCS_HERE/) {
		doProcDLM();
	}
	elsif ($_ =~ /IDL_DLM_FUNCS_HERE/) {
		doFuncDLM();
	}
	else {
		print $_
	}
}

exit 0;
