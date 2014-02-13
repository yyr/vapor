#include <iostream>
#include <cstdio>
#include <cstring>
#include <vector>
#include <sstream>
#include <cassert>
#include <algorithm>
#include <vapor/OptionParser.h>
#include <vapor/NetCDFCFCollection.h>
#include <vapor/CFuncs.h>
#include <vapor/VDCNetCDF.h>

#ifdef WIN32
#pragma warning(disable : 4996)
#endif
using namespace VetsUtil;
using namespace VAPoR;

struct opt_t {
	vector <string> vars;
	OptionParser::Boolean_T	help;
	OptionParser::Boolean_T	quiet;
	OptionParser::Boolean_T	debug;
} opt;

OptionParser::OptDescRec_T	set_opts[] = {
	{"vars",1,    "",	"Colon delimited list of variables to be copied "
		"from ncdf data. The default is to copy all 2D and 3D variables"},
	{"help",	0,	"",	"Print this message and exit"},
	{"quiet",	0,	"",	"Operate quietly"},
	{"debug",	0,	"",	"Turn on debugging"},
	{NULL}
};

OptionParser::Option_T	get_options[] = {
	{"vars", VetsUtil::CvtToStrVec, &opt.vars, sizeof(opt.vars)},
	{"help", VetsUtil::CvtToBoolean, &opt.help, sizeof(opt.help)},
	{"quiet", VetsUtil::CvtToBoolean, &opt.quiet, sizeof(opt.quiet)},
	{"debug", VetsUtil::CvtToBoolean, &opt.debug, sizeof(opt.debug)},
	{NULL}
};

const char *ProgName;

void Usage(OptionParser &op, const char * msg) {

	if (msg) {
		cerr << ProgName << " : " << msg << endl;
	}
	cerr << "Usage: " << ProgName << " [options] ncdf_file... vdf_file" << endl;
	op.PrintOptionHelp(stderr, 80, false);

}


//
// Print a coordinate variable from the VDC
//
void print_vdc_cvar(
	string varname, vector <string> dimnames, string units, int axis, 
	VDC::XType type, bool compressed, bool uniform
) {
	
	cout << "	Varname : " << varname << endl;
	cout << "		Dimnames : "; 
	for (int i=0; i<dimnames.size(); i++) {
		cout << dimnames[i] << " ";
	}
	cout << endl;
	cout << "		Units: " << units << endl;
	cout << "		Axis: " << axis << endl;
	cout << "		XType: " << type << endl;
	cout << "		Compressed: " << compressed << endl;
	cout << "		Uniform: " << uniform << endl;
}

//
// Print a data varaible from the VDC
//
void print_vdc_var(
	string varname, vector <string> dimnames, vector <string> cvars,
	string units, 
	VDC::XType type, bool compressed, bool has_missing, double missing_value
) {
	cout << "	Varname : " << varname << endl;
	cout << "		Dimnames : "; 
	for (int i=0; i<dimnames.size(); i++) {
		cout << dimnames[i] << " ";
	}
	cout << endl;
	cout << "		Coordvars : "; 
	for (int i=0; i<cvars.size(); i++) {
		cout << cvars[i] << " ";
	}
	cout << endl;
	cout << "		Units: " << units << endl;
	cout << "		XType: " << type << endl;
	cout << "		Compressed: " << compressed << endl;
	cout << "		HasMissing: " << has_missing << endl;
	cout << "		MissingValue: " << missing_value << endl;
}

void print_path(const VDC &vdc, const VDC::VarBase *var) {

	int numts = vdc.GetNumTimeSteps(var->GetName());
	assert (numts >=0);
	if (numts == 0) numts = 1;	// Not time varying
	for (int ts=0; ts<numts; ts++) {
		int n = var->GetCRatios().size();
		if (n==0) n = 1;
		for (int lod=0; lod<n; lod++) {
			string  path;
			size_t file_ts;
			vdc.GetPath(var->GetName(), ts, lod, path, file_ts);
			cout << "		Path: " << path << endl;
		}
	}
}

//
// Generate a list of variable names to copy from NetCDF to VDC
//
void get_var_names(
	const NetCDFCFCollection &cf, vector <string> clvarnames,
	vector <string> &vars1d, vector <string> &vars2d, vector <string> &vars3d
) {
	vars1d.clear();
	vars2d.clear();
	vars3d.clear();

	if (clvarnames.size()) {
		for (int i=0; i<clvarnames.size(); i++) {
			if (cf.GetSpatialDims(clvarnames[i]).size() == 1) {
				vars1d.push_back(clvarnames[i]);
			}
			else if (cf.GetSpatialDims(clvarnames[i]).size() == 2) {
				vars2d.push_back(clvarnames[i]);
			}
			else if (cf.GetSpatialDims(clvarnames[i]).size() == 3) {
				vars3d.push_back(clvarnames[i]);
			}
			else {
				MyBase::SetErrMsg(
					"Invalid variable name : %s", clvarnames[i].c_str()
				);
				exit(1);
			}
		}
	}
	else {
		vars1d = cf.GetDataVariableNames(1, true); 
		vars2d = cf.GetDataVariableNames(2, true); 
		vars3d = cf.GetDataVariableNames(3, true); 
	}
}


