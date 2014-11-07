#include <iostream>
#include <cstdio>
#include <cstring>
#include <vector>
#include <sstream>
#include <algorithm>
#include <vapor/OptionParser.h>
#include <vapor/MetadataVDC.h>
#include <vapor/DCReaderNCDF.h>
#include <vapor/VDCFactory.h>
#include <vapor/CFuncs.h>

#ifdef WIN32
#pragma warning(disable : 4996)
#endif
using namespace VetsUtil;
using namespace VAPoR;

struct opt_t {
	OptionParser::Dimension3D_T	dims;
	vector <string> vars;
	vector <string> timedims;
	vector <string> timevars;
	vector <string> stagdims;
	string missattr;
	string xcoordvar;
	string ycoordvar;
	string zcoordvar;
	OptionParser::Boolean_T	vdc2;
	OptionParser::Boolean_T	misstv;
	OptionParser::Boolean_T	help;
	OptionParser::Boolean_T	quiet;
	OptionParser::Boolean_T	debug;
} opt;

OptionParser::OptDescRec_T	set_opts[] = {
	{"dims",		1, 	"-1x-1x-1",	"Spatial dimensions of 3D variables. "
		"Defaults to dimensions of first 3D variable found in netCDF files."},
	{"vars",1,    "",	"Colon delimited list of variables to be copied "
		"from ncdf data. The default is to copy all 2D and 3D variables"},
	{"timedims",	1,	"",	"Colon delimited list of time dimension variable "
		"names"},
	{"timevars",	1,	"",	"Colon delimited list of time coordinate "
		"variables. If this 1D variable "
		"is present, it specfies the time in user-defined units for each "
		"time step. This option takes precedence over -usertime or -startt."},
	{"stagdims",	1,	"",	"Colon delimited list of staggered dimension names"},
	{"missattr",	1,	"",	"Name of netCDF attribute specifying missing value "
		" This option takes precdence over -missing"},
	{"xcoordvar",	1,	"NONE",	"Name of netCDF variable containing X dimension "
		"grid coordinates. This option is ignored if the grid type (-gridtype) "
		"is not layered or stretched"
	},
	{"ycoordvar",	1,	"NONE",	"Name of netCDF variable containing Y dimension "
		"grid coordinates. This option is ignored if the grid type (-gridtype) "
		"is not layered or stretched"
	},
	{"zcoordvar",	1,	"NONE",	"Name of netCDF variable containing Z dimension "
		"grid coordinates. This option is ignored if the grid type (-gridtype) "
		"is not layered or stretched"
	},
	{
		"vdc2", 0,  "",
		"Generate a VDC Type 2 .vdf file (default is VDC Type 1)"
	},
	{"misstv",	0,	"",	"Set this option if missing data locations vary "
		"between time steps and/or variables"},
	{"help",	0,	"",	"Print this message and exit"},
	{"quiet",	0,	"",	"Operate quietly"},
	{"debug",	0,	"",	"Turn on debugging"},
	{NULL}
};