//
// Copy variable 'varname's attributes from cf to vdc
//
void copy_attributes(const NetCDFCFCollection &cf, VDC &vdc, string varname) {

	vector <string> atts;
	atts = cf.GetAttNames(varname);
	for (int i=0; i<atts.size(); i++) {
		int type = cf.GetAttType(varname, atts[i]);
		if (NetCDFSimple::IsNCTypeInt(type)) {
			vector <long> vec;
			cf.GetAtt(varname,atts[i], vec);
			vdc.PutAtt(varname,atts[i], VDC::INT64,vec);
		}
		else if (NetCDFSimple::IsNCTypeFloat(type)) {
			vector <double> vec;
			cf.GetAtt(varname,atts[i], vec);
			vdc.PutAtt(varname,atts[i], VDC::DOUBLE,vec);
		}
		else if (NetCDFSimple::IsNCTypeText(type)) {
			string s;
			cf.GetAtt(varname,atts[i], s);
			vdc.PutAtt(varname,atts[i], VDC::TEXT,s);
		}
	}
}

//
// Define dimensions in VDC from NetCDF files. The dimensions are
// determined by looking at a list of coordinate variables from the
// NetCDF files.
//
void define_dimensions(
	const NetCDFCFCollection &cf, VDC &vdc, 
	vector <string> cvars, int axis
) {
	size_t dimlen;

	for (int i=0; i<cvars.size(); i++) {

		// Only define dimensions for 1D coordinate variables
		//
		if (! cf.IsCoordVarCF(cvars[i])) continue;
 
		if (axis == 3) {	// time axis has different interface
			dimlen = cf.GetNumTimeSteps(cvars[i]);
		}
		else {
			vector <size_t> dimlens;
			dimlens = cf.GetDims(cvars[i]);

			// VDC and NetCDF have different dimension orderings
			//
			std::reverse(dimlens.begin(), dimlens.end());
			assert(dimlens.size() == 1);	// guaranteed to be 1D
			dimlen = dimlens[0];
		}
		vdc.DefineDimension(cvars[i], dimlen, axis);
	}
}

//
// Define coordinate variables using definitions from NetCDF
//
void define_coord_variables(
	const NetCDFCFCollection &cf, VDC &vdc, 
	vector <string> cvars, int axis
) {
	vector <string> dimnames;
	string units;

	for (int i=0; i<cvars.size(); i++) {

		if (axis == 3) {
			string time_dim = cf.GetTimeDimName(cvars[i]);
			dimnames.clear();
			dimnames.push_back(time_dim);
		}
		else {
			dimnames = cf.GetDimNames(cvars[i]);
		}
		std::reverse(dimnames.begin(), dimnames.end());
		cf.GetVarUnits(cvars[i], units);
		vdc.DefineCoordVar(
			cvars[i], dimnames, units, axis, VDC::FLOAT, false
		);

		//
		// copy all coordinate variable attributes
		//
		copy_attributes(cf, vdc, cvars[i]);
	}
}

//
// Define data variables using definitions from NetCDF
//
void define_data_variables(
	const NetCDFCFCollection &cf, VDC &vdc, 
	vector <string> vars, bool compress
) {
	double missing_value;

	vector <string> cvars;
	vector <string> dimnames;
	string units;

	for (int i=0; i<vars.size(); i++) {

		dimnames = cf.GetDimNames(vars[i]);

		// 
		// Ugh. Need to reverse order of dimensions: NetCDF convention
		// is slowest to fastest, while VAPOR (VDC) is fastest to slowest
		//
		std::reverse(dimnames.begin(), dimnames.end());
		cf.GetVarCoordVarNames(vars[i], cvars);
		std::reverse(cvars.begin(), cvars.end());
		cf.GetVarUnits(vars[i], units);
		if (cf.GetMissingValue(vars[i], missing_value)) {
			vdc.DefineDataVar(
				vars[i], dimnames, cvars, units, VDC::FLOAT, compress, 
				missing_value
			);
		}
		else {
			vdc.DefineDataVar(
				vars[i], dimnames, cvars, units, VDC::FLOAT, compress
			);
		}
		copy_attributes(cf, vdc, vars[i]);
	}
}

void print_attributes(const VDC &vdc, string varname, string prefix) {

	vector <string> atts = vdc.GetAttNames(varname);

	for (int i=0; i<atts.size(); i++) {
		int type = vdc.GetAttType(varname, atts[i]);
		cout << prefix << varname << ":" << atts[i] << " = ";
		if (type == VDC::INT32 || type == VDC::INT64) {
			vector <long> vec;
			vdc.GetAtt(varname,atts[i], vec);
			for (int i=0; i<vec.size(); i++) {
				cout << vec[i] << " ";
			}
			cout << endl;
		}
		else if (type == VDC::FLOAT || type == VDC::DOUBLE) {
			vector <double> vec;
			vdc.GetAtt(varname,atts[i], vec);
			for (int i=0; i<vec.size(); i++) {
				cout << vec[i] << " ";
			}
			cout << endl;
		}
		else if (type == VDC::TEXT) {
			string s;
			vdc.GetAtt(varname,atts[i], s);
			cout << s << endl;
		}
	}
}

int	main(int argc, char **argv) {

    MyBase::SetErrMsgFilePtr(stderr);

	OptionParser op;

	string s;

	ProgName = Basename(argv[0]);

	if (op.AppendOptions(set_opts) < 0) {
		exit(1);
	}

	if (op.ParseOptions(&argc, argv, get_options) < 0) {
		exit(1);
	}
	if (opt.debug) MyBase::SetDiagMsgFilePtr(stderr);

	if (opt.help) {
		Usage(op, NULL);
		exit(0);
	}


	argv++;
	argc--;

	if (argc < 2) {
		Usage(op, "No files to process");
		exit(1);
	}

	vector<string> ncdffiles;
	for (int i=0; i<argc-1; i++) {
		 ncdffiles.push_back(argv[i]);
	}

	string vdc_master = argv[argc-1];
	
	NetCDFCFCollection cf;
	int rc = cf.Initialize(ncdffiles);
	if (rc<0) exit(1);

	VDCNetCDF vdc;
	rc = vdc.Initialize(vdc_master, VDC::W);
	if (rc<0) exit(1);

	//
	// copy global attributes
	//
	copy_attributes(cf, vdc, "");

	//
	// Define dimension names and implicity define 1d coord vars
	//
	vector <string> cvars;
	cvars = cf.GetLonCoordVars();
	define_dimensions(cf, vdc, cvars, 0);

	cvars = cf.GetLatCoordVars();
	define_dimensions(cf, vdc, cvars, 1);

	cvars = cf.GetVertCoordVars();
	define_dimensions(cf, vdc, cvars, 2);

	cvars = cf.GetTimeCoordVars();
	define_dimensions(cf, vdc, cvars, 3);


	// 
	// Define coordinate variables
	//
	// N.B. VDC::DefineDimension() implicitly defines 1D "CF" coordinate
	// variables, but we redefine them here to get units correct
	//
	cvars = cf.GetLonCoordVars();
	define_coord_variables(cf, vdc, cvars, 0);

	cvars = cf.GetLatCoordVars();
	define_coord_variables(cf, vdc, cvars, 1);

	cvars = cf.GetVertCoordVars();
	define_coord_variables(cf, vdc, cvars, 2);

	cvars = cf.GetTimeCoordVars();
	define_coord_variables(cf, vdc, cvars, 3);


	//
	// Define data variables
	//

	vector <string> vars1d;
	vector <string> vars2d;
	vector <string> vars3d;
	get_var_names(cf, opt.vars, vars1d, vars2d, vars3d);

	define_data_variables(cf, vdc, vars1d, false);
	define_data_variables(cf, vdc, vars2d, true);
	define_data_variables(cf, vdc, vars3d, true);

	vdc.EndDefine();



	//
	// At this point the VDC contains all metadata definitions. 
	// Next we print out the VDC contents
	//

	
	cout << "Dimensions:" << endl;
	vector <string> dimnames;
	dimnames = vdc.GetDimensionNames();
	for (int i=0; i<dimnames.size(); i++) {
		size_t len;
		int axis;
		vdc.GetDimension(dimnames[i], -1, len, axis);
		
		cout << "	";
		cout << dimnames[i] << ": " << len << " (" << axis << ")" << endl;
	}

	
	int axis;
	bool compressed;
	bool uniform;
	VDC::XType type;
	string units;
	cout << "Coordinate vars:\n";
	for (int d=1; d<4; d++) {
		cvars = vdc.GetCoordVarNames(d, true);
		for (int i=0; i<cvars.size(); i++) {
			vdc.GetCoordVar(
				cvars[i], dimnames, units, axis, type, compressed, uniform
			);
			print_vdc_cvar(
				cvars[i], dimnames, units, axis,  type, compressed, uniform
			);
			VDC::CoordVar cvar;
			vdc.GetCoordVar(cvars[i], cvar);
			print_path(vdc, &cvar);
			cout << endl;
			print_attributes(vdc, cvars[i], "		");
		}
	}

	cout << "Data vars:\n";
	for (int d=1; d<5; d++) {
		vector <string> vars;
		vars = vdc.GetDataVarNames(d, true);
		for (int i=0; i<vars.size(); i++) {
			bool has_missing;
			double missing_value;

			vdc.GetDataVar(
				vars[i], dimnames, cvars, units, type, compressed,
				has_missing, missing_value
			);
			print_vdc_var(
				vars[i], dimnames, cvars, units, type,  compressed, 
				has_missing, missing_value
			);
			VDC::DataVar dvar;
			vdc.GetDataVar(vars[i], dvar);
			print_path(vdc, &dvar);
			cout << endl;
			print_attributes(vdc, vars[i], "		");
		}
	}

	cout << "Global attributes:\n";
	print_attributes(vdc, "", "	");

	exit(0);
}