OptionParser::Option_T	get_options[] = {
	{"vars", VetsUtil::CvtToStrVec, &opt.vars, sizeof(opt.vars)},
	{"dims", VetsUtil::CvtToDimension3D, &opt.dims, sizeof(opt.dims)},
	{"timedims", VetsUtil::CvtToStrVec, &opt.timedims, sizeof(opt.timedims)},
	{"timevars", VetsUtil::CvtToStrVec, &opt.timevars, sizeof(opt.timevars)},
	{"stagdims", VetsUtil::CvtToStrVec, &opt.stagdims, sizeof(opt.stagdims)},
	{"missattr", VetsUtil::CvtToCPPStr, &opt.missattr, sizeof(opt.missattr)},
	{"xcoordvar", VetsUtil::CvtToCPPStr, &opt.xcoordvar, sizeof(opt.xcoordvar)},
	{"ycoordvar", VetsUtil::CvtToCPPStr, &opt.ycoordvar, sizeof(opt.ycoordvar)},
	{"zcoordvar", VetsUtil::CvtToCPPStr, &opt.zcoordvar, sizeof(opt.zcoordvar)},
	{"vdc2", VetsUtil::CvtToBoolean, &opt.vdc2, sizeof(opt.vdc2)},
	{"misstv", VetsUtil::CvtToBoolean, &opt.misstv, sizeof(opt.misstv)},
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

void GetCoordinates(
	NetCDFCollection *ncdfc,
	size_t ts,
	string coordvar, 
	size_t dim,
	vector <double> &coords
) {
	coords.clear();

	int rc;

	rc = ncdfc->OpenRead(ts, coordvar);
	if (rc<0) exit(1);

	size_t start[] = {0};
	size_t count[] = {dim};

	float *buffer = new float[dim];
	rc = ncdfc->Read(start, count, buffer);
	if (rc<0) exit(1);

	for (int i=0; i<dim; i++) {
		coords.push_back(buffer[i]);
	}
	delete [] buffer;

	(void) ncdfc->Close();
}

	

MetadataVDC *CreateMetadataVDC(
	const VDCFactory &vdcf,
	const DCReaderNCDF *ncdfData
) {

	size_t dims[3];
	ncdfData->GetGridDim(dims);

	//
	// Create default MetadataVDC object based on command line
	// options
	//
	MetadataVDC *file;
	file = vdcf.New(dims);
	if (MetadataVDC::GetErrCode() != 0) exit(1);

	// Copy values over from DCReaderNCDF to MetadataVDC.
	// Add checking of return values and error messsages.
	//
	if(file->SetNumTimeSteps(ncdfData->GetNumTimeSteps())) {
		cerr << "Error populating NumTimeSteps." << endl;
		exit(1);
	}

	//
	//
	if (opt.missattr.size() && !opt.misstv) {
		file->SetMissingValue(1e37);
	}

	//
	// By default we populate the VDC with all 2D and 3D variables found in 
	// the netCDF files. This can be overriden by the "-vars" option
	// 

	vector <string> candidate_vars, vars;
	candidate_vars = ncdfData->GetVariables3D();
	if (! opt.vars.size()) {
		vars = candidate_vars;
	}
	else {

		vars.clear();
		for (int i=0; i<opt.vars.size(); i++) {
			vector <string>::iterator itr = find(candidate_vars.begin(), candidate_vars.end(), opt.vars[i]);
			if (itr != candidate_vars.end()) {
				vars.push_back(opt.vars[i]);
			}
		}
	}
	if(file->SetVariables3D(vars)) {
		cerr << "Error populating Variables." << endl;
		exit(1);
	}

	candidate_vars = ncdfData->GetVariables2DXY();
	if (! opt.vars.size()) {
		vars = candidate_vars;
	}
	else {

		vars.clear();
		for (int i=0; i<opt.vars.size(); i++) {
			vector <string>::iterator itr = find(candidate_vars.begin(), candidate_vars.end(), opt.vars[i]);
			if (itr != candidate_vars.end()) {
				vars.push_back(opt.vars[i]);
			}
		}
	}
	if(file->SetVariables2DXY(vars)) {
		cerr << "Error populating Variables." << endl;
		exit(1);
	}

	candidate_vars = ncdfData->GetVariables2DXZ();
	if (! opt.vars.size()) {
		vars = candidate_vars;
	}
	else {

		vars.clear();
		for (int i=0; i<opt.vars.size(); i++) {
			vector <string>::iterator itr = find(candidate_vars.begin(), candidate_vars.end(), opt.vars[i]);
			if (itr != candidate_vars.end()) {
				vars.push_back(opt.vars[i]);
			}
		}
	}
	if(file->SetVariables2DXZ(vars)) {
		cerr << "Error populating Variables." << endl;
		exit(1);
	}

	candidate_vars = ncdfData->GetVariables2DYZ();
	if (! opt.vars.size()) {
		vars = candidate_vars;
	}
	else {

		vars.clear();
		for (int i=0; i<opt.vars.size(); i++) {
			vector <string>::iterator itr = find(candidate_vars.begin(), candidate_vars.end(), opt.vars[i]);
			if (itr != candidate_vars.end()) {
				vars.push_back(opt.vars[i]);
			}
		}
	}
	if(file->SetVariables2DYZ(vars)) {
		cerr << "Error populating Variables." << endl;
		exit(1);
	}

	if (opt.missattr.size() && opt.misstv) {
		vars = file->GetVariableNames();
		for(int i = 0; i < ncdfData->GetNumTimeSteps(); i++) {

			for (int j=0; j<vars.size(); j++) {
				file->SetVMissingValue(i,vars[j], 1e37);
			}
		}
	}


	vector <double> usertime;
	for(int i = 0; i < ncdfData->GetNumTimeSteps(); i++) {

		usertime.clear();
		usertime.push_back(ncdfData->GetTSUserTime(i));
		if(file->SetTSUserTime(i, usertime)) {
			cerr << "Error populating TSUserTime." << endl;
			exit(1);
		}
	}

	string gridtype = file->GetGridType();

	if (gridtype.compare("layered")==0) {
		vector <string> vec;
		vec.push_back("NONE");
		vec.push_back("NONE");
		vec.push_back(opt.zcoordvar);
		file->SetCoordinateVariables(vec);
	}
	else if (gridtype.compare("stretched")==0) {
		NetCDFCollection *ncdfc = ncdfData->GetNetCDFCollection();
		for(size_t ts = 0; ts < ncdfData->GetNumTimeSteps(); ts++) {
			vector <double> coords;
			if (! (opt.xcoordvar.compare("NONE")==0)) {
				GetCoordinates(ncdfc, ts, opt.xcoordvar, dims[0], coords);
			}
			else {
				// Ugh. Stupid hack to fix bug #1007. The VDCFactory
				// object parses the -{x,y,z}coords option and sets 
				// the coordinates, but only for the first time step.
				// Need to grab coordinates from ts=0, and set the rest of
				// the time steps
				//
				coords = file->GetTSXCoords(0);
			}
			int rc = file->SetTSXCoords(ts, coords);
			if (rc<0) exit(1);

			if (! (opt.ycoordvar.compare("NONE")==0)) {
				GetCoordinates(ncdfc, ts, opt.ycoordvar, dims[0], coords);
			}
			else {
				coords = file->GetTSYCoords(0);
			}
			rc = file->SetTSYCoords(ts, coords);
			if (rc<0) exit(1);

			if (! (opt.zcoordvar.compare("NONE")==0)) {
				GetCoordinates(ncdfc, ts, opt.zcoordvar, dims[0], coords);
			}
			else {
				coords = file->GetTSZCoords(0);
			}
			rc = file->SetTSZCoords(ts, coords);
			if (rc<0) exit(1);

		}
	}

	return(file);
}


int	main(int argc, char **argv) {

    MyBase::SetErrMsgFilePtr(stderr);

	OptionParser op;

	string s;
	DCReaderNCDF *ncdfData;

	ProgName = Basename(argv[0]);

	if (op.AppendOptions(set_opts) < 0) {
		exit(1);
	}

	if (op.ParseOptions(&argc, argv, get_options) < 0) {
		exit(1);
	}
	if (opt.debug) MyBase::SetDiagMsgFilePtr(stderr);

	VDCFactory vdcf(opt.vdc2);
	if (vdcf.Parse(&argc, argv) < 0) {
		exit(1);
	}

	if (opt.help) {
		Usage(op, NULL);
		vdcf.Usage(stderr);
		exit(0);
	}


	argv++;
	argc--;

	if (argc < 2) {
		Usage(op, "No files to process");
		vdcf.Usage(stderr);
		exit(1);
	}

	vector<string> ncdffiles;
	for (int i=0; i<argc-1; i++) {
		 ncdffiles.push_back(argv[i]);
	}
	
	//
	// If the "-dimension" option was present it specifies the grid
	// dimensions (which must be present in the netCDF files), otherwise
	// let DCReaderNCDF figure out the grid size
	//
	size_t *dimsptr = NULL;
	size_t dims[3];
	if (opt.dims.nx != -1 && opt.dims.ny != -1 && opt.dims.nz != -1) {
		dims[0] = opt.dims.nx; dims[1] = opt.dims.ny; dims[2] = opt.dims.nz;
		dimsptr = dims;
	}
	ncdfData = new DCReaderNCDF(
		ncdffiles, opt.timedims, opt.timevars, opt.stagdims, opt.missattr,
		dimsptr
	);
	if (MyBase::GetErrCode() != 0) exit(1);

	if(ncdfData->GetNumTimeSteps() < 0) {
		cerr << "No output file generated due to no input files processed." << endl;
		exit(0);
	}

	//
	// Create a MetadataVDC object
	//
	MetadataVDC *file;
	file = CreateMetadataVDC(vdcf, ncdfData);

	if (file->GetVDCType() == 1 && opt.missattr.length()) {
		cerr << "Missing values not supported with VDC Type 1\n";
		exit(1);
	}

	// Write file.

	if (file->Write(argv[argc-1]) < 0) {
		exit(1);
	}

	if (! opt.quiet && ncdfData->GetNumTimeSteps() > 0) {
		cout << "Created VDF file:" << endl;
		cout << "\tNum time steps : " << file->GetNumTimeSteps() << endl;

		cout << "\t3D Variable names : ";
		for (int i = 0; i < file->GetVariables3D().size(); i++) {
			cout << file->GetVariables3D()[i] << " ";
		}
		cout << endl;

		cout << "\t2DXY Variable names : ";
		for (int i=0; i < file->GetVariables2DXY().size(); i++) {
			cout << file->GetVariables2DXY()[i] << " ";
		}
		cout << endl;

		cout << "\t2DXZ Variable names : ";
		for (int i=0; i < file->GetVariables2DXZ().size(); i++) {
			cout << file->GetVariables2DXZ()[i] << " ";
		}
		cout << endl;

		cout << "\t2DYZ Variable names : ";
		for (int i=0; i < file->GetVariables2DYZ().size(); i++) {
			cout << file->GetVariables2DYZ()[i] << " ";
		}
		cout << endl;

		cout << "\tExcluded 3D Variable names : ";
		for (int i = 0; i < ncdfData->GetVariables3DExcluded().size(); i++) {
			cout << ncdfData->GetVariables3DExcluded()[i] << " ";
		}
		cout << endl;

		cout << "\tExcluded 2D Variable names : ";
		for (int i = 0; i < ncdfData->GetVariables2DExcluded().size(); i++) {
			cout << ncdfData->GetVariables2DExcluded()[i] << " ";
		}
		cout << endl;

		cout << "\tCoordinate extents : ";
		const vector <double> extptr = file->GetExtents();
		for(int i=0; i<6; i++) {
			cout << extptr[i] << " ";
		}
		cout << endl;
		
	} // End if quiet.

	return(0);
} // End of main.
